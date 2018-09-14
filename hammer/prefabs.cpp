//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a system for managing prefabs. There are two types
//			of prefabs implemented here: Half-Life style prefabs, and Half-Life 2
//			style prefabs.
//
//			For Half-Life, prefab libraries are stored as binary .OL files, each of
//			which contains multiple .RMF files that are the prefabs.
//
//			For Half-Life 2, prefabs are stored in a tree of folders, each folder
//			representing a library, and the each .VMF file in the folder containing
//			a single prefab.
//
//=============================================================================//

#include "stdafx.h"
#include "Prefabs.h"
#include "Prefab3D.h"
#include "hammer.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CPrefabList CPrefab::PrefabList;
CPrefabLibraryList CPrefabLibrary::PrefabLibraryList;


//-----------------------------------------------------------------------------
// Purpose: Creates a prefab library from a given path.
// Input  : szFile - 
// Output : 
//-----------------------------------------------------------------------------
CPrefabLibrary *CreatePrefabLibrary(const char *szFile)
{
	CPrefabLibrary *pLibrary = new CPrefabLibraryVMF;

	if (pLibrary->Load(szFile) == -1)
	{
		delete pLibrary;
		return(NULL);
	}

	return(pLibrary);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefab::CPrefab()
{
	static DWORD dwRunningID = 1;
	// assign running ID
	dwID = dwRunningID++;
	PrefabList.AddTail(this);

	// assign blank name/notes
	szName[0] = szNotes[0] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefab::~CPrefab()
{
	POSITION p = PrefabList.Find(this);
	if(p)
		PrefabList.RemoveAt(p);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwID - 
// Output : CPrefab *
//-----------------------------------------------------------------------------
CPrefab * CPrefab::FindID(DWORD dwID)
{
	POSITION p = PrefabList.GetHeadPosition();
	while(p)
	{
		CPrefab *pPrefab = PrefabList.GetNext(p);
		if(pPrefab->dwID == dwID)
			return pPrefab;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPrefab::FreeAllData()
{
	// free all prefab data memory
	POSITION p = PrefabList.GetHeadPosition();
	while(p)
	{
		CPrefab *pPrefab = PrefabList.GetNext(p);
		pPrefab->FreeData();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefabLibrary::CPrefabLibrary()
{
	static DWORD dwRunningID = 1;
	// assign running ID
	dwID = dwRunningID++;
	m_szName[0] = '\0';
	szNotes[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefabLibrary::~CPrefabLibrary()
{
	FreePrefabs();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPrefabLibrary::FreePrefabs()
{
	// nuke prefabs
	POSITION p = Prefabs.GetHeadPosition();
	while (p != NULL)
	{
		CPrefab *pPrefab = Prefabs.GetNext(p);
		delete pPrefab;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszFilename - 
//-----------------------------------------------------------------------------
void CPrefabLibrary::SetNameFromFilename(LPCTSTR pszFilename)
{
	const char *cp = strrchr(pszFilename, '\\');
	strcpy(m_szName, cp ? (cp + 1) : pszFilename);
	char *p = strchr(m_szName, '.');
	if (p != NULL)
	{
		p[0] = '\0';
	}
}


//-----------------------------------------------------------------------------
// Purpose: Frees all the libraries in the prefab library list.
//-----------------------------------------------------------------------------
void CPrefabLibrary::FreeAllLibraries(void)
{
	POSITION pos = PrefabLibraryList.GetHeadPosition();
	while (pos != NULL)
	{
		CPrefabLibrary *pPrefabLibrary = PrefabLibraryList.GetNext(pos);
		if (pPrefabLibrary != NULL)
		{
			delete pPrefabLibrary;
		}
	}

	PrefabLibraryList.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Load all libraries in the prefabs directory.
//-----------------------------------------------------------------------------
void CPrefabLibrary::LoadAllLibraries()
{
	char szDir[MAX_PATH];
	APP()->GetDirectory(DIR_PREFABS, szDir);

	//
	// Add one prefab library for the root prefabs folder in case they put something there.
	//
	CPrefabLibrary *pLibrary = FindOpenLibrary(szDir);
	if (pLibrary == NULL)
	{
		pLibrary = CreatePrefabLibrary(szDir);
		if (pLibrary != NULL)
		{
			PrefabLibraryList.AddTail(pLibrary);
		}
	}
	else
	{
		pLibrary->Load(szDir);
	}


    FileFindHandle_t handle;
    const char *pMatched = g_pFullFileSystem->FindFirstEx("*.*", "hammer_prefabs", &handle);

    if (handle == FILESYSTEM_INVALID_FIND_HANDLE)
        return; // No libraries

    char szFile[MAX_PATH];

	do
	{
		if ((g_pFullFileSystem->FindIsDirectory(handle)) && (pMatched[0] != '.'))
		{
            V_ComposeFileName(szDir, pMatched, szFile, MAX_PATH);

			pLibrary = FindOpenLibrary(szFile);
			if (pLibrary == NULL)
			{
				pLibrary = CreatePrefabLibrary(szFile);
				if (pLibrary != NULL)
				{
					PrefabLibraryList.AddTail(pLibrary);
				}
			}
			else
			{
				pLibrary->Load(szDir);
			}
		}
	} while ((pMatched = g_pFullFileSystem->FindNext(handle)) != nullptr);

	g_pFullFileSystem->FindClose(handle);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPrefab - 
//-----------------------------------------------------------------------------
void CPrefabLibrary::Add(CPrefab *pPrefab)
{
	if(!Prefabs.Find(pPrefab))
		Prefabs.AddTail(pPrefab);
	pPrefab->dwLibID = dwID;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPrefab - 
//-----------------------------------------------------------------------------
void CPrefabLibrary::Remove(CPrefab *pPrefab)
{
	POSITION p = Prefabs.Find(pPrefab);
	if(p)
		Prefabs.RemoveAt(p);
	if(pPrefab->dwLibID == dwID)	// make sure it doesn't reference this
		pPrefab->dwLibID = 0xffff;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &p - 
// Output : CPrefab *
//-----------------------------------------------------------------------------
CPrefab * CPrefabLibrary::EnumPrefabs(POSITION &p)
{
	if(p == ENUM_START)
		p = Prefabs.GetHeadPosition();
	if(!p)
		return NULL;
	return Prefabs.GetNext(p);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwID - 
// Output : CPrefabLibrary *
//-----------------------------------------------------------------------------
CPrefabLibrary * CPrefabLibrary::FindID(DWORD dwID)
{
	POSITION p = PrefabLibraryList.GetHeadPosition();
	while(p)
	{
		CPrefabLibrary *pPrefabLibrary = PrefabLibraryList.GetNext(p);
		if(pPrefabLibrary->dwID == dwID)
			return pPrefabLibrary;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszFilename - 
// Output : 
//-----------------------------------------------------------------------------
CPrefabLibrary *CPrefabLibrary::FindOpenLibrary(LPCTSTR pszFilename)
{
	// checks to see if a library is open under that filename
	POSITION p = ENUM_START;
	CPrefabLibrary *pLibrary = EnumLibraries(p);
	while (pLibrary != NULL)
	{
		if (pLibrary->IsFile(pszFilename))
		{
			return(pLibrary);
		}
		pLibrary = EnumLibraries(p);
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Enumerates the prefab libraries of a given type.
// Input  : p - Iterator.
//			eType - Type of library to return, LibType_None returns all
//				library types.
// Output : Returns the next library of the given type.
//-----------------------------------------------------------------------------
CPrefabLibrary *CPrefabLibrary::EnumLibraries(POSITION &p)
{
	if (p == ENUM_START)
	{
		p = PrefabLibraryList.GetHeadPosition();
	}

	return p ? PrefabLibraryList.GetNext(p) : nullptr;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefabLibraryVMF::CPrefabLibraryVMF(): m_szFolderName{'\0'}
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefabLibraryVMF::~CPrefabLibraryVMF()
{
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this prefab represents the given filename, false if not.
// Input  : szFilename - Path of a prefab library or folder.
//-----------------------------------------------------------------------------
bool CPrefabLibraryVMF::IsFile(const char *szFilename)
{
	return(strcmpi(m_szFolderName, szFilename) == 0);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszFilename - 
// Output : int
//-----------------------------------------------------------------------------
int CPrefabLibraryVMF::Load(LPCTSTR pszFilename)
{
	FreePrefabs();

    strcpy(m_szFolderName, pszFilename);
    V_StripTrailingSlash(m_szFolderName);
	SetNameFromFilename(m_szFolderName);

	// dvs: new prefab libs have no notes! who cares?

	//
	// Read the prefabs - they are stored as individual VMF files.
	//
    char fullWildcard[MAX_PATH];
    V_ComposeFileName(pszFilename, "*.vmf", fullWildcard, MAX_PATH);

    FileFindHandle_t handle;
    const char *pMatched = g_pFullFileSystem->FindFirst(fullWildcard, &handle);
    if (handle == FILESYSTEM_INVALID_FIND_HANDLE)
	{
		// No prefabs in this folder.
		return -1;
	}

	do
	{
		if (pMatched[0] != '.')
		{
			//
			// Build the full path to the prefab file.
			//
            char fullFile[MAX_PATH];
            V_ComposeFileName(pszFilename, pMatched, fullFile, MAX_PATH);

			CPrefabVMF *pPrefab = new CPrefabVMF;
			pPrefab->SetFilename(fullFile);

			Add(pPrefab);
		}
	} while ((pMatched = g_pFullFileSystem->FindNext(handle)) != nullptr);

    g_pFullFileSystem->FindClose(handle);

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Set's the library's name by renaming the folder.
// Input  : pszName - 
// Output : Returns zero on error, nonzero on success.
//-----------------------------------------------------------------------------
int CPrefabLibraryVMF::SetName(LPCTSTR pszName)
{
	// dvs: rename the folder - or maybe don't implement for VMF prefabs
	strcpy(m_szName, pszName);
	return 1;
}