#include "server/basecombatcharacter.h"
#include "usercmd.h"
#include "server/playerlocaldata.h"
#include "PlayerState.h"
#include "game/server/iplayerinfo.h"
#include "hintsystem.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "simtimer.h"
#include "vprof.h"

class CustomCBasePlayer : CBasePlayer
{
	friend class TicklessJump;
public:
	using CBasePlayer::pl;
	using CBasePlayer::m_flWaterJumpTime;
};

class CustomCGameMovement : CGameMovement
{
public:
	friend class TicklessJump;
};