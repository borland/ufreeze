#ifndef MULTIVIEW__H
#define MULTIVIEW__H

void CG_InitializeWindowRequests( void );
void CG_SendPipWindowCommand( void );
void CG_RequestPipWindowUp( int clientNum );
void CG_RequestPipWindowDown( int clientNum );
void CG_RequestPipWindowMaximize( int clientNum );

qboolean CG_PipAllowed( void );
void CG_RefreshPipWindows( void );
void CG_DrawPipWindows( void );
void CG_DrawPipCrosshair( void );
void CG_PipMouseEvent(float x, float y);
qboolean CG_InsertPipWindow( int zorder, int x, int y, int w, int h, int clientNum );
qboolean CG_RemovePipWindow( int clientNum );
void CG_RemoveAllPipWindows( void );
void CG_ManagePipWindows(void);

void CG_InitializeRefEntities( void );
void CG_ResetRefEntities( void );
void CG_AddRefEntityToScene( refEntity_t *re, int clientNum, int refFlags, qboolean testPVS );
void CG_AddAllRefEntitiesFor( int clientNum, refdef_t *refdef, qboolean mainView, qboolean thirdPerson );

//void CG_ParsePlayerSnapshot( const char *base64 );
void CG_ParsePipPlayerSnapshot( const char *base64 );
void CG_ParsePipPlayerDelta( const char *base64 );
#endif
