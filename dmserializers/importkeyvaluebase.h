//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef IMPORTKEYVALUEBASE_H
#define IMPORTKEYVALUEBASE_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/idatamodel.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;
class KeyValues;
class CDmElement;


//-----------------------------------------------------------------------------
// Serialization class for Key Values
//-----------------------------------------------------------------------------
abstract_class CImportKeyValueBase : public IDmSerializer
{
public:
	// Inherited from IDMSerializer
	bool StoresVersionInFile() const override { return false; }
	bool IsBinaryFormat() const override { return false; }
	bool Serialize( CUtlBuffer& buf, CDmElement* pRoot ) override;
	bool Unserialize( CUtlBuffer& buf, const char* pEncodingName, int nEncodingVersion,
						const char* pSourceFormatName, int nSourceFormatVersion,
						DmFileId_t fileid, DmConflictResolution_t idConflictResolution, CDmElement** ppRoot );

protected:
	// Main entry point for derived classes to implement unserialization
	virtual CDmElement* UnserializeFromKeyValues( KeyValues* pKeyValues ) = 0;

	// Returns the file name associated with the unserialization
	const char* FileName() const;

	// Creates new elements
	CDmElement* CreateDmElement( const char* pElementType, const char* pElementName, DmObjectId_t* pId );

	// Recursively resolves all attributes pointing to elements
	void RecursivelyResolveElement( CDmElement* pElement );

	// Used to add typed attributes from keyvalues
	static bool AddBoolAttribute( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, const bool* pDefault = nullptr );
	static bool AddIntAttribute( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, const int* pDefault = nullptr );
	static bool AddFloatAttribute( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, const float* pDefault = nullptr );
	static bool AddStringAttribute( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, const char* pDefault = nullptr );

	// Used to add typed attributes from keyvalues
	static bool AddBoolAttributeFlags( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, int nFlags, const bool* pDefault = nullptr );
	static bool AddIntAttributeFlags( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, int nFlags, const int* pDefault = nullptr );
	static bool AddFloatAttributeFlags( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, int nFlags, const float* pDefault = nullptr );
	static bool AddStringAttributeFlags( CDmElement* pElement, KeyValues* pKeyValue, const char* pKeyName, int nFlags, const char* pDefault = nullptr );

	// Used to output typed attributes to keyvalues
	static void PrintBoolAttribute( CDmElement* pElement, CUtlBuffer& outBuf, const char* pKeyName );
	static void PrintIntAttribute( CDmElement* pElement, CUtlBuffer& outBuf, const char* pKeyName );
	static void PrintFloatAttribute( CDmElement* pElement, CUtlBuffer& outBuf, const char* pKeyName );
	static void PrintStringAttribute( CDmElement* pElement, CUtlBuffer& outBuf, const char* pKeyName, bool bSkipEmptryStrings = false, bool bPrintValueOnly = false );

private:
	const char* m_pFileName;
};


//-----------------------------------------------------------------------------
// Returns the file name associated with the unserialization
//-----------------------------------------------------------------------------
inline const char *CImportKeyValueBase::FileName() const
{
	return m_pFileName;
}


#endif // IMPORTKEYVALUEBASE_H