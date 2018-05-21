//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#include "basetypes.h"
#include "pacifier.h"
#include "dbg.h"
#include "strtools.h"

#include "winlite.h"
#include "ShObjIdl.h"
#include "comdef.h"

#include "tier0/memdbgon.h"

static ITaskbarList3* g_pTaskbar = NULL;

void InitTaskBar()
{
	if ( g_pTaskbar )
		return;
	const DWORD ver = GetVersion();
	const DWORD dwMajor = LOBYTE( LOWORD( ver ) );
	const DWORD dwMinor = HIBYTE( LOWORD( ver ) );
	if ( dwMajor > 6 || ( dwMajor == 6 && dwMinor > 0 ) )
	{
		CoInitialize( NULL );
		CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, reinterpret_cast<LPVOID*>( &g_pTaskbar ) );
	}
}

static int g_LastPacifierDrawn = -1;
static bool g_bPacifierSuppressed = false;

void StartPacifier( char const *pPrefix )
{
	Msg( "%s", pPrefix );
	g_LastPacifierDrawn = -1;
	if ( g_pTaskbar )
	{
		g_pTaskbar->SetProgressState( GetConsoleWindow(), TBPF_NORMAL );
		/*if ( pPrefix && *pPrefix )
		{
			char text[MAX_PATH];
			V_strncpy( text, pPrefix, V_strlen( pPrefix ) - 1 );
			g_pTaskbar->SetThumbnailTooltip( GetConsoleWindow(), CStrAutoEncode( text ).ToWString() );
		}*/
	}
	UpdatePacifier( 0.001f );
}

void UpdatePacifier( float flPercent )
{
	const int iCur = static_cast<int>( flPercent * 40.f );

	if ( g_bPacifierSuppressed || g_LastPacifierDrawn == iCur )
		return;

	for( int i = g_LastPacifierDrawn + 1; i <= iCur; i++ )
	{
		if ( !( i % 4 ) )
		{
			Msg("%d", i/4);
		}
		else
		{
			if( i != 40 )
			{
				Msg(".");
			}
		}
	}

	g_LastPacifierDrawn = iCur;
	if ( g_pTaskbar )
		g_pTaskbar->SetProgressValue( GetConsoleWindow(), flPercent * 100, 100 );
}

void EndPacifier( bool bCarriageReturn )
{
	UpdatePacifier( 1 );
	
	if( bCarriageReturn && !g_bPacifierSuppressed )
		Msg("\n");
	if ( g_pTaskbar )
	{
		g_pTaskbar->SetProgressState( GetConsoleWindow(), TBPF_NOPROGRESS );
		//g_pTaskbar->SetThumbnailTooltip( GetConsoleWindow(), NULL );
	}
}

void SuppressPacifier( bool bSuppress )
{
	g_bPacifierSuppressed = bSuppress;
}
