#pragma once
#define PLATFORM_WINDOWS
#define COMPILER_MSVC
#define COMPILER_MSVC32 1



#include <stdio.h>
#include <chrono>
#include "tier0/platform.h"
#include "convar.h"
#include "basehandle.h"
#include "server/cbase.h"
#include "shared/gamemovement.h"
#include "custombaseplayer.h"
#include "ticklessMath.h"
#include "tickless_airstrafe.h"

IServerGameClients *gameclients = nullptr;
IGameMovement* SgmIface = nullptr;
CMoveData* mv = nullptr;
CGameMovement* gm = nullptr;
CGlobalVars *gpGlobals = nullptr;
ICvar *icvar = nullptr;

SH_DECL_MANUALHOOK3_void(AirAccelerate, 25, 0, 0, Vector&, float, float);

SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK2_void(IGameMovement, ProcessMovement, SH_NOATTRIB, 0, CBasePlayer*, CMoveData*);

TicklessAirstrafe g_TicklessAirstrafe;
ConVar sv_airaccelerate_tickless("sv_airaccelerate_tickless", "1", 0, "Toggles analytical 'tickless' air acceleration"); 
//Not implemented for now. Is intended to do things like print the current state, so that we can figure out where stuff is going wrong if there is a bug.
//ConVar sv_airaccelerate_tickless_debug("sv_airaccelerate_tickless_debug", "0", 0, "debug 'tickless' airstrafe states");
ConVar sv_airaccelerate_tickless_profile("sv_airaccelerate_tickless_profile", "1", 0, "prints out the runtime in milliseconds");



class BaseAccessor : public IConCommandBaseAccessor
 {
 public:
 	bool RegisterConCommandBase(ConCommandBase *pCommandBase)
 	{
                if (g_SMAPI && g_PLAPI) {
                    return META_REGCVAR(pCommandBase);
                }
                return false;
 	}
 } s_BaseAccessor;


PLUGIN_EXPOSE(TicklessAirstrafe, g_TicklessAirstrafe);
bool TicklessAirstrafe::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
    gpGlobals = ismm->GetCGlobals();


    PLUGIN_SAVEVARS();

    GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetServerFactory, SgmIface, IGameMovement, INTERFACENAME_GAMEMOVEMENT);

    g_pCVar = icvar;
    ConVar_Register(0, &s_BaseAccessor);

    gm = static_cast<CGameMovement*>(SgmIface);

    if (SgmIface)
    {
        SH_ADD_MANUALHOOK(AirAccelerate, SgmIface, SH_MEMBER(this, &TicklessAirstrafe::AirAccelerateTickless), false);
        SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this, &TicklessAirstrafe::PostConnect, true);
        SH_ADD_HOOK_MEMFUNC(IGameMovement, ProcessMovement, SgmIface, this, &TicklessAirstrafe::PostMove, true);
    }
    else
    {
        if (ismm) ismm->LogMsg(this, "Failed to load TLAS, SgmIface is null \n");
        return false;
    }
    DisablePred();
    return true;
}

void TicklessAirstrafe::PostConnect(edict_t *pEntity, char const *playername)
{
    DisablePred();
}

bool TicklessAirstrafe::Unload(char *error, size_t maxlen)
{
    ConVar_Unregister();
    if (g_SMAPI) 
    {
        g_SMAPI->LogMsg(this, "TicklessAirstrafe plugin unloaded.");
    }
    SH_REMOVE_MANUALHOOK(AirAccelerate, SgmIface, SH_MEMBER(this, &TicklessAirstrafe::AirAccelerateTickless), false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this, &TicklessAirstrafe::PostConnect, true);
    SH_REMOVE_HOOK_MEMFUNC(IGameMovement, ProcessMovement, SgmIface, this, &TicklessAirstrafe::PostMove, true);
    return true;
}

void TicklessAirstrafe::AirAccelerateTickless(Vector &wishdir, float wishspeed, float accel)
{
    mv = gm->GetMoveData();

    auto time_start = std::chrono::high_resolution_clock::now();
    CustomCBasePlayer *player = reinterpret_cast<CustomCBasePlayer *>(static_cast<CGameMovement*>(SgmIface)->player);
    float surfaceFriction = *(float *)((char *)player + 0x125c);
    float angle_start = m_vecPrevViewAngles[YAW] * M_PI / 180;
    float angle_stop = mv->m_vecViewAngles[YAW] * M_PI / 180;
    
    if (!sv_airaccelerate_tickless.GetBool() || angle_start == angle_stop)
    {
        RETURN_META(MRES_IGNORED);
    }

    float addspeed, accelspeed, currentspeed;
    float wishspd;

    if (wishspeed == 0)
        RETURN_META(MRES_SUPERCEDE);

    float t = 0; // H7per: we are not necessarily progressing through the tick all at once. 
    wishspd = wishspeed;
    if (player->pl.deadflag)
        RETURN_META(MRES_SUPERCEDE);

    if (player->m_flWaterJumpTime)
        RETURN_META(MRES_SUPERCEDE);

    // Determine veer amount
    currentspeed = mv->m_vecVelocity.Dot(wishdir);

    // 46.875 at 64 tick with full stamina
    accelspeed = accel * wishspeed * gpGlobals->frametime * surfaceFriction;
    // wishspeed becomes our maximum acceleration, the unit of time becomes the tick.

    Vector addedvel = Vector(0, 0, 0);
    double w = normalizeAngleRad(angle_stop - angle_start);

    // this is the actual r0 with respect to the tick. Local r0 for each step would be r0 + w * t
    float r0 = atan2(wishdir.y, wishdir.x) - w;

    int sign_w = (w > 0) - (w < 0);

    int state = 0;
    
    while (true)
    {
        
        double offset_to_vel = r0 + w * t - atan2(mv->m_vecVelocity.y + addedvel.y, mv->m_vecVelocity.x + addedvel.x);
        // DETERMINE CURRSPEED
        if (state == 0)
        {
            printf("STATE 0\n");
            currentspeed = cos(offset_to_vel) * (mv->m_vecVelocity + addedvel).Length2D();
            // will compute the first point in the future at which we meet the 30u/s limit.
            // TODO H7per: Verify that this actually works correctly

            if (currentspeed >= 30.f - FLT_EPSILON)
            {
                state = 1;
            }
            else
            {
                state = 3;
            }
        }
        // FIND NEXT TIME WE HIT 30 ALONG AXIS OF ACCEL
        if (state == 1)
        {
            printf("STATE 1\n");
            // since we know we are at >30, the next time we hit 30 must be in the direction of w, hence the sign.
            offset_to_vel = r0 + w * t - atan2(mv->m_vecVelocity.y + addedvel.y, mv->m_vecVelocity.x + addedvel.x);
            double rot = sign_w * (acos(double(30.f) / (mv->m_vecVelocity + addedvel).Length2D()));

            rot = double(rot - offset_to_vel);
            rot = normalizeAngleRad(rot);

            t += rot / w;
            offset_to_vel = r0 + w * t - atan2(mv->m_vecVelocity.y + addedvel.y, mv->m_vecVelocity.x + addedvel.x);
            //for debugging purposes
            //float dot = Vector(cos(r0 + w * t), sin(r0 + w * t), 0).Dot(mv->m_vecVelocity + addedvel);

            state = 2;
        }
        // FIND CORRECT AIR ACCEL MODE
        if (state == 2)
        {
            printf("STATE 2\n");
            if (t >= 1.0)
            {
                state = 5;
            }
            else
            {
                float start_accel = sin(offset_to_vel) * w * (mv->m_vecVelocity + addedvel).Length2D();
                //for debugging purposes
                //float dot = Vector(cos(r0 + w * t), sin(r0 + w * t), 0).Dot(mv->m_vecVelocity + addedvel);
                if (start_accel > accelspeed)
                {

                    state = 3;
                }
                else
                {
                    state = 4;
                }
            }
        }
        // DO CONSTANT ACCEL INTEGRATION
        if (state == 3)
        {
            printf("STATE 3\n");
            state = integrateConstantAccel(mv->m_vecVelocity, addedvel, accelspeed, r0, w, t);
        }
        // DO CONSTANT VEL ALONG AXIS OF ACCEL INTEGRATION
        if (state == 4)
        {
            printf("STATE 4\n");
            state = integrateConstantDotProductSpeed(mv->m_vecVelocity, addedvel, accelspeed, r0, w, t);
        }
        // END
        if (state == 5)
        {
            mv->m_vecVelocity += addedvel;
            mv->m_outWishVel += addedvel;
            currentspeed = mv->m_vecVelocity.Dot(wishdir);
            printf("OUT\n");
            if (currentspeed > 30.001f)
                Msg("The currentspeed exceeded max at the end: %.2f \n", currentspeed);
            printf("END\n");

            Msg("added vel on this step: %.2f \n", addedvel.Length2D());

            if(sv_airaccelerate_tickless_profile.GetBool())
            {
                Msg("TLAS execution time:  %.3f ms \n", 
                std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_start).count() / 1000.f);
            }

            RETURN_META(MRES_SUPERCEDE);
        }
    }
}

void TicklessAirstrafe::PostMove(CBasePlayer *pPlayer, CMoveData *pMove)
{
    m_vecPrevViewAngles = pMove->m_vecViewAngles;
}