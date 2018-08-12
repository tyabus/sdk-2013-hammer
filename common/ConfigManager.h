//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"

// See filesystem_init for the vconfig registry values.


#define	TOKEN_GAMES				"Games"
#define	TOKEN_GAME_DIRECTORY	"GameDir"
#define	TOKEN_TOOLS				"Tools"

enum eSDKEpochs
{
    HL2 = 1,
    EP1 = 2,
    EP2 = 3,
    UKNOWN = 4,
    SDK2013 = 5
};

class CGameConfigManager
{
public:

    enum loadStatus_t
    {
        LOADSTATUS_NONE = 0,	// Configs were loaded with no error
        LOADSTATUS_CONVERTED,	// GameConfig.txt did not exist and was created by converting GameCfg.INI
        LOADSTATUS_CREATED,		// GameCfg.INI was not found, the system created the default configuration based on found GameInfo.txt resources
        LOADSTATUS_ERROR,		// File was not loaded and was unable to perform the above fail-safe procedures
    };

    CGameConfigManager(void);

    ~CGameConfigManager(void);

    bool	LoadConfigs(const char *baseDir = NULL);
    bool	SaveConfigs(const char *baseDir = NULL);
    bool	ResetConfigs(const char *baseDir = NULL);

    KeyValues	*GetGameBlock(void);
    KeyValues	*GetGameSubBlock(const char *keyName);
    bool		GetDefaultGameBlock(KeyValues *pIn);

    bool	IsLoaded(void) const { return m_pData != NULL; }

    bool	WasConvertedOnLoad(void) const { return m_LoadStatus == LOADSTATUS_CONVERTED; }
    bool	WasCreatedOnLoad(void) const { return m_LoadStatus == LOADSTATUS_CREATED; }

    void	SetBaseDirectory(const char *pDirectory);

    void	SetSDKEpoch(eSDKEpochs epoch) { m_eSDKEpoch = epoch; };

private:

    const char *GetBaseDirectory(void);

    bool	LoadConfigsInternal(const char *baseDir);
    void	UpdateConfigsInternal(void);
    void	VersionConfig(void);
    bool	IsConfigCurrent(void);

    bool	CreateDefaultConfig(void);

    loadStatus_t	m_LoadStatus;	// Holds various state about what occured while loading
    KeyValues		*m_pData;		// Data as read from configuration file
    char			m_szBaseDirectory[MAX_PATH];	// Default directory
    eSDKEpochs		m_eSDKEpoch;    // Holds the "working version" of the SDK for times when we need to create an older set of game configurations.
                                    // This is required now that the SDK is deploying the tools for both the latest and previous versions of the engine.
};

#endif // CONFIGMANAGER_H
