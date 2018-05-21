//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef URLLABEL_H
#define URLLABEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Label.h>

namespace vgui
{

class URLLabel : public Label
{
	DECLARE_CLASS_SIMPLE( URLLabel, Label );

public:
	URLLabel(Panel *parent, const char *panelName, const char *text, const char *pszURL);
	URLLabel(Panel *parent, const char *panelName, const wchar_t *wszText, const char *pszURL);
    virtual ~URLLabel();

    void SetURL(const char *pszURL);

	DECLARE_PANEL_SETTINGS();

protected:
	virtual void OnMousePressed(MouseCode code);
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void GetSettings( KeyValues *outResourceData );
	virtual void ApplySchemeSettings(IScheme *pScheme);
	//virtual const char *GetDescription( void );

	const char *GetURL( void ) { return m_pszURL; }

private:
    CUtlString    m_pszURL;
	bool	m_bUnderline;
};

}

#endif // URLLABEL_H
