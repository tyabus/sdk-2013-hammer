//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TEXTUREBAR_H
#define TEXTUREBAR_H
#ifdef _WIN32
#pragma once
#endif

#include "HammerBar.h"
#include "TextureBox.h"
#include "wndTex.h"


class IEditorTexture;


class CTextureBar : public CHammerBar
{
	public:

    CTextureBar();

		BOOL Create(CWnd *pParentWnd);

		void NotifyGraphicsChanged(void);
		void NotifyNewMaterial( IEditorTexture *pTexture );
		void SelectTexture(LPCSTR pszTextureName);

	protected:

		void UpdateTexture(void);

		IEditorTexture *m_pCurTex;
		CTextureBox m_TextureList;
		CComboBox m_TextureGroupList;
		wndTex m_TexturePic;

		afx_msg void UpdateControl(CCmdUI *);
		afx_msg void OnBrowse(void);
		afx_msg void OnChangeTextureGroup(void);
		afx_msg void OnReplace(void);
		afx_msg void OnUpdateTexname(void);
		afx_msg void OnWindowPosChanged(WINDOWPOS *pPos);
		virtual afx_msg void OnSelChangeTexture(void);

		DECLARE_MESSAGE_MAP()
};


#endif // TEXTUREBAR_H
