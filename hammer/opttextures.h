//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OPTTEXTURES_H
#define OPTTEXTURES_H
#pragma once

class CGameConfig;

class COPTTextures : public CPropertyPage
{
	DECLARE_DYNCREATE( COPTTextures )

public:

	//=========================================================================
	//
	// Construction/Deconstruction
	//
	COPTTextures();
	~COPTTextures();

	//=========================================================================
	//
	// Dialog Data
	//
	//{{AFX_DATA(COPTTextures)
	enum { IDD = IDD_OPTIONS_TEXTURES };
	CSliderCtrl	m_cBrightness;
	CListBox    m_MaterialExclude;
	//}}AFX_DATA

	//=========================================================================
	//
	// Overrides
	// ClassWizard generate virtual function overrides
	//
	//{{AFX_VIRTUAL(COPTTextures)
	public:
	virtual BOOL OnApply();
	BOOL OnSetActive( void );
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CGameConfig		*m_pMaterialConfig;				// copy of the current gaming config

	BOOL BrowseForFolder( char *pszTitle, char *pszDirectory );
	void MaterialExcludeUpdate( void );

	//=========================================================================
	//
	// Generated message map functions
	//
	//{{AFX_MSG(COPTTextures)
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMaterialExcludeAdd( void );
	afx_msg void OnMaterialExcludeRemove( void );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // OPTTEXTURES_H
