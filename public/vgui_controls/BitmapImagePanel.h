//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef BITMAPIMAGEPANEL_H
#define BITMAPIMAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Label.h>

namespace vgui {

class CBitmapImagePanel : public vgui::Panel
{
public:
	CBitmapImagePanel( vgui::Panel *parent, char const *panelName, char const *filename = NULL );
	virtual ~CBitmapImagePanel();

	virtual void	PaintBackground();

	virtual void	setTexture( char const *filename, bool hardwareFiltered = true );

	void setImageColor( Color color ) { m_bgColor = color; }

	// Set how the image aligns itself within the panel
	virtual void SetContentAlignment(Label::Alignment alignment);

	DECLARE_PANEL_SETTINGS();

protected:
	virtual void GetSettings(KeyValues *outResourceData);
	virtual void ApplySettings(KeyValues *inResourceData);
	//virtual const char *GetDescription();
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PaintBorder();

private:
	typedef vgui::Panel BaseClass;

	virtual void ComputeImagePosition(int &x, int &y, int &w, int &h);
	Label::Alignment  m_contentAlignment;

	bool m_preserveAspectRatio;
	bool m_hardwareFiltered;

	IImage		*m_pImage;
	Color	 m_bgColor;
	CUtlString m_pszImageName;
	CUtlString m_pszColorName;
};

};

#endif // BITMAPIMAGEPANEL_H
