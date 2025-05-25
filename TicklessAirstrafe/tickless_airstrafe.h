#pragma once
#ifndef _TICKLESS_AIRSTRAFE_H_
#define _TICKLESS_AIRSTRAFE_H_

#include "convar.h"
#include <ISmmPlugin.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <sh_vector.h>

void DisablePred() {
    ConVar* myCvar = g_pCVar->FindVar("cl_predict");
    if (myCvar) {
        myCvar->SetValue(0);
    }
}

class TicklessAirstrafe : public ISmmPlugin
{
public:
    bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
    bool Unload(char *error, size_t maxlen);
    void AirAccelerateTickless(Vector& wishdir, float wishspeed, float accel);
    void PostMove(CBasePlayer *pPlayer, CMoveData *pMove);
    void PostConnect(edict_t *pEntity, char const *playername);
    const char *GetAuthor() { return "H7per"; }
    const char *GetName() { return "Tickless Airstrafe"; }
    const char *GetDescription() { return "This plugin implements an analytical integration of air acceleration. It assumes that the viewangle has rotated continuously from the last tick to the current one, with no change in air acceleration over the tick interval."; }
    const char *GetURL() { return "https://github.com/H7perus/CSGO-Tickless-Movement"; }
    const char *GetLicense() { return "MIT"; }
    const char *GetVersion() { return "1.0.0"; }
    const char *GetDate() { return __DATE__; }
    const char *GetLogTag() { return "TLAS"; }

};

extern TicklessAirstrafe g_TicklessAirstrafe;

PLUGIN_GLOBALVARS();

#endif // _TICKLESS_AIRSTRAFE_H_ 
