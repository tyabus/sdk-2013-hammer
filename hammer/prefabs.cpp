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
#include <io.h>
#include <fcntl.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


BOOL CPrefab::bCacheEnabled = TRUE;
CPrefabList CPrefab::PrefabList;
CPrefabList CPrefab::MRU;
CPrefabLibraryList CPrefabLibrary::PrefabLibraryList;


static char *pLibHeader = "Worldcraft Prefab Library\r\n\x1a";
static float fLibVersion = 0.1f;


typedef struct
{
	DWORD dwOffset;
	DWORD dwSize;
	char szName[31];
	char szNotes[MAX_NOTES];
	int iType;
} PrefabHeader;


typedef struct
{
	float fVersion;
	DWORD dwDirOffset;
	DWORD dwNumEntries;
	char szNotes[MAX_NOTES];
} PrefabLibraryHeader;


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
	p = MRU.Find(this);
	if(p)
		MRU.RemoveAt(p);
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
// Input  : b - 
//-----------------------------------------------------------------------------
void CPrefab::EnableCaching(BOOL b)
{
	bCacheEnabled = b;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPrefab - 
//-----------------------------------------------------------------------------
void CPrefab::AddMRU(CPrefab *pPrefab)
{
	if(!bCacheEnabled)
		return;

	POSITION p = MRU.Find(pPrefab);
	if(p)
	{
		// remove there and add to head
		MRU.RemoveAt(p);
	}
	else if(MRU.GetCount() == 5)
	{
		// uncache tail object
		p = MRU.GetTailPosition();
		if(p)	// might not be any yet
		{
			CPrefab *pUncache = MRU.GetAt(p);
			pUncache->FreeData();
			MRU.RemoveAt(p);
		}
	}

	// add to head
	MRU.AddHead(pPrefab);
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
// Input  : pszFilename - 
// Output : CPrefab::pfiletype_t
//-----------------------------------------------------------------------------
CPrefab::pfiletype_t CPrefab::CheckFileType(LPCTSTR pszFilename)
{
	// first check extensions
	const char *p = strrchr(pszFilename, '.');
	if(p)
	{
		if(!strcmpi(p, ".rmf"))
			return pftRMF;
		else if(!strcmpi(p, ".map"))
			return pftMAP;
		else if(!strcmpi(p, ".os"))
			return pftScript;
	}

	std::fstream file(pszFilename, std::ios::in | std::ios::binary);

	// read first 16 bytes of file
	char szBuf[255];
	file.read(szBuf, 16);

	// check 1: RMF
	float f = ((float*) szBuf)[0];

	// 0.8 was version at which RMF tag was started
	if(f <= 0.7f || !strncmp(szBuf+sizeof(float), "RMF", 3))
	{
		return pftRMF;
	}

	// check 2: script
	if(!strnicmp(szBuf, "[Script", 7))
	{
		return pftScript;
	}

	// check 3: MAP
	int i = 500;
	while(i--)
	{
		file >> std::ws;
		file.getline(szBuf, 255);
		if(szBuf[0] == '{')
			return pftMAP;
		if(file.eof())
			break;
	}

	return pftUnknown;
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
// Input  : *a - 
//			*b - 
// Output : static int
//-----------------------------------------------------------------------------
static int __cdecl SortPrefabs(CPrefab *a, CPrefab *b)
{
	return(strcmpi(a->GetName(), b->GetName()));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPrefabLibrary::Sort(void)
{
	int nPrefabs = Prefabs.GetCount();
	if (nPrefabs < 2)
	{
		return;
	}

	CPrefab **TmpPrefabArray = new CPrefab *[nPrefabs];

	//
	// Make an array we can pass to qsort.
	//
	POSITION p = ENUM_START;
	CPrefab *pPrefab = EnumPrefabs(p);
	int iPrefab = 0;
	while (pPrefab != NULL)
	{
		TmpPrefabArray[iPrefab++] = pPrefab;
		pPrefab = EnumPrefabs(p);
	}

	//
	// Sort the prefabs array by name.
	//
	qsort(TmpPrefabArray, nPrefabs, sizeof(CPrefab *), (int (__cdecl *)(const void *, const void *))SortPrefabs);

	//
	// Store back in list in sorted order.
	//
	Prefabs.RemoveAll();
	for (int i = 0; i < nPrefabs; i++)
	{
		Prefabs.AddTail(TmpPrefabArray[i]);
	}

	delete[] TmpPrefabArray;
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
	char szFile[MAX_PATH];
	((CHammer *)AfxGetApp())->GetDirectory(DIR_PREFABS, szDir);

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

	strcat(szDir, "\\*.*");

	WIN32_FIND_DATA fd;
	HANDLE hnd = FindFirstFile(szDir, &fd);
	strrchr(szDir, '\\')[0] = 0;	// truncate that

	if (hnd == INVALID_HANDLE_VALUE)
	{
		return;	// no libraries
	}

	do
	{
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (fd.cFileName[0] != '.'))
		{
			sprintf(szFile, "%s\\%s", szDir, fd.cFileName);

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
	} while (FindNextFile(hnd, &fd));

	FindClose(hnd);
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
CPrefabLibrary *CPrefabLibrary::EnumLibraries(POSITION &p, LibraryType_t eType)
{
	if (p == ENUM_START)
	{
		p = PrefabLibraryList.GetHeadPosition();
	}

	while (p != NULL)
	{
		CPrefabLibrary *pLibrary = PrefabLibraryList.GetNext(p);
		if ((eType == LibType_None) || pLibrary->IsType(eType))
		{
			return(pLibrary);
		}
	}

	return(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrefabLibraryVMF::CPrefabLibraryVMF()
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

	SetNameFromFilename(pszFilename);
	strcpy(m_szFolderName, pszFilename);

	m_eType = LibType_HalfLife2;

	// dvs: new prefab libs have no notes! who cares?

	//
	// Read the prefabs - they are stored as individual VMF files.
	//
	char szDir[MAX_PATH];
	strcpy(szDir, pszFilename);
	strcat(szDir, "\\*.vmf");

	WIN32_FIND_DATA fd;
	HANDLE hnd = FindFirstFile(szDir, &fd);
	if (hnd == INVALID_HANDLE_VALUE)
	{
		// No prefabs in this folder.
		return(1);
	}

	*strrchr(szDir, '*') = '\0';

	do
	{
		if (fd.cFileName[0] != '.')
		{
			//
			// Build the full path to the prefab file.
			//
			char szFile[MAX_PATH];
			strcpy(szFile, szDir);
			strcat(szFile, fd.cFileName);

			CPrefabVMF *pPrefab = new CPrefabVMF;
			pPrefab->SetFilename(szFile);

			Add(pPrefab);
		}
	} while (FindNextFile(hnd, &fd));

	FindClose(hnd);

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Removes this prefab library from disk.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPrefabLibraryVMF::DeleteFile(void)
{
	// dvs: can't remove the prefab folder yet
	return(false);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszFilename - 
//			bIndexOnly - 
// Output : int
//-----------------------------------------------------------------------------
int CPrefabLibraryVMF::Save(LPCTSTR pszFilename, BOOL bIndexOnly)
{
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


