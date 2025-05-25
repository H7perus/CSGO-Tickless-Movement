#pragma once
#define PLATFORM_WINDOWS
#define COMPILER_MSVC
#define COMPILER_MSVC32 1

#define IN_JUMP (1 << 1)
#define IN_DUCK (1 << 2)
#define STAMINA_RANGE 100.0

enum PlayerAnimEvent_t
{
    PLAYERANIMEVENT_FIRE_GUN_PRIMARY = 0,
    PLAYERANIMEVENT_FIRE_GUN_PRIMARY_OPT, // an optional primary attack for variation in animation. For example, the knife toggles between left AND right slash animations.
    PLAYERANIMEVENT_FIRE_GUN_PRIMARY_SPECIAL1,
    PLAYERANIMEVENT_FIRE_GUN_PRIMARY_OPT_SPECIAL1, // an optional primary special attack for variation in animation.
    PLAYERANIMEVENT_FIRE_GUN_SECONDARY,
    PLAYERANIMEVENT_FIRE_GUN_SECONDARY_SPECIAL1,
    PLAYERANIMEVENT_GRENADE_PULL_PIN,
    PLAYERANIMEVENT_THROW_GRENADE,
    PLAYERANIMEVENT_JUMP,
    PLAYERANIMEVENT_RELOAD,
    PLAYERANIMEVENT_RELOAD_START, ///< w_model partial reload for shotguns
    PLAYERANIMEVENT_RELOAD_LOOP,  ///< w_model partial reload for shotguns
    PLAYERANIMEVENT_RELOAD_END,   ///< w_model partial reload for shotguns
    PLAYERANIMEVENT_CLEAR_FIRING, ///< clear animations on the firing layer
    PLAYERANIMEVENT_DEPLOY,       ///< Used to play deploy animations on third person models.
    PLAYERANIMEVENT_SILENCER_ATTACH,
    PLAYERANIMEVENT_SILENCER_DETACH,

    // new events
    PLAYERANIMEVENT_THROW_GRENADE_UNDERHAND,
    PLAYERANIMEVENT_CATCH_WEAPON,
    PLAYERANIMEVENT_COUNT
};

#include <cfloat>
#include <cmath>
#include <stdio.h>
#include <chrono>
#include "tier0/platform.h"
#include "convar.h"
#include "basehandle.h"
#include "basetypes.h"
// #include "ehandle.h"
#include "server/cbase.h"
#include "shared/gamemovement.h"
#include "custombaseplayer.h"
#include "iplayeranimstate.h"

#include "tickless_jump.h"

IServerGameClients *gameclients = nullptr;
IGameMovement *SgmIface = nullptr;
CMoveData *mv = nullptr;
CustomCGameMovement *gm = nullptr;
CGlobalVars *gpGlobals = nullptr;
ICvar *icvar = nullptr;

bool isJumping = false;

float m_outWishVel_z = 0.0;

ConVar sv_tickless_jump("sv_tickless_jump", "1", 0, "enables tickless jumping, which nukes an extra half tick of gravity");
ConVar sv_tickless_jump_uncrouchedheight("sv_tickless_jump_uncrouchedheight", "55.82644189", 0, "sets the uncrouched jumpheight, defaults to the value for 128 tick");
ConVar sv_tickless_jump_crouchedheight("sv_tickless_jump_crouchedheight", "57.0", 0, "sets the crouched jumpheight, this was always the same across tickrates");

SH_DECL_MANUALHOOK0_void(FinishGravity, 36, 0, 0);
SH_DECL_MANUALHOOK0(CheckJumpButton, 39, 0, 0, bool);

class BaseAccessor : public IConCommandBaseAccessor
{
public:
    bool RegisterConCommandBase(ConCommandBase *pCommandBase)
    {
        if (g_SMAPI && g_PLAPI)
        {
            return META_REGCVAR(pCommandBase);
        }
        return false;
    }
} s_BaseAccessor;

TicklessJump g_TicklessJump;
PLUGIN_EXPOSE(TicklessJump, g_TicklessJump);

bool TicklessJump::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
    gpGlobals = ismm->GetCGlobals();
    PLUGIN_SAVEVARS();

    GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetServerFactory, SgmIface, IGameMovement, INTERFACENAME_GAMEMOVEMENT);

    g_pCVar = icvar;
    ConVar_Register(0, &s_BaseAccessor);

    gm = reinterpret_cast<CustomCGameMovement *>(SgmIface);

    if (SgmIface)
    {
        SH_ADD_MANUALHOOK(CheckJumpButton, SgmIface, SH_MEMBER(this, &TicklessJump::PreJump), false);
        SH_ADD_MANUALHOOK(CheckJumpButton, SgmIface, SH_MEMBER(this, &TicklessJump::PostJump), true);
    }
    else
    {
        if (ismm)
            ismm->LogMsg(this, "Failed to load TLJ, SgmIface is null \n");
        return false;
    }
    return true;
}

bool TicklessJump::Unload(char *error, size_t maxlen)
{
    ConVar_Unregister();
    if (g_SMAPI)
    {
        g_SMAPI->LogMsg(this, "TicklessJump plugin unloaded.");
    }
    return true;
}

// void SetGroundEntity( CBaseEntity *player, CBaseEntity *ground )
// {
//     if ( player->m_hGroundEntity.Get() == ground )
// 		return;
// }

bool TicklessJump::PreJump()
{
    mv = gm->GetMoveData();
    m_outWishVel_z = mv ->m_vecVelocity.z;
    RETURN_META_VALUE(MRES_IGNORED, true);
}

bool TicklessJump::PostJump()
{
    if(!sv_tickless_jump.GetBool())
        return false;
    
    ConVar &sv_gravity = *icvar->FindVar("sv_gravity");
    mv = gm->GetMoveData();
    m_outWishVel_z = mv ->m_vecVelocity.z - m_outWishVel_z;
    CustomCBasePlayer *player = reinterpret_cast<CustomCBasePlayer *>(static_cast<CGameMovement *>(SgmIface)->player);

    //H7per: this breaks if we are on a moving platform that has a vertical velocity of 80 or 100...woopsie daisy. This is a hack for water jumps
    if(mv ->m_vecVelocity.z == 80.f || mv ->m_vecVelocity.z == 100.f)
        return false;
    //H7per: First, we remove whatever impulse they are adding:
    mv->m_vecVelocity.z -= m_outWishVel_z;
    //Then we readd the correct impulse, based on if it was a crouchjump or not:
    float impulse = 0.f;

    if(m_outWishVel_z == 0)
    {
        return false;
    }

    if(m_outWishVel_z > 300.f)
    {
        impulse = sqrt( sv_gravity.GetFloat() * 2 * sv_tickless_jump_crouchedheight.GetFloat());
    }
    else
    {
        impulse = sqrt( sv_gravity.GetFloat() * 2 * sv_tickless_jump_uncrouchedheight.GetFloat());
    }
    mv->m_vecVelocity.z += impulse;
    isJumping = false;

    return true;
}

