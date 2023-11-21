#ifndef SSDEMO__H
#define SSDEMO__H

// demo control functions
const char *G_SRecord( void );
const char *G_SStopRecord( void );
const char *G_SDemo( void );
const char *G_SSkipTo( int ms );
const char *G_SPause( void );
const char *G_SStopDemo( void );
const char *G_SDemoStatus( void );

// demo control wrapper functions for server commands
void Svcmd_SRecord_f( void );
void Svcmd_SStopRecord_f( void );
void Svcmd_SDemo_f( void );
void Svcmd_SSkip_f( void );
void Svcmd_SSkipTo_f( void );
void Svcmd_SPause_f( void );
void Svcmd_SStopDemo_f( void );
void Svcmd_SDemoStatus_f( void );

// functions that will be called right after a map restart
// we use cvars to get it to work since VM memory is cleared
void G_ContinueRecord( void );
void G_ContinueDemo( void );

// called at the end of G_ShutdownGame() to make sure the file
// buffers are flushed
void G_ShutdownDemosClean( void );

// helpful forward (for global chat)
void G_RecordServerCommand( int clientNum, const char *text );

// called at the end of G_RunFrame() to record players and entities
void G_RecordSDemoData( void );

// callback wrapper functions
void G_SetConfigstring( int num, const char *string ); // records
void G_SendServerCommand( int clientNum, const char *text ); // records
void G_LinkEntity( gentity_t *ent ); // keeps track of changes
void G_UnlinkEntity( gentity_t *ent ); // keeps track of changes
void G_AdjustAreaPortalState( gentity_t *ent, qboolean open ); // records

// tracks changes only - like G_LinkEntity() without the link
// you should call this when an entity changes WITHOUT linking
void G_EntityChanged( gentity_t *ent );

// plays ONE demo frame (may be called many times from G_RunDemoLoop()
void G_PlayDemoFrame( int levelTime );
// substitute for G_RunFrame() when demo is playing
void G_RunDemoLoop( int levelTime );
// used in both G_RunFrame() and G_RunDemoLoop()
void G_SetFreezeStats( void );

// called by G_RunFrame()
void G_CleanUserinfo( void );

void G_SetDemoState( qboolean playing );

// checks server and console commands to see if they're allowed
// during demo playback
qboolean G_CantExecDuringDemo( const char *cmd );

#endif
