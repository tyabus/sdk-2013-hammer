//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#pragma once

//
// RunCommands functions
//

enum
{
	CCChangeDir = 0x100,
	CCCopyFile,
	CCDelFile,
	CCRenameFile
};

class KeyValues;

// command:
typedef struct
{
	BOOL bEnable;				// Run this command?

	int iSpecialCmd;			// Nonzero if special command exists
	char szRun[MAX_PATH];
	char szParms[MAX_PATH];
	BOOL bLongFilenames;		// Obsolete, but kept here for file backwards compatibility
	BOOL bEnsureCheck;
	char szEnsureFn[MAX_PATH];
	BOOL bUseProcessWnd;
	BOOL bNoWait;

    void Save(KeyValues *pKv);
    void Load(KeyValues *pKv);

} CCOMMAND, *PCCOMMAND;

// list of commands:
typedef CArray<CCOMMAND, CCOMMAND&> CCommandArray;

// run a list of commands:
bool RunCommands(CCommandArray& Commands, LPCTSTR pszDocName);
void FixGameVars(char *pszSrc, char *pszDst, BOOL bUseQuotes = TRUE);
bool IsRunningCommands();
