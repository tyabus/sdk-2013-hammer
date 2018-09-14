//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "stdafx.h"
#include "hammer.h"
#include "GameConfig.h"
#include "OptionProperties.h"
#include "OPTTextures.h"
#include "Options.h"
#include "tier1/strtools.h"
#include <shlobj.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>
/////////////////////////////////////////////////////////////////////////////
// COPTTextures property page

IMPLEMENT_DYNCREATE(COPTTextures, CPropertyPage)

COPTTextures::COPTTextures() : CPropertyPage(COPTTextures::IDD)
{
	//{{AFX_DATA_INIT(COPTTextures)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pMaterialConfig = NULL;
}

COPTTextures::~COPTTextures()
{
	// detach the material exclusion list box
	m_MaterialExclude.Detach();
}

void COPTTextures::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COPTTextures)
	DDX_Control(pDX, IDC_BRIGHTNESS, m_cBrightness);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COPTTextures, CPropertyPage)
	//{{AFX_MSG_MAP(COPTTextures)
	ON_WM_HSCROLL()
	ON_BN_CLICKED( ID_MATERIALEXCLUDE_ADD, OnMaterialExcludeAdd )
	ON_BN_CLICKED( ID_MATERIALEXCLUDE_REM, OnMaterialExcludeRemove )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COPTTextures message handlers

BOOL COPTTextures::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// set brightness control & values
	m_cBrightness.SetRange(1, 50); // 10 is default
	m_cBrightness.SetPos(int(Options.textures.fBrightness * 10));

	// attach the material exclusion list box
	m_MaterialExclude.Attach( GetDlgItem( ID_MATERIALEXCLUDE_LIST )->m_hWnd );

	return TRUE;
}


void COPTTextures::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if(pScrollBar == (CScrollBar*) &m_cBrightness)
		SetModified();
	
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL COPTTextures::OnApply() 
{
	Options.textures.fBrightness = (float)m_cBrightness.GetPos() / 10.0f;

	Options.PerformChanges(COptions::secTextures);

	return CPropertyPage::OnApply();
}

void GetDirectory(char *pDest, const char *pLongName)
{
	strcpy(pDest, pLongName);
	int i = strlen(pDest);
	while (pLongName[i] != '\\' && pLongName[i] != '/' && i > 0)
		i--;

	if (i <= 0)
		i = 0;
	
	pDest[i] = 0;

	return;
}

static char s_szStartFolder[MAX_PATH];
static int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
	switch ( uMsg )
	{
		case BFFM_INITIALIZED:
		{
			if ( lpData )
			{
				SendMessage( hwnd, BFFM_SETSELECTION, TRUE, ( LPARAM ) s_szStartFolder );
			}
			break;
		}

		default:
		{
		   break;
		}
	}
         
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszTitle - 
//			*pszDirectory - 
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
BOOL COPTTextures::BrowseForFolder( char *pszTitle, char *pszDirectory )
{
	USES_CONVERSION;

	static bool s_bFirst = true;
	if ( s_bFirst )
	{
		APP()->GetDirectory( DIR_MATERIALS, s_szStartFolder );
		s_bFirst = false;
	}

	LPITEMIDLIST pidlStartFolder = NULL;

	IShellFolder *pshDesktop = NULL;
	SHGetDesktopFolder( &pshDesktop );
	if ( pshDesktop )
	{
		ULONG ulEaten;
		ULONG ulAttributes;
		pshDesktop->ParseDisplayName( NULL, NULL, A2OLE( s_szStartFolder ), &ulEaten, &pidlStartFolder, &ulAttributes );
	}	
	
	char szTmp[MAX_PATH];

	BROWSEINFO bi;
	memset( &bi, 0, sizeof( bi ) );
	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = szTmp;
	bi.lpszTitle = pszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS /*| BIF_NEWDIALOGSTYLE*/;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = TRUE;

	LPITEMIDLIST idl = SHBrowseForFolder( &bi );

	if ( idl == NULL )
	{
		return FALSE;
	}

	SHGetPathFromIDList( idl, pszDirectory );

	// Start in this folder next time.	
	Q_strncpy( s_szStartFolder, pszDirectory, sizeof( s_szStartFolder ) ); 

	CoTaskMemFree( pidlStartFolder );
	CoTaskMemFree( idl );

	return TRUE;
}


//-----------------------------------------------------------------------------
// Purpose: intercept this call and update the "material" configuration pointer
//          if it is out of sync with the game configuration "parent" (on the
//          "Options Configs" page)
//  Output: returns TRUE on success, FALSE on failure
//-----------------------------------------------------------------------------
BOOL COPTTextures::OnSetActive( void )
{
	//
	// get the current game configuration from the "Options Configs" page
	//
	COptionProperties *pOptProps = ( COptionProperties* )GetParent();
	if( !pOptProps )
		return FALSE;

	CGameConfig *pConfig = pOptProps->Configs.GetCurrentConfig();
	if( !pConfig )
		return FALSE;

	// compare for a change
	if( m_pMaterialConfig != pConfig )
	{
		// update the materail config
		m_pMaterialConfig = pConfig;

		// update the all config specific material data on this page
		MaterialExcludeUpdate();

		// update the last materail config
		m_pMaterialConfig = pConfig;
	}

	return CPropertyPage::OnSetActive();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COPTTextures::MaterialExcludeUpdate( void )
{
	// remove all of the data in the current "material exclude" list box
	m_MaterialExclude.ResetContent();

	//
	// add the data from the current material config
	//
	for( int i = 0; i < m_pMaterialConfig->m_MaterialExcludeCount; i++ )
	{
		int result = m_MaterialExclude.AddString( m_pMaterialConfig->m_szMaterialExcludeDirs[i] );
		if( ( result == LB_ERR ) || ( result == LB_ERRSPACE ) )
			return;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void StripOffMaterialDirectory( const char *pszDirectoryName, char *pszName )
{
	// clear name
	pszName[0] = '\0';

	// create a lower case version of the string
	char *pLowerCase = _strlwr( _strdup( pszDirectoryName ) );
	char *pAtMat = strstr( pLowerCase, "materials" );
	if( !pAtMat )
		return;

	// move the pointer ahead 10 spaces = "materials\"
	pAtMat += 10;

	// copy the rest to the name string
	strcpy( pszName, pAtMat );

	// free duplicated string's memory
	free( pLowerCase );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COPTTextures::OnMaterialExcludeAdd( void )
{
	//
	// get the directory path to exclude
	//
	char szTmp[MAX_PATH];
	if( !BrowseForFolder( "Select Material Exclude Directory", szTmp ) )
		return;

	// strip off the material directory
	char szSubDirName[MAX_PATH];
	StripOffMaterialDirectory( szTmp, &szSubDirName[0] );
	if( szSubDirName[0] == '\0' )
		return;

	//
	// add directory to list box
	//
	int result = m_MaterialExclude.AddString( szSubDirName );
	if( ( result == LB_ERR ) || ( result == LB_ERRSPACE ) )
		return;
	
	//
	// add name of directory to the global exlusion list
	//
	int ndx = m_pMaterialConfig->m_MaterialExcludeCount;
	if( ndx >= MAX_DIRECTORY_SIZE )
		return;


	strcpy( m_pMaterialConfig->m_szMaterialExcludeDirs[ndx], szSubDirName );
	m_pMaterialConfig->m_MaterialExcludeCount++;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void COPTTextures::OnMaterialExcludeRemove( void )
{
	//
	// get the directory to remove
	//
	int ndxSel = m_MaterialExclude.GetCurSel();
	if( ndxSel == LB_ERR )
		return;

	char szTmp[MAX_PATH];
	m_MaterialExclude.GetText( ndxSel, &szTmp[0] );

	//
	// remove directory from the list box
	//
	int result = m_MaterialExclude.DeleteString( ndxSel );
	if( result == LB_ERR )
		return;

	//
	// remove the name of the directory from the global exlusion list
	//
	for( int i = 0; i < m_pMaterialConfig->m_MaterialExcludeCount; i++ )
	{
		if( !strcmp( szTmp, m_pMaterialConfig->m_szMaterialExcludeDirs[i] ) )
		{
			// remove the directory
			if( i != ( m_pMaterialConfig->m_MaterialExcludeCount - 1 ) )
			{
				strcpy( m_pMaterialConfig->m_szMaterialExcludeDirs[i],
					    m_pMaterialConfig->m_szMaterialExcludeDirs[m_pMaterialConfig->m_MaterialExcludeCount-1] );
			}

			// decrement count
			m_pMaterialConfig->m_MaterialExcludeCount--;
		}
	}
}
