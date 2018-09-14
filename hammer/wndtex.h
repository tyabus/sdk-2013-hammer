//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a window for displaying a single texture within.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef WNDTEX_H
#define WNDTEX_H
#pragma once

class IEditorTexture;

class wndTex : public CStatic
{
public:

    wndTex();

	void SetTexture(IEditorTexture *pTex);

    IEditorTexture* GetTexture(void);

protected:

	IEditorTexture *m_pTexture;

	afx_msg void OnPaint();
	
	DECLARE_MESSAGE_MAP();
};

#endif // WNDTEX_H
