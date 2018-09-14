//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "interface.h"
#include "filesystem_tools.h"
#include "KeyValues.h"
#include "ConfigManager.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define	GAME_CONFIG_FILENAME	"GameConfig.txt"
#define DEFAULT_GAME_CONFIG_FILENAME "GameConfig_default.txt"
#define TOKEN_SDK_VERSION		"SDKVersion"

// Version history:
//	0 - Initial release
//	1 - Versioning added, DoD configuration added
//	2 - Ep1 added
//	3 - Ep2, TF2, and Portal added
// 4 - ??? (probably 2009)
// 5 - SDK 2013

#define SDK_LAUNCHER_VERSION 5
// NOTE: Remember to update the default game config file if you up this!

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CGameConfigManager::CGameConfigManager(void) : m_LoadStatus(LOADSTATUS_NONE), m_pData(nullptr)
{
    // Start with default directory
    g_pFullFileSystem->GetSearchPath_safe("hammer_cfg", false, m_szBaseDirectory);
    m_eSDKEpoch = (eSDKEpochs) SDK_LAUNCHER_VERSION;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CGameConfigManager::~CGameConfigManager(void)
{
    // Release the keyvalues
    if (m_pData != nullptr)
    {
        m_pData->deleteThis();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Config loading interface
// Input  : *baseDir - base directory for our file
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameConfigManager::LoadConfigs(const char *baseDir)
{
    return LoadConfigsInternal(baseDir);
}


//-----------------------------------------------------------------------------
// Purpose: Load a game configuration file (with fail-safes)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameConfigManager::LoadConfigsInternal(const char *baseDir)
{
    // Init the config if it doesn't exist
    if (!IsLoaded())
    {
        m_pData = new KeyValues(GAME_CONFIG_FILENAME);

        if (!IsLoaded())
        {
            m_LoadStatus = LOADSTATUS_ERROR;
            return false;
        }
    }

    // Clear it out
    m_pData->Clear();

    // Build our default directory
    if (baseDir != nullptr && baseDir[0] != NULL)
    {
        SetBaseDirectory(baseDir);
    }

    if (!m_pData->LoadFromFile(g_pFullFileSystem, GAME_CONFIG_FILENAME, "hammer_cfg"))
    {
        // Attempt to re-create the config
        if (!CreateDefaultConfig())
        {
            m_LoadStatus = LOADSTATUS_ERROR;
            return false;
        }
    }

    // Check to see if the gameconfig.txt is up to date.
    UpdateConfigsInternal();

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add to the current config.
//-----------------------------------------------------------------------------
void CGameConfigManager::UpdateConfigsInternal(void)
{
    // Check to a valid gameconfig.txt file buffer.
    if (!IsLoaded())
        return;

    // Check for version first.  If the version is up to date, it is assumed to be accurate
    if (IsConfigCurrent())
        return;

    KeyValues *pGameBlock = GetGameBlock();
    if (!pGameBlock)
    {
        // If we don't have a game block, reset the config file.
        ResetConfigs();
        return;
    }

    KeyValues *pDefaultBlock = new KeyValues("DefaultConfig");
    if (pDefaultBlock)
    {
        // Compile our default configurations
        GetDefaultGameBlock(pDefaultBlock);

        // Compare our default block to our current configs
        KeyValues *pNextSubKey = pDefaultBlock->GetFirstTrueSubKey();
        while (pNextSubKey)
        {
            // If we already have the name, we don't care about it
            if (pGameBlock->FindKey(pNextSubKey->GetName()))
            {
                // Advance by one key
                pNextSubKey = pNextSubKey->GetNextTrueSubKey();
                continue;
            }

            // Copy the data through to our game block
            KeyValues *pKeyCopy = pNextSubKey->MakeCopy();
            pGameBlock->AddSubKey(pKeyCopy);

            // Advance by one key
            pNextSubKey = pNextSubKey->GetNextTrueSubKey();
        }

        // All done
        pDefaultBlock->deleteThis();
    }

    // Save the new config.
    SaveConfigs();

    // Add the new version as we have been updated.
    VersionConfig();
}

//-----------------------------------------------------------------------------
// Purpose: Update the gameconfig.txt version number.
//-----------------------------------------------------------------------------
void CGameConfigManager::VersionConfig(void)
{
    // Check to a valid gameconfig.txt file buffer.
    if (!IsLoaded())
        return;

    // Look for the a version key value pair and update it.
    KeyValues *pKeyVersion = m_pData->FindKey(TOKEN_SDK_VERSION);

    // Check if our version is the same one
    if (pKeyVersion && pKeyVersion->GetInt() >= m_eSDKEpoch)
        return;

    // Create/update version key value pair.
    m_pData->SetInt(TOKEN_SDK_VERSION, m_eSDKEpoch);

    // Save the configuration.
    SaveConfigs();
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the version of the gameconfig.txt is up to date.
//-----------------------------------------------------------------------------
bool CGameConfigManager::IsConfigCurrent(void)
{
    // Check to a valid gameconfig.txt file buffer.
    if (!IsLoaded())
        return false;

    KeyValues *pKeyValue = m_pData->FindKey(TOKEN_SDK_VERSION);
    if (!pKeyValue)
        return false;

    return pKeyValue->GetInt() >= m_eSDKEpoch;
}

//-----------------------------------------------------------------------------
// Purpose: Create the default config as the loaded data
//-----------------------------------------------------------------------------
bool CGameConfigManager::CreateDefaultConfig(void)
{
    // Start our new block
    KeyValues *configBlock = new KeyValues("Configs");
    KeyValues *gameBlock = new KeyValues("Games");

    // Get our defaults
    GetDefaultGameBlock(gameBlock);
    // Add it to our parent
    configBlock->AddSubKey(gameBlock);

    configBlock->SaveToFile(g_pFullFileSystem, GAME_CONFIG_FILENAME, "hammer_cfg");

    if (m_pData)
        m_pData->deleteThis();

    m_pData = configBlock->MakeCopy();

    if (m_pData)
        VersionConfig();

    configBlock->deleteThis();

    m_LoadStatus = LOADSTATUS_CREATED;

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Write out a game configuration file
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameConfigManager::SaveConfigs(const char *baseDir)
{
    if (!IsLoaded())
        return false;

    // Build our default directory
    if (baseDir != nullptr && baseDir[0] != NULL)
    {
        SetBaseDirectory(baseDir);
    }

    return m_pData->SaveToFile(g_pFullFileSystem, GAME_CONFIG_FILENAME, "hammer_cfg");
}

//-----------------------------------------------------------------------------
// Purpose: Find the directory our .exe is based out of
//-----------------------------------------------------------------------------
const char *CGameConfigManager::GetBaseDirectory(void)
{
    return m_szBaseDirectory;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the game configuation block
//-----------------------------------------------------------------------------
KeyValues *CGameConfigManager::GetGameBlock(void)
{
    if (!IsLoaded())
        return nullptr;

    return (m_pData->FindKey(TOKEN_GAMES));
}

//-----------------------------------------------------------------------------
// Purpose: Returns a piece of the game configuation block of the given name
// Input  : *keyName - name of the block to return
//-----------------------------------------------------------------------------
KeyValues *CGameConfigManager::GetGameSubBlock(const char *keyName)
{
    if (!IsLoaded())
        return nullptr;

    KeyValues *pGameBlock = GetGameBlock();
    if (pGameBlock == nullptr)
        return nullptr;

    // Return the data
    KeyValues *pSubBlock = pGameBlock->FindKey(keyName);

    return pSubBlock;
}

//-----------------------------------------------------------------------------
// Purpose: Deletes the current config and recreates it with default values
//-----------------------------------------------------------------------------
bool CGameConfigManager::ResetConfigs(const char *baseDir /*= NULL*/)
{
    // Build our default directory
    if (baseDir != nullptr && baseDir[0] != NULL)
    {
        SetBaseDirectory(baseDir);
    }

    // Delete the file
    g_pFullFileSystem->RemoveFile(GAME_CONFIG_FILENAME, "hammer_cfg");
    if (g_pFullFileSystem->FileExists(GAME_CONFIG_FILENAME, "hammer_cfg"))
        return false;

    // Load the file again (causes defaults to be created)
    if (!LoadConfigsInternal(baseDir))
        return false;

    // Save it out
    return SaveConfigs(baseDir);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameConfigManager::SetBaseDirectory(const char *pDirectory)
{
    // Clear it
    if (pDirectory == nullptr || pDirectory[0] == '\0')
    {
        m_szBaseDirectory[0] = '\0';
        return;
    }

    // Copy it
    Q_strncpy(m_szBaseDirectory, pDirectory, sizeof(m_szBaseDirectory));
    Q_StripTrailingSlash(m_szBaseDirectory);
}

//-----------------------------------------------------------------------------
// Purpose: Create a block of keyvalues containing our default configuration
// Output : A block of keyvalues
//-----------------------------------------------------------------------------
bool CGameConfigManager::GetDefaultGameBlock(KeyValues *pIn)
{
    return pIn->LoadFromFile(g_pFullFileSystem, DEFAULT_GAME_CONFIG_FILENAME, "hammer_cfg");
}
