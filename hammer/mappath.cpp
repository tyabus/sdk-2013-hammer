//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "stdafx.h"
#include "MapPath.h"
#include "EditPathDlg.h"
#include "MapEntity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


float GetFileVersion(void);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapPath::CMapPath(void)
{
	m_iDirection = dirOneway;
	SetName("");
	SetClass("path_corner");
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapPath::~CMapPath(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMapPathNode::CMapPathNode(void)
{
	bSelected = FALSE;
	szName[0] = 0;
}

CMapPathNode::CMapPathNode(const CMapPathNode& src)
{
	*this = src;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
// Output : CMapPathNode
//-----------------------------------------------------------------------------
CMapPathNode &CMapPathNode::operator=(const CMapPathNode &src)
{
	// we don't care.
	Q_strncpy( szName, src.szName, sizeof(szName) );
	bSelected = src.bSelected;
	kv.RemoveAll();
	for ( int i=src.kv.GetFirst(); i != src.kv.GetInvalidIndex(); i=src.kv.GetNext( i ) )
	{
		MDkeyvalue KeyValue = src.kv.GetKeyValue(i);
		kv.SetValue(KeyValue.szKey, KeyValue.szValue);
	}
	pos = src.pos;
	dwID = src.dwID;

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwID - 
//			piIndex - 
// Output : CMapPathNode *
//-----------------------------------------------------------------------------
CMapPathNode *CMapPath::NodeForID(DWORD dwID, int* piIndex)
{
	for(int iNode = 0; iNode < m_Nodes.Count(); iNode++)
	{
		if(m_Nodes[iNode].dwID == dwID)
		{
			if(piIndex)
				piIndex[0] = iNode;
			return &m_Nodes[iNode];
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : DWORD
//-----------------------------------------------------------------------------
DWORD CMapPath::GetNewNodeID(void)
{
	DWORD dwNewID = 1;
	while(true)
	{
		int iNode;
		for(iNode = 0; iNode < m_Nodes.Count(); iNode++)
		{
			if(m_Nodes[iNode].dwID == dwNewID)
				break;
		}

		if(iNode == m_Nodes.Count())
			return dwNewID;

		++dwNewID;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwAfterID - 
//			vecPos - 
// Output : 
//-----------------------------------------------------------------------------
DWORD CMapPath::AddNode(DWORD dwAfterID, const Vector &vecPos)
{
	int iPos;

	if(dwAfterID == ADD_START)
		iPos = 0;
	else if(dwAfterID == ADD_END)
		iPos = m_Nodes.Count();
	else if(!NodeForID(dwAfterID, &iPos))
		return 0;	// not found!

	CMapPathNode node;
	node.pos = vecPos;
	node.bSelected = FALSE;
	node.dwID = GetNewNodeID();

	if(iPos == m_Nodes.Count())
	{
		// add at tail
		m_Nodes.AddToTail(node);
	}
	else
	{
		m_Nodes.InsertBefore( iPos, node );
	}

	return node.dwID;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwID - 
//			*pt - 
//-----------------------------------------------------------------------------
void CMapPath::SetNodePosition(DWORD dwID, Vector& pt)
{
	int iIndex;
	NodeForID(dwID, &iIndex);

	m_Nodes[iIndex].pos = pt;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwID - 
//-----------------------------------------------------------------------------
void CMapPath::DeleteNode(DWORD dwID)
{
	int iIndex;
	if ( NodeForID(dwID, &iIndex) )
	{
		m_Nodes.Remove(iIndex);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iIndex - 
//			iName - 
//			str - 
//-----------------------------------------------------------------------------
void CMapPath::GetNodeName(int iIndex, int iName, CString& str)
{
	if(m_Nodes[iIndex].szName[0])
		str = m_Nodes[iIndex].szName;
	else
	{
		if(iName)
			str.Format("%s%02d", m_szName, iName);
		else
			str = m_szName;
	}
}

// Edit

void CMapPath::EditInfo()
{
	CEditPathDlg dlg;
	dlg.m_strName = m_szName;
	dlg.m_strClass = m_szClass;
	dlg.m_iDirection = m_iDirection;
	
	if(dlg.DoModal() != IDOK)
		return;

	SetName(dlg.m_strName);
	SetClass(dlg.m_strClass);
	m_iDirection = dlg.m_iDirection;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwNodeID - 
// Output : CMapEntity *
//-----------------------------------------------------------------------------
CMapEntity *CMapPath::CreateEntityForNode(DWORD dwNodeID)
{
	int iIndex;
	CMapPathNode *pNode = NodeForID(dwNodeID, &iIndex);
	if (pNode == NULL)
	{
		return NULL;	// no node, no entity!
	}

	CMapEntity *pEntity = new CMapEntity;

	for (int k = pNode->kv.GetFirst(); k != pNode->kv.GetInvalidIndex(); k=pNode->kv.GetNext( k ) )
	{
		pEntity->SetKeyValue(pNode->kv.GetKey(k), pNode->kv.GetValue(k));
	}
	
	// store target/targetname properties:
	CString str;
	str.Format("%s%02d", m_szName, iIndex);
	pEntity->SetKeyValue("targetname", str);

	int iNext = iIndex + 1;
	if(iNext != -1)
	{
		str.Format("%s%02d", m_szName, iNext);
		pEntity->SetKeyValue("target", str);
	}

	pEntity->SetClass(m_szClass);

	return pEntity;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dwNodeID - 
//			*pEntity - 
//-----------------------------------------------------------------------------
void CMapPath::CopyNodeFromEntity(DWORD dwNodeID, CMapEntity *pEntity)
{
	CMapPathNode *pNode = NodeForID(dwNodeID);
	if (!pNode)
	{
		return;	// no node, no copy!
	}

	pNode->kv.RemoveAll();

	//
	// Copy all the keys except target and targetname from the entity to the pathnode.
	//
	for ( int i=pEntity->GetFirstKeyValue(); i != pEntity->GetInvalidKeyValue(); i=pEntity->GetNextKeyValue( i ) )
	{
		if (!strcmp(pEntity->GetKey(i), "target") || !strcmp(pEntity->GetKey(i), "targetname"))
		{
			continue;
		}

		pNode->kv.SetValue(pEntity->GetKey(i), pEntity->GetKeyValue(i));
	}
}


/*
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pNode - 
// Output : CChunkFileResult_t
//-----------------------------------------------------------------------------

UNDONE: Nobody uses the path tool because the user interface is so poor.
		Path support has been pulled until the tool itself can be fixed or replaced.

CChunkFileResult_t CMapPathNode::LoadKeyCallback(const char *szKey, const char *szValue, CMapPathNode *pNode)
{
	if (!stricmp(szKey, "origin"))
	{
		CChunkFile::ReadKeyValueVector3(szValue, pNode->pos);
	}
	else if (!stricmp(szKey, "id"))
	{
		CChunkFile::ReadKeyValueInt(szValue, &pNode->dwID);
	}
	else if (!stricmp(szKey, "name"))
	{
		strcpy(pNode->szName, szValue);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapPathNode::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	ChunkFileResult_t  eResult = pFile->BeginChunk("node");

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueVector3("origin", node.pos);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("id", node.dwID);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValue("name", node.szName);
	}
	
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->BeginChunk("keys");
	}

	//
	// Write keyvalues.
	//
	if (eResult == ChunkFile_Ok)
	{
		iSize = kv.GetCount();
		for (int k = 0; k < iSize; k++)
		{
			MDkeyvalue &KeyValue = kv.GetKeyValue(k);
			if (eResult == ChunkFile_Ok)
			{
				eResult = pFile->WriteKeyValue(KeyValue.GetKey(), KeyValue.GetValue());
			}
		}
	}

	// End the keys chunk.
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	// End the node chunk.
	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}

	return(eResult);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pPath - 
// Output : CChunkFileResult_t
//-----------------------------------------------------------------------------
CChunkFileResult_t CMapPath::LoadKeyCallback(const char *szKey, const char *szValue, CMapPath *pPath)
{
	if (!stricmp(szKey, "name"))
	{
		pPath->SetName(szValue);
	}
	else if (!stricmp(szKey, "classname"))
	{
		pPath->SetClass(szValue);
	}
	else if (!stricmp(szKey, "direction"))
	{
		CChunkFile::ReadKeyValueInt(szValue, &pPath->m_iDirection);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//-----------------------------------------------------------------------------
void CMapPath::LoadVMF(CChunkFile *pFile)
{
	file.read((char*) &iSize, sizeof iSize);
	m_nNodes = iSize;
	m_Nodes.SetSize(m_nNodes);

	// read nodes
	for (int i = 0; i < m_nNodes; i++)
	{
		CMapPathNode &node = m_Nodes[i];

			// read keyvalues
			file.read((char*) &iSize, sizeof(iSize));
			KeyValues &kv = node.kv;
			kv.SetSize(iSize);
			for (int k = 0; k < iSize; k++)
			{
				MDkeyvalue &KeyValue = kv.GetKeyValue(k);
				KeyValue.SerializeRMF(file, FALSE);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//-----------------------------------------------------------------------------
void CMapPath::SaveVMF(CChunkFile *pFile, CSaveInfo *pSaveInfo)
{
	int iSize;

	ChunkFileResult_t eResult = pFile->BeginChunk("path");

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile-WriteKeyValue("name", m_szName);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValue("classname", m_szClass);
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->WriteKeyValueInt("direction", m_iDirection);
	}

	if (eResult == ChunkFile_Ok)
	{
		for (int i = 0; i < m_nNodes; i++)
		{
			CMapPathNode &node = m_Nodes[i];
			eResult = node.SaveVMF(pFile, pSaveInfo);
		}
	}

	if (eResult == ChunkFile_Ok)
	{
		eResult = pFile->EndChunk();
	}
}
*/
