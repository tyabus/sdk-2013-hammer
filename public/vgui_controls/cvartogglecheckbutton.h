//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef CVARTOGGLECHECKBUTTON_H
#define CVARTOGGLECHECKBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui/VGUI.h"
#include "vgui_controls/CheckButton.h"
#include "tier1/utlstring.h"
#include "tier1/KeyValues.h"
namespace vgui
{
	class CvarToggleCheckButton : public CheckButton
	{
		DECLARE_CLASS_SIMPLE( CvarToggleCheckButton, CheckButton );

		CvarToggleCheckButton( Panel *parent, const char *panelName, const char *text = "", char const *cvarname = NULL, bool ignoreMissingCvar = false );
		virtual ~CvarToggleCheckButton();

		void Reset();
		void ApplyChanges();
		bool HasBeenModified();

		virtual void SetSelected( bool state );
		virtual void Paint();
		virtual void ApplySettings( KeyValues *inResourceData );
		virtual void GetSettings( KeyValues *outResources );

		DECLARE_PANEL_SETTINGS();

	private:
		// Called when the OK / Apply button is pressed.  Changed data should be written into cvar.
		MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );
		MESSAGE_FUNC( OnButtonChecked, "CheckButtonChecked" );

		ConVarRef m_cvar;
		bool m_bStartValue;
		bool m_bIgnoreMissingCvar;
	};

} // namespace vgui

#endif // CVARTOGGLECHECKBUTTON_H
