//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef DMEBASEIMPORTER_H
#define DMEBASEIMPORTER_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/idatamodel.h"

class CDmeBaseImporter : public IDmLegacyUpdater
{
	typedef IDmLegacyUpdater BaseClass;

public:
	CDmeBaseImporter( char const *formatName, char const *nextFormatName );
	virtual ~CDmeBaseImporter() = default;

	const char* GetName() const override { return m_pFormatName; }
	bool IsLatestVersion() const override;

	bool Update( CDmElement **ppRoot ) override;

protected:
	virtual bool DoFixup( CDmElement *pRoot ) = 0;

	char const* m_pFormatName;
	char const* m_pNextSerializer;
};
#endif // DMEBASEIMPORTER_H