#include "server/basecombatcharacter.h"
#include "usercmd.h"
#include "server/playerlocaldata.h"
#include "PlayerState.h"
#include "game/server/iplayerinfo.h"
#include "hintsystem.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "simtimer.h"
#include "vprof.h"

class CustomCBasePlayer : public CBaseCombatCharacter
{
public:
	DECLARE_CLASS( CustomCBasePlayer, CBaseCombatCharacter );
protected:
	// HACK FOR BOTS
	friend class CBotManager;
	static edict_t *s_PlayerEdict; // must be set before calling constructor
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	// script description
	DECLARE_ENT_SCRIPTDESC();
	
	CustomCBasePlayer();
	~CustomCBasePlayer();

	void					StartUserMessageThrottling( char const *pchMessageNames[], int nNumMessageNames );
	void					FinishUserMessageThrottling();

	bool					ShouldThrottleUserMessage( char const *pchMessageName );

	
	// IPlayerInfo passthrough (because we can't do multiple inheritance)
	IPlayerInfo *GetPlayerInfo() { return &m_PlayerInfo; }
	IBotController *GetBotController() { return &m_PlayerInfo; }

	virtual void			SetModel( const char *szModelName );
	void					SetBodyPitch( float flPitch );

	virtual void			UpdateOnRemove( void );

	static CustomCBasePlayer		*CreatePlayer( const char *className, edict_t *ed );

	virtual void			CreateViewModel( int viewmodelindex = 0 );
	CBaseViewModel			*GetViewModel( int viewmodelindex = 0 );
	void					HideViewModels( void );
	void					DestroyViewModels( void );

	CPlayerState			*PlayerData( void ) { return &pl; }
	
	int						RequiredEdictIndex( void ) { return ENTINDEX(edict()); } 

	void					LockPlayerInPlace( void );
	void					UnlockPlayer( void );

	virtual void			DrawDebugGeometryOverlays(void);
	
	// Networking is about to update this entity, let it override and specify it's own pvs
	virtual void			SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize );
	virtual int				UpdateTransmitState();
	virtual int				ShouldTransmit( const CCheckTransmitInfo *pInfo );

	// Returns true if this player wants pPlayer to be moved back in time when this player runs usercmds.
	// Saves a lot of overhead on the server if we can cull out entities that don't need to lag compensate
	// (like team members, entities out of our PVS, etc).
	virtual bool			WantsLagCompensationOnEntity( const CBaseEntity *entity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

	virtual void			Spawn( void );
	virtual void			Activate( void );
	virtual void			SharedSpawn(); // Shared between client and server.
	virtual void			ForceRespawn( void );

	virtual void			InitialSpawn( void );
	virtual void			InitHUD( void ) {}
	virtual void			ShowViewPortPanel( const char * name, bool bShow = true, KeyValues *data = NULL );

	virtual void			PlayerDeathThink( void );

	virtual void			Jump( void );
	virtual void			Duck( void );

	const char				*GetTracerType( void );
	void					MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void					DoImpactEffect( trace_t &tr, int nDamageType );

#if !defined( NO_ENTITY_PREDICTION )
	void					AddToPlayerSimulationList( CBaseEntity *other );
	void					RemoveFromPlayerSimulationList( CBaseEntity *other );
	void					SimulatePlayerSimulatedEntities( void );
	void					ClearPlayerSimulationList( void );
#endif

	// Physics simulation (player executes it's usercmd's here)
	virtual void			PhysicsSimulate( void );

	// Forces processing of usercmds (e.g., even if game is paused, etc.)
	void					ForceSimulation();

	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual void			PreThink( void );
	virtual void			PostThink( void );
	virtual int				TakeHealth( float flHealth, int bitsDamageType );
	virtual void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	bool					ShouldTakeDamageInCommentaryMode( const CTakeDamageInfo &inputInfo );
	virtual int				OnTakeDamage( const CTakeDamageInfo &info );
	virtual void			DamageEffect(float flDamage, int fDamageType);

	virtual void			OnDamagedByExplosion( const CTakeDamageInfo &info );

	void					PauseBonusProgress( bool bPause = true );
	void					SetBonusProgress( int iBonusProgress );
	void					SetBonusChallenge( int iBonusChallenge );

	int						GetBonusProgress() const { return m_iBonusProgress; }
	int						GetBonusChallenge() const { return m_iBonusChallenge; }

	virtual Vector			EyePosition( );			// position of eyes
	const QAngle			&EyeAngles( );
	void					EyePositionAndVectors( Vector *pPosition, Vector *pForward, Vector *pRight, Vector *pUp );
	virtual const QAngle	&LocalEyeAngles();		// Direction of eyes
	void					EyeVectors( Vector *pForward, Vector *pRight = NULL, Vector *pUp = NULL );
	void					CacheVehicleView( void );	// Calculate and cache the position of the player in the vehicle

	// Sets the view angles
	void					SnapEyeAngles( const QAngle &viewAngles );

	virtual QAngle			BodyAngles();
	virtual Vector			BodyTarget( const Vector &posSrc, bool bNoisy);
	virtual bool			ShouldFadeOnDeath( void ) { return FALSE; }
	
	virtual const impactdamagetable_t &GetPhysicsImpactDamageTable();
	virtual int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void			Event_Killed( const CTakeDamageInfo &info );
	// Notifier that I've killed some other entity. (called from Victim's Event_Killed).
	virtual void			Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	void					Event_Dying( void );

	bool					IsHLTV( void ) const { return pl.hltv; }
	bool					IsReplay( void ) const { return pl.replay; }
	virtual	bool			IsPlayer( void ) const { return true; }			// Spectators return TRUE for this, use IsObserver to seperate cases
	virtual bool			IsNetClient( void ) const { return true; }		// Bots should return FALSE for this, they can't receive NET messages
																			// Spectators should return TRUE for this

	virtual bool			IsFakeClient( void ) const;

	// Get the client index (entindex-1).
	int						GetClientIndex()	{ return ENTINDEX( edict() ) - 1; }

	// returns the player name
	const char *			GetPlayerName() const { return m_szNetname; }
	void					SetPlayerName( const char *name );

	virtual const char *	GetCharacterDisplayName() { return GetPlayerName(); }

	int						GetUserID() const { return engine->GetPlayerUserId( edict() ); }
	const char *			GetNetworkIDString(); 
	virtual const Vector	GetPlayerMins( void ) const; // uses local player
	virtual const Vector	GetPlayerMaxs( void ) const; // uses local player

	virtual void			UpdateCollisionBounds( void );

	void					VelocityPunch( const Vector &vecForce );
	void					ViewPunch( const QAngle &angleOffset );
	void					ViewPunchReset( float tolerance = 0 );
	void					ShowViewModel( bool bShow );
	void					ShowCrosshair( bool bShow );

	bool					ScriptIsPlayerNoclipping( void );
	virtual void			NoClipStateChanged( void ) { };

	// View model prediction setup
	void					CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );

	// Handle view smoothing when going up stairs
	void					SmoothViewOnStairs( Vector& eyeOrigin );
	virtual float			CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);
	void					CalcViewRoll( QAngle& eyeAngles );
	virtual void			CalcViewBob( Vector& eyeOrigin );

	virtual int				Save( ISave &save );
	virtual int				Restore( IRestore &restore );
	virtual bool			ShouldSavePhysics();
	virtual void			OnRestore( void );

	virtual void			PackDeadPlayerItems( void );
	virtual void			RemoveAllItems( bool removeSuit );
	bool					IsDead() const;


	bool					HasPhysicsFlag( unsigned int flag ) { return (m_afPhysicsFlags & flag) != 0; }

	virtual	CBaseCombatCharacter *ActivePlayerCombatCharacter( void ) { return this; }

	// Weapon stuff
	virtual Vector			Weapon_ShootPosition( );
	virtual bool			Weapon_CanUse( CBaseCombatWeapon *pWeapon );
	virtual void			Weapon_Equip( CBaseCombatWeapon *pWeapon );
	virtual	void			Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ );
	virtual	bool			Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );		// Switch to given weapon if has ammo (false if failed)
	virtual void			Weapon_SetLast( CBaseCombatWeapon *pWeapon );
	virtual bool			Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon ) { return true; }
	virtual bool			Weapon_ShouldSelectItem( CBaseCombatWeapon *pWeapon );
	void					Weapon_DropSlot( int weaponSlot );
	CBaseCombatWeapon		*Weapon_GetLast( void ) { return m_hLastWeapon.Get(); }

	virtual bool			HasUnlockableWeapons( int iUnlockedableIndex ) { return false; }
	bool					HasUnlockedWpn( int iIndex ) { return false; }
	bool					HasAnyAmmoOfType( int nAmmoIndex );

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void			UpdateClientData( void );
	virtual void			UpdateBattery( void );
	virtual void			RumbleEffect( unsigned char index, unsigned char rumbleData, unsigned char rumbleFlags );
	
	// Player is moved across the transition by other means
	virtual int				ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void			Precache( void );
	bool					IsOnLadder( void );
	virtual void			ExitLadder() {}
	virtual surfacedata_t	*GetLadderSurface( const Vector &origin );

	#define PLAY_FLASHLIGHT_SOUND true
	virtual void			SetFlashlightEnabled( bool bState ) { };
	virtual int				FlashlightIsOn( void ) { return false; }
	virtual bool			FlashlightTurnOn( bool playSound = false ) { return false; };		// Skip sounds when not necessary
	virtual void			FlashlightTurnOff( bool playSound = false ) { };	// Skip sounds when not necessary
	virtual bool			IsIlluminatedByFlashlight( CBaseEntity *pEntity, float *flReturnDot ) {return false; }
	
	virtual void			UpdatePlayerSound ( void );
	virtual void			UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual void			PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void			GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void			SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual void			DeathSound( const CTakeDamageInfo &info );
	const Vector &			GetMovementCollisionNormal( void ) const;	// return the normal of the surface we last collided with
	const Vector &			GetGroundNormal( void ) const;

	// return the entity used for soundscape radius checks
	virtual CBaseEntity		*GetSoundscapeListener();

	Class_T					Classify ( void );
	virtual void			SetAnimation( PLAYER_ANIM playerAnim );
	virtual void			OnMainActivityComplete( Activity newActivity, Activity oldActivity ) {}
	virtual void			OnMainActivityInterrupted( Activity newActivity, Activity oldActivity ) {}
	void					SetWeaponAnimType( const char *szExtention );

	// custom player functions
	virtual void			ImpulseCommands( void );
	virtual void			CheatImpulseCommands( int iImpulse );
	virtual bool			ClientCommand( const CCommand &args );

	void					NotifySinglePlayerGameEnding() { m_bSinglePlayerGameEnding = true; }
	bool					IsSinglePlayerGameEnding() { return m_bSinglePlayerGameEnding == true; }
	
	// Observer functions
	virtual bool			StartObserverMode(int mode); // true, if successful
	virtual void			StopObserverMode( void );	// stop spectator mode
	virtual bool			ModeWantsSpectatorGUI( int iMode ) { return true; }
	virtual bool			SetObserverMode(int mode); // sets new observer mode, returns true if successful
	virtual int				GetObserverMode( void ); // returns observer mode or OBS_NONE
	virtual bool			SetObserverTarget(CBaseEntity * target);
	virtual void			ObserverUse( bool bIsPressed ); // observer pressed use
	virtual CBaseEntity		*GetObserverTarget( void ); // returns players targer or NULL
	virtual CBaseEntity		*FindNextObserverTarget( bool bReverse ); // returns next/prev player to follow or NULL
	virtual int				GetNextObserverSearchStartPoint( bool bReverse ); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual bool			PassesObserverFilter( const CBaseEntity *entity );	// returns true if the entity passes the specified filter
	virtual bool			IsValidObserverTarget(CBaseEntity * target); // true, if player is allowed to see this target
	virtual void			CheckObserverSettings(); // checks, if target still valid (didn't die etc)
	virtual void			JumptoPosition(const Vector &origin, const QAngle &angles);
	virtual void			ForceObserverMode(int mode); // sets a temporary mode, force because of invalid targets
	virtual void			ResetObserverMode(); // resets all observer related settings
	virtual void			ValidateCurrentObserverTarget( void ); // Checks the current observer target, and moves on if it's not valid anymore
	virtual void			AttemptToExitFreezeCam( void );

	virtual bool			StartReplayMode( float fDelay, float fDuration, int iEntity );
	virtual void			StopReplayMode();
	virtual int				GetDelayTicks();
	virtual int				GetReplayEntity();

	CLogicPlayerProxy		*GetPlayerProxy( void );
	void					FirePlayerProxyOutput( const char *pszOutputName, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller );

	virtual void			CreateCorpse( void ) { }
	virtual CBaseEntity		*EntSelectSpawnPoint( void );

	// Vehicles
	virtual bool			IsInAVehicle( void ) const;
			bool			CanEnterVehicle( IServerVehicle *pVehicle, int nRole );
	virtual bool			GetInVehicle( IServerVehicle *pVehicle, int nRole );
	virtual void			LeaveVehicle( const Vector &vecExitPoint = vec3_origin, const QAngle &vecExitAngles = vec3_angle );
	int						GetVehicleAnalogControlBias() { return m_iVehicleAnalogBias; }
	void					SetVehicleAnalogControlBias( int bias ) { m_iVehicleAnalogBias = bias; }
	
	// override these for 
	virtual void			OnVehicleStart() {}
	virtual void			OnVehicleEnd( Vector &playerDestPosition ) {} 
	IServerVehicle			*GetVehicle();
	CBaseEntity				*GetVehicleEntity( void );
	bool					UsingStandardWeaponsInVehicle( void );
	
	void					AddPoints( int score, bool bAllowNegativeScore );
	void					AddPointsToTeam( int score, bool bAllowNegativeScore );
	virtual bool			BumpWeapon( CBaseCombatWeapon *pWeapon );
	bool					RemovePlayerItem( CBaseCombatWeapon *pItem );
	CBaseEntity				*HasNamedPlayerItem( const char *pszItemName );
	bool 					HasWeapons( void );// do I have ANY weapons?
	virtual void			SelectLastItem(void);
	virtual void 			SelectItem( const char *pstr, int iSubType = 0 );
	void					ItemPreFrame( void );
	virtual void			ItemPostFrame( void );
	virtual CBaseEntity		*GiveNamedItem( const char *szName, int iSubType = 0, bool removeIfNotCarried = true );
	void					EnableControl(bool fControl);
	virtual void			CheckTrainUpdate( void );
	void					AbortReload( void );

	void					SendAmmoUpdate(void);

	void					WaterMove( void );
	float					GetWaterJumpTime() const;
	void					SetWaterJumpTime( float flWaterJumpTime );
	float					GetSwimSoundTime( void ) const;
	void					SetSwimSoundTime( float flSwimSoundTime );

	virtual void			SetPlayerUnderwater( bool state );
	void					UpdateUnderwaterState( void );
	bool					IsPlayerUnderwater( void ) { return m_bPlayerUnderwater; }

	virtual bool			CanBreatheUnderwater() const { return false; }
	virtual bool			CanRecoverCurrentDrowningDamage( void ) const { return true; }// Are we allowed to later recover the drowning damage we are taking right now?  (Not, can we right now recover drowning damage.)
	virtual void			PlayerUse( void );
	virtual void			PlayUseDenySound() {}

	virtual CBaseEntity		*FindUseEntity( void );
	virtual bool			IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps );
	bool					ClearUseEntity();
	CBaseEntity				*DoubleCheckUseNPC( CBaseEntity *pNPC, const Vector &vecSrc, const Vector &vecDir );


	// physics interactions
	// mass/size limit set to zero for none
	static bool				CanPickupObject( CBaseEntity *pObject, float massLimit, float sizeLimit );
	virtual void			PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize = true ) {}
	virtual void			ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldindThis = NULL ) {}
	virtual float			GetHeldObjectMass( IPhysicsObject *pHeldObject );

	void					CheckSuitUpdate();
	void					SetSuitUpdate(char *name, int fgroup, int iNoRepeat);
	virtual void			UpdateGeigerCounter( void );
	void					CheckTimeBasedDamage( void );

	void					ResetAutoaim( void );
	
	virtual Vector			GetAutoaimVector( float flScale );
	virtual Vector			GetAutoaimVector( float flScale, float flMaxDist );
	virtual Vector			GetAutoaimVector( float flScale, float flMaxDist, float flMaxDeflection, AimResults *pAimResults );
	virtual void			GetAutoaimVector( autoaim_params_t &params );

	virtual bool			ShouldAutoaim( void );
	void					SetTargetInfo( Vector &vecSrc, float flDist );

	void					SetViewEntity( CBaseEntity *pEntity );
	CBaseEntity				*GetViewEntity( void ) { return m_hViewEntity; }

	virtual void			ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void					DeathMessage( CBaseEntity *pKiller );

	virtual void			ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds,
								int dropped_packets, bool paused );
	bool					HasQueuedUsercmds( void ) const;

	void					AvoidPhysicsProps( CUserCmd *pCmd );

	// Run a user command. The default implementation calls ::PlayerRunCommand. In TF, this controls a vehicle if
	// the player is in one.
	virtual void			PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper);
	void					RunNullCommand();

	// Team Handling
	virtual void			ChangeTeam( int iTeamNum ) { ChangeTeam(iTeamNum,false, false); }
	virtual void			ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent );

	// say/sayteam allowed?
	virtual bool		CanHearAndReadChatFrom( CustomCBasePlayer *pPlayer ) { return true; }
	virtual bool		CanSpeak( void ) { return true; }

	audioparams_t			&GetAudioParams() { return m_Local.m_audio; }

	virtual void 			ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set );

	const QAngle& GetPunchAngle();
	void SetPunchAngle( const QAngle &punchAngle );
	void SetPunchAngle( int axis, float value );

	virtual void DoMuzzleFlash();

	CNavArea *GetLastKnownArea( void ) const		{ return m_lastNavArea; }		// return the last nav area the player occupied - NULL if unknown
	const char *GetLastKnownPlaceName( void ) const	{ return m_szLastPlaceName; }	// return the last nav place name the player occupied

	virtual void			CheckChatText( char *p, int bufsize ) {}

	virtual void			CreateRagdollEntity( void ) { return; }

	virtual void			HandleAnimEvent( animevent_t *pEvent );

	CBaseEntity				*GetTonemapController( void ) const
	{
		return m_hTonemapController.Get();
	}

	virtual bool			ShouldAnnounceAchievement( void ){ return true; }


	bool					IsSplitScreenPartner( CustomCBasePlayer *pPlayer );
	void					SetSplitScreenPlayer( bool bSplitScreenPlayer, CustomCBasePlayer *pOwner );
	bool					IsSplitScreenPlayer() const;
	CustomCBasePlayer		*GetSplitScreenPlayerOwner();
	bool					IsSplitScreenUserOnEdict( edict_t *edict );
	int						GetSplitScreenPlayerSlot();

	void					AddSplitScreenPlayer( CustomCBasePlayer *pOther );
	void					RemoveSplitScreenPlayer( CustomCBasePlayer *pOther );
	CUtlVector< CHandle< CustomCBasePlayer > > &GetSplitScreenPlayers();

	// Returns true if team was changed
	virtual bool			EnsureSplitScreenTeam();

	virtual void			ForceChangeTeam( int iTeamNum ) { }

	void					UpdateFXVolume( void );
public:
	// Player Physics Shadow
	void					SetupVPhysicsShadow( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity, CPhysCollide *pStandModel, const char *pStandHullName, CPhysCollide *pCrouchModel, const char *pCrouchHullName );
	IPhysicsPlayerController* GetPhysicsController() { return m_pPhysicsController; }
	virtual void			VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void					VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual void			VPhysicsShadowUpdate( IPhysicsObject *pPhysics );
	virtual bool			IsFollowingPhysics( void ) { return false; }
	bool					IsRideablePhysics( IPhysicsObject *pPhysics );
	IPhysicsObject			*GetGroundVPhysics();

	virtual void			Touch( CBaseEntity *pOther );
	void					SetTouchedPhysics( bool bTouch );
	bool					TouchedPhysics( void );
	Vector					GetSmoothedVelocity( void );

	virtual void			InitVCollision( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity );
	virtual void			VPhysicsDestroyObject();
	void					SetVCollisionState( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity, int collisionState );
	void					PostThinkVPhysics( void );
	virtual void			UpdatePhysicsShadowToCurrentPosition();
	void					UpdatePhysicsShadowToPosition( const Vector &vecAbsOrigin );
	void					UpdateVPhysicsPosition( const Vector &position, const Vector &velocity, float secondsToArrival );

	// Hint system
	virtual CHintSystem		*Hints( void ) { return NULL; }
	bool					ShouldShowHints( void ) { return Hints() ? Hints()->ShouldShowHints() : false; }
	void					SetShowHints( bool bShowHints ) { if (Hints()) Hints()->SetShowHints( bShowHints ); }
	bool 					HintMessage( int hint, bool bForce = false ) { return Hints() ? Hints()->HintMessage( hint, bForce ) : false; }
	void 					HintMessage( const char *pMessage ) { if (Hints()) Hints()->HintMessage( pMessage ); }
	void					StartHintTimer( int iHintID ) { if (Hints()) Hints()->StartHintTimer( iHintID ); }
	void					StopHintTimer( int iHintID ) { if (Hints()) Hints()->StopHintTimer( iHintID ); }
	void					RemoveHintTimer( int iHintID ) { if (Hints()) Hints()->RemoveHintTimer( iHintID ); }

	// Accessor methods
	int		FragCount() const		{ return m_iFrags; }
	int		DeathCount() const		{ return m_iDeaths;}
	bool	IsConnected() const		{ return m_iConnected != PlayerDisconnected; }
	bool	IsDisconnecting() const	{ return m_iConnected == PlayerDisconnecting; }
	bool	IsSuitEquipped() const	{ return m_Local.m_bWearingSuit; }
	int		ArmorValue() const		{ return m_ArmorValue; }
	bool	HUDNeedsRestart() const { return m_fInitHUD; }
	float	MaxSpeed() const		{ return m_flMaxspeed; }
	Activity GetActivity( ) const	{ return m_Activity; }
	inline void SetActivity( Activity eActivity ) { m_Activity = eActivity; }
	bool	IsPlayerLockedInPlace() const { return m_iPlayerLocked != 0; }
	bool	IsObserver() const		{ return (m_afPhysicsFlags & PFLAG_OBSERVER) != 0; }
	bool	IsOnTarget() const		{ return m_fOnTarget; }
	float	MuzzleFlashTime() const { return m_flFlashTime; }
	float	PlayerDrownTime() const	{ return m_AirFinished; }

	void	SetPlayerLocked( int nLock )					{ m_iPlayerLocked = nLock; }
	int		GetPlayerLocked( void )							{ return m_iPlayerLocked; }

	int		GetObserverMode() const	{ return m_iObserverMode; }
	CBaseEntity *GetObserverTarget() const	{ return m_hObserverTarget; }

	// Round gamerules
	virtual bool	IsReadyToPlay( void ) { return true; }
	virtual bool	IsReadyToSpawn( void ) { return true; }
	virtual bool	ShouldGainInstantSpawn( void ) { return false; }
	virtual void	ResetPerRoundStats( void ) { return; }
	void			AllowInstantSpawn( void ) { m_bAllowInstantSpawn = true; }

	virtual void	ResetScores( void ) { ResetFragCount(); ResetDeathCount(); }
	void	ResetFragCount();
	void	IncrementFragCount( int nCount );

	void	ResetDeathCount();
	void	IncrementDeathCount( int nCount );

	void	SetArmorValue( int value );
	void	IncrementArmorValue( int nCount, int nMaxValue = -1 );

	void	SetConnected( PlayerConnectedState iConnected ) { m_iConnected = iConnected; }
	virtual void EquipSuit( bool bPlayEffects = true );
	virtual void RemoveSuit( void );
	void	SetMaxSpeed( float flMaxSpeed ) { m_flMaxspeed = flMaxSpeed; }

	void	NotifyNearbyRadiationSource( float flRange );

	void	SetAnimationExtension( const char *pExtension );

	void	SetAdditionalPVSOrigin( const Vector &vecOrigin );
	void	SetCameraPVSOrigin( const Vector &vecOrigin );
	void	SetMuzzleFlashTime( float flTime );
	void	SetUseEntity( CBaseEntity *pUseEntity );
	virtual CBaseEntity* GetUseEntity( void );
	virtual CBaseEntity* GetPotentialUseEntity( void );

	// Used to set private physics flags PFLAG_*
	void	SetPhysicsFlag( int nFlag, bool bSet );

	void	AllowImmediateDecalPainting();

	// Suicide...
	virtual void CommitSuicide( bool bExplode = false, bool bForce = false );
	virtual void CommitSuicide( const Vector &vecForce, bool bExplode = false, bool bForce = false );

	// For debugging...
	void	ForceOrigin( const Vector &vecOrigin );

	// Bot accessors...
	void	SetTimeBase( float flTimeBase );
	float	GetTimeBase() const;
	void	SetLastUserCommand( const CUserCmd &cmd );
	const CUserCmd *GetLastUserCommand( void );
	virtual bool IsBot() const;

	bool	IsPredictingWeapons( void ) const; 
	int		CurrentCommandNumber() const;
	const CUserCmd *GetCurrentUserCommand() const;

	int		GetFOV( void );														// Get the current FOV value
	int		GetDefaultFOV( void ) const;										// Default FOV if not specified otherwise
	int		GetFOVForNetworking( void );										// Get the current FOV used for network computations
	bool	SetFOV( CBaseEntity *pRequester, int FOV, float zoomRate = 0.0f, int iZoomStart = 0 );	// Alters the base FOV of the player (must have a valid requester)
	void	SetDefaultFOV( int FOV );											// Sets the base FOV if nothing else is affecting it by zooming
	CBaseEntity *GetFOVOwner( void ) { return m_hZoomOwner; }
	float	GetFOVDistanceAdjustFactor(); // shared between client and server
	float	GetFOVDistanceAdjustFactorForNetworking();

	int		GetImpulse( void ) const { return m_nImpulse; }

	// Movement constraints
	void	ActivateMovementConstraint( CBaseEntity *pEntity, const Vector &vecCenter, float flRadius, float flConstraintWidth, float flSpeedFactor, bool constraintPastRadius = false );
	void	DeactivateMovementConstraint( );

	// talk control
	void	NotePlayerTalked() { m_fLastPlayerTalkTime = gpGlobals->curtime; }
	float	LastTimePlayerTalked() { return m_fLastPlayerTalkTime; }

	void	DisableButtons( int nButtons );
	void	EnableButtons( int nButtons );
	void	ForceButtons( int nButtons );
	void	UnforceButtons( int nButtons );

	//---------------------------------
	// Inputs
	//---------------------------------
	void	InputSetHealth( inputdata_t &inputdata );
	void	InputSetHUDVisibility( inputdata_t &inputdata );

	surfacedata_t *GetSurfaceData( void ) const { return m_pSurfaceData; }
	void SetLadderNormal( Vector vecLadderNormal ) { m_vecLadderNormal = vecLadderNormal; }
	// If a derived class extends ImpulseCommands() and changes an existing impulse, it'll need to clear out the impulse.
	void ClearImpulse( void ) { m_nImpulse = 0; }

	// Here so that derived classes can use the expresser
	virtual CAI_Expresser *GetExpresser() { return NULL; };

	void				IncrementEFNoInterpParity();
	int					GetEFNoInterpParity() const;

private:
	
	// For queueing up CUserCmds and running them from PhysicsSimulate
	int					GetCommandContextCount( void ) const;
	CCommandContext		*GetCommandContext( int index );
	CCommandContext		*AllocCommandContext( void );
	void				RemoveCommandContext( int index );
	void				RemoveAllCommandContexts( void );
	CCommandContext		*RemoveAllCommandContextsExceptNewest( void );
	void				ReplaceContextCommands( CCommandContext *ctx, CUserCmd *pCommands, int nCommands );

	int					DetermineSimulationTicks( void );
	void				AdjustPlayerTimeBase( int simulation_ticks );

public:
	
	// Used by gamemovement to check if the entity is stuck.
	int m_StuckLast;
	
	// FIXME: Make these protected or private!

	// This player's data that should only be replicated to 
	//  the player and not to other players.
	CNetworkVarEmbedded( CPlayerLocalData, m_Local );

	CNetworkVarEmbedded( fogplayerparams_t, m_PlayerFog );
	void InitFogController( void );
	void InputSetFogController( inputdata_t &inputdata );

	void OnTonemapTriggerStartTouch( CTonemapTrigger *pTonemapTrigger );
	void OnTonemapTriggerEndTouch( CTonemapTrigger *pTonemapTrigger );
	CUtlVector< CHandle< CTonemapTrigger > > m_hTriggerTonemapList;

	CNetworkHandle( CPostProcessController, m_hPostProcessCtrl );	// active postprocessing controller
	CNetworkHandle( CColorCorrection, m_hColorCorrectionCtrl );		// active FXVolume color correction
	void InitPostProcessController( void );
	void InputSetPostProcessController( inputdata_t &inputdata );
	void InitColorCorrectionController( void );
	void InputSetColorCorrectionController( inputdata_t &inputdata );

	// Used by env_soundscape_triggerable to manage when the player is touching multiple
	// soundscape triggers simultaneously.
	// The one at the HEAD of the list is always the current soundscape for the player.
	CUtlVector<EHANDLE> m_hTriggerSoundscapeList;

	// Player data that's sometimes needed by the engine
	CNetworkVarEmbedded( CPlayerState, pl );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_fFlags );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecViewOffset );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_flFriction );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iAmmo );
	
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_hGroundEntity );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_lifeState );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecBaseVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nNextThinkTick );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nWaterLevel );
	
	int						m_nButtons;
	int						m_afButtonPressed;
	int						m_afButtonReleased;
	int						m_afButtonLast;
	int						m_afButtonDisabled;	// A mask of input flags that are cleared automatically
	int						m_afButtonForced;	// These are forced onto the player's inputs

	CNetworkVar( bool, m_fOnTarget );		//Is the crosshair on a target?

	char					m_szAnimExtension[32];

	int						m_nUpdateRate;		// user snapshot rate cl_updaterate
	float					m_fLerpTime;		// users cl_interp
	bool					m_bLagCompensation;	// user wants lag compenstation
	bool					m_bPredictWeapons; //  user has client side predicted weapons
	
	float		GetDeathTime( void ) { return m_flDeathTime; }

	void		ClearZoomOwner( void );

	void		SetPreviouslyPredictedOrigin( const Vector &vecAbsOrigin );
	const Vector &GetPreviouslyPredictedOrigin() const;
	float		GetFOVTime( void ){ return m_flFOVTime; }

	void		AdjustDrownDmg( int nAmount );

private:

	Activity				m_Activity;

protected:

	virtual void			CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void					CalcVehicleView( IServerVehicle *pVehicle, Vector& eyeOrigin, QAngle& eyeAngles, 	
								float& zNear, float& zFar, float& fov );
	void					CalcObserverView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void					CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles);

	// FIXME: Make these private! (tf_player uses them)

	// Secondary point to derive PVS from when zoomed in with binoculars/sniper rifle.  The PVS is 
	//  a merge of the standing origin and this additional origin
	Vector					m_vecAdditionalPVSOrigin; 
	// Extra PVS origin if we are using a camera object
	Vector					m_vecCameraPVSOrigin;

	CNetworkHandle( CBaseEntity, m_hUseEntity );			// the player is currently controlling this entity because of +USE latched, NULL if no entity

	int						m_iTrain;				// Train control position

	float					m_iRespawnFrames;	// used in PlayerDeathThink() to make sure players can always respawn
 	unsigned int			m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	
	// Vehicles
	CNetworkHandle( CBaseEntity, m_hVehicle );

	int						m_iVehicleAnalogBias;

	void					UpdateButtonState( int nUserCmdButtonMask );

	bool	m_bPauseBonusProgress;
	CNetworkVar( int, m_iBonusProgress );
	CNetworkVar( int, m_iBonusChallenge );

	int						m_lastDamageAmount;		// Last damage taken
	float					m_fTimeLastHurt;

	Vector					m_DmgOrigin;
	float					m_DmgTake;
	float					m_DmgSave;
	int						m_bitsDamageType;	// what types of damage has player taken
	int						m_bitsHUDDamage;	// Damage bits for the current fame. These get sent to the hud via gmsgDamage

	CNetworkVar( float, m_flDeathTime );		// the time at which the player died  (used in PlayerDeathThink())
	float					m_flDeathAnimTime;	// the time at which the player finished their death anim (used in PlayerDeathThink() and ShouldTransmit())

	CNetworkVar( int, m_iObserverMode );	// if in spectator mode != 0
	CNetworkVar( int,	m_iFOV );			// field of view
	CNetworkVar( int,	m_iDefaultFOV );	// default field of view
	CNetworkVar( int,	m_iFOVStart );		// What our FOV started at
	CNetworkVar( float,	m_flFOVTime );		// Time our FOV change started
	
	int						m_iObserverLastMode; // last used observer mode
	CNetworkHandle( CBaseEntity, m_hObserverTarget );	// entity handle to m_iObserverTarget
	bool					m_bForcedObserverMode; // true, player was forced by invalid targets to switch mode
	
	CNetworkHandle( CBaseEntity, m_hZoomOwner );	//This is a pointer to the entity currently controlling the player's zoom
													//Only this entity can change the zoom state once it has ownership

	float					m_tbdPrev;				// Time-based damage timer
	int						m_idrowndmg;			// track drowning damage taken
	int						m_idrownrestored;		// track drowning damage restored
	int						m_nPoisonDmg;			// track recoverable poison damage taken
	int						m_nPoisonRestored;		// track poison damage restored
	// NOTE: bits damage type appears to only be used for time-based damage
	BYTE					m_rgbTimeBasedDamage[CDMG_TIMEBASED];

	// Player Physics Shadow
	int						m_vphysicsCollisionState;

	virtual int SpawnArmorValue( void ) const { return 0; }

	float					m_fNextSuicideTime; // the time after which the player can next use the suicide command
	int						m_iSuicideCustomKillFlags;

	// Replay mode	
	float					m_fDelay;			// replay delay in seconds
	float					m_fReplayEnd;		// time to stop replay mode
	int						m_iReplayEntity;	// follow this entity in replay

	virtual void UpdateTonemapController( void );
	CNetworkHandle( CBaseEntity, m_hTonemapController );

private:
	void HandleFuncTrain();

// DATA
private:
	CUtlVector< CCommandContext > m_CommandContext;
	// Player Physics Shadow

protected: //used to be private, but need access for portal mod (Dave Kircher)
	IPhysicsPlayerController	*m_pPhysicsController;
	IPhysicsObject				*m_pShadowStand;
	IPhysicsObject				*m_pShadowCrouch;
	Vector						m_oldOrigin;
	Vector						m_vecSmoothedVelocity;
	bool						m_touchedPhysObject;
	bool						m_bPhysicsWasFrozen;

private:

	int						m_iPlayerSound;// the index of the sound list slot reserved for this player
	int						m_iTargetVolume;// ideal sound volume. 
	
	int						m_rgItems[MAX_ITEMS];

	// these are time-sensitive things that we keep track of
	float					m_flSwimTime;		// how long player has been underwater
	float					m_flDuckTime;		// how long we've been ducking
	float					m_flDuckJumpTime;	

	float					m_flSuitUpdate;					// when to play next suit update
	int						m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int						m_iSuitPlayNext;				// next sentence slot for queue storage;
	int						m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float					m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat

	float					m_flgeigerRange;		// range to nearest radiation source
	float					m_flgeigerDelay;		// delay per update of range msg to client
	int						m_igeigerRangePrev;

	bool					m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	bool					m_fGameHUDInitialized;
	bool					m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	int						m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int						m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new

	// Autoaim data
	QAngle					m_vecAutoAim;

	int						m_iFrags;
	int						m_iDeaths;

	float					m_flNextDecalTime;// next time this player can spray a decal

	// Team Handling
	// char					m_szTeamName[TEAM_NAME_LENGTH];

	// Multiplayer handling
	PlayerConnectedState	m_iConnected;

	// from edict_t
	// CustomCBasePlayer doesn't send this but CCSPlayer does.
	CNetworkVarForDerived( int, m_ArmorValue );
	float					m_AirFinished;
	float					m_PainFinished;

	// player locking
	int						m_iPlayerLocked;

	CSimpleSimTimer			m_AutoaimTimer;

protected:
	// the player's personal view model
	typedef CHandle<CBaseViewModel> CBaseViewModelHandle;
	CNetworkArray( CBaseViewModelHandle, m_hViewModel, MAX_VIEWMODELS );

	// Last received usercmd (in case we drop a lot of packets )
	CUserCmd				m_LastCmd;
	CUserCmd				*m_pCurrentCommand;

	float					m_flStepSoundTime;	// time to check for next footstep sound

	bool					m_bAllowInstantSpawn;

private:

// Replicated to all clients
	CNetworkVar( float, m_flMaxspeed );
	CNetworkVar( int, m_ladderSurfaceProps );
	CNetworkVector( m_vecLadderNormal );	// Clients may need this for climbing anims
	
protected:
// Not transmitted
	float					m_flWaterJumpTime;  // used to be called teleport_time
	Vector					m_vecWaterJumpVel;
	int						m_nImpulse;
	float					m_flSwimSoundTime;
	
	
private:
	float					m_flFlashTime;
	int						m_nDrownDmgRate;		// Drowning damage in points per second without air.

	int						m_nNumCrouches;			// Number of times we've crouched (for hinting)
	bool					m_bDuckToggled;		// If true, the player is crouching via a toggle

public:
	bool					GetToggledDuckState( void ) { return m_bDuckToggled; }
	void					ToggleDuck( void );
	float					GetStickDist( void );

	float					m_flForwardMove;
	float					m_flSideMove;
	int						m_nNumCrateHudHints;

private:

	// Used in test code to teleport the player to random locations in the map.
	Vector					m_vForcedOrigin;
	bool					m_bForceOrigin;	

	// Clients try to run on their own realtime clock, this is this client's clock
	CNetworkVar( int, m_nTickBase );

	bool					m_bGamePaused;
	float					m_fLastPlayerTalkTime;
	
	CNetworkVar( CBaseCombatWeaponHandle, m_hLastWeapon );

#if !defined( NO_ENTITY_PREDICTION )
	CUtlVector< CHandle< CBaseEntity > > m_SimulatedByThisPlayer;
#endif

	float					m_flOldPlayerZ;
	float					m_flOldPlayerViewOffsetZ;

	bool					m_bPlayerUnderwater;

	CNetworkHandle( CBaseEntity, m_hViewEntity );

	// Movement constraints
	CNetworkHandle( CBaseEntity, m_hConstraintEntity );
	CNetworkVector( m_vecConstraintCenter );
	CNetworkVar( float, m_flConstraintRadius );
	CNetworkVar( float, m_flConstraintWidth );
	CNetworkVar( float, m_flConstraintSpeedFactor );
	CNetworkVar( bool, m_bConstraintPastRadius );

	friend class CPlayerMove;
	friend class CPlayerClass;
	friend class CASW_PlayerMove;
	friend class CPaintPlayerMove;

	// Player name
	char					m_szNetname[MAX_PLAYER_NAME_LENGTH];

protected:
	// HACK FOR TF2 Prediction
	friend class CTFGameMovementRecon;
	friend class TicklessAirstrafe;
	friend class CGameMovement;
	friend class CTFGameMovement;
	friend class CHL1GameMovement;
	friend class CCSGameMovement;	
	friend class CHL2GameMovement;
	friend class CPortalGameMovement;
	friend class CASW_MarineGameMovement;
	friend class CPaintGameMovement;
	
	// Accessors for gamemovement
	bool IsDucked( void ) const { return m_Local.m_bDucked; }
	bool IsDucking( void ) const { return m_Local.m_bDucking; }
	float GetStepSize( void ) const { return m_Local.m_flStepSize; }

	CNetworkVar( float,  m_flLaggedMovementValue );

	// These are generated while running usercmds, then given to UpdateVPhysicsPosition after running all queued commands.
	Vector m_vNewVPhysicsPosition;
	Vector m_vNewVPhysicsVelocity;
	
	Vector	m_vecVehicleViewOrigin;		// Used to store the calculated view of the player while riding in a vehicle
	QAngle	m_vecVehicleViewAngles;		// Vehicle angles
	float	m_flVehicleViewFOV;			// FOV of the vehicle driver
	int		m_nVehicleViewSavedFrame;	// Used to mark which frame was the last one the view was calculated for

	Vector m_vecPreviouslyPredictedOrigin; // Used to determine if non-gamemovement game code has teleported, or tweaked the player's origin
	int		m_nBodyPitchPoseParam;

	// last known navigation area of player - NULL if unknown
	CNavArea *m_lastNavArea;
	CNetworkString( m_szLastPlaceName, MAX_PLACE_NAME_LENGTH );

	char m_szNetworkIDString[MAX_NETWORKID_LENGTH];
	CPlayerInfo m_PlayerInfo;

	// Texture names and surface data, used by CGameMovement
	int				m_surfaceProps;
	surfacedata_t*	m_pSurfaceData;
	float			m_surfaceFriction;
	char			m_chTextureType;
	char			m_chPreviousTextureType;	// Separate from m_chTextureType. This is cleared if the player's not on the ground.

	bool			m_bSinglePlayerGameEnding;

	CNetworkVar( int, m_ubEFNoInterpParity );

	EHANDLE			m_hPlayerProxy;	// Handle to a player proxy entity for quicker reference

public:

	float  GetLaggedMovementValue( void ){ return m_flLaggedMovementValue;	}
	void   SetLaggedMovementValue( float flValue ) { m_flLaggedMovementValue = flValue;	}

	inline bool IsAutoKickDisabled( void ) const;
	inline void DisableAutoKick( bool disabled );

	void	DumpPerfToRecipient( CustomCBasePlayer *pRecipient, int nMaxRecords );

	// picker debug utility functions
	virtual CBaseEntity*	FindEntityClassForward( char *classname );
	virtual CBaseEntity*	FindEntityForward( bool fHull );
	virtual CBaseEntity*	FindPickerEntityClass( char *classname );
	virtual CBaseEntity*	FindPickerEntity();
	virtual CAI_Node*		FindPickerAINode( int nNodeType );
	virtual CAI_Link*		FindPickerAILink();


	void PrepareForFullUpdate( void );

	virtual void OnSpeak( CustomCBasePlayer *actor, const char *sound, float duration ) {}

	// A voice packet from this client was received by the server
	virtual void OnVoiceTransmit( void ) {}

private:

	bool m_autoKickDisabled;

	struct StepSoundCache_t
	{
		StepSoundCache_t() : m_usSoundNameIndex( 0 ) {}
		CSoundParameters	m_SoundParameters;
		unsigned short		m_usSoundNameIndex;
	};
	// One for left and one for right side of step
	StepSoundCache_t		m_StepSoundCache[ 2 ];

	CUtlLinkedList< CPlayerSimInfo >  m_vecPlayerSimInfo;
	CUtlLinkedList< CPlayerCmdInfo >  m_vecPlayerCmdInfo;

	friend class CGameMovement;
	friend class CMoveHelperServer;
	Vector m_movementCollisionNormal;
	Vector m_groundNormal;
	CHandle< CBaseCombatCharacter > m_stuckCharacter;

	// If true, the m_hSplitOwner points to who owns us
	bool			m_bSplitScreenPlayer;
	CHandle< CustomCBasePlayer > m_hSplitOwner;
	// If we have any attached split users, this is the list of them
	CUtlVector< CHandle< CustomCBasePlayer > > m_hSplitScreenPlayers;

private:
	float	GetAutoaimScore( const Vector &eyePosition, const Vector &viewDir, const Vector &vecTarget, CBaseEntity *pTarget, float fScale, CBaseCombatWeapon *pActiveWeapon );
	QAngle	AutoaimDeflection( Vector &vecSrc, autoaim_params_t &params );

public:
	virtual unsigned int PlayerSolidMask( bool brushOnly = false ) const;	// returns the solid mask for the given player, so bots can have a more-restrictive set
};