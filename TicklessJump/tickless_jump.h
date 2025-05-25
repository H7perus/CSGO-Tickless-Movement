#pragma once
#ifndef _TICKLESS_JUMP_H_
#define _TICKLESS_JUMP_H_

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

class TicklessJump : public ISmmPlugin
{
public:
    bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
    bool Unload(char *error, size_t maxlen);
    bool PreJump();
    bool PostJump();
    const char *GetAuthor() { return "H7per"; }
    const char *GetName() { return "Tickless Jumping"; }
    const char *GetDescription() { return "Effectively patches out the tickrate dependent component of jumping. Aims for 128 tick jumpheight/airtime."; }
    const char *GetURL() { return "https://github.com/H7perus/CSGO-Tickless-Movement"; }
    const char *GetLicense() { return "MIT"; }
    const char *GetVersion() { return "1.0.0"; }
    const char *GetDate() { return __DATE__; }
    const char *GetLogTag() { return "TLJ"; }

};

extern TicklessJump g_TicklessJump;

PLUGIN_GLOBALVARS();

#endif // _TICKLESS_JUMP_H_ 