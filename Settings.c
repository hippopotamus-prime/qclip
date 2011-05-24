/****************************************************************************
** QClip
** Copyright 2006 Aaron Curtis
**
** QClip is a clipboard extender for Windows.  It monitors the standard
** Windows clipboard and pushes any data placed there into a queue.
** Users can then pop data from either end of the queue when needed  or
** select items arbitrarily from a menu.  In either case, the program
** performs a "paste" operation automatically.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this file; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
****************************************************************************/

#define _CRT_SECURE_NO_DEPRECATE
#define _WIN32_IE       0x0600
#define _WIN32_WINNT    0x0600

#include <windows.h>
#include <prsht.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include "Settings.h"
#include "QClip.h"
#include "Clipboard.h"
#include "KeySettings.h"
#include "GeneralSettings.h"
#include "FormatSettings.h"
#include "RecentFiles.h"
#include "resource.h"

#define PROFILE_FILE_NAME       _T("QClip.ini")

#define MAX_PROFILE_LENGTH      20

#define PROFILE_SECTION_KEYS    _T("Keys")
#define PROFILE_SECTION_GENERAL _T("General")
#define PROFILE_SECTION_FORMATS _T("Formats")

#define PROFILE_RECENT_FILES    _T("RecentFiles")
#define PROFILE_RECENT_BASE     _T("Recent%02d")
#define PROFILE_LOAD_PREVIOUS   _T("LoadPrevious")
#define PROFILE_PREVIEW_BITMAPS _T("PreviewBitmaps")
#define PROFILE_CMD_LIST_INDEX  _T("CmdListIndex")
#define PROFILE_QUEUE_SIZE      _T("QueueSize")
#define PROFILE_COMMON_FILE     _T("CommonFile")
#define PROFILE_LONG_DATE       _T("ShowLongDate")
#define PROFILE_SHORT_DATE      _T("ShowShortDate")
#define PROFILE_CUSTOM_DATE     _T("ShowCustomDate")
#define PROFILE_ALL_FORMATS     _T("EnableAllFormats")
#define PROFILE_DYNAMIC_QUEUE   _T("DynamicQueue")
#define PROFILE_DATE_FORMAT     _T("CustomDateFormat")

//All other defaults are 0
#define DEFAULT_RECENT_FILES    5
#define DEFAULT_DYNAMIC_QUEUE   0
#define DEFAULT_LOAD_PREVIOUS   1
#define DEFAULT_PREVIEW_BITMAPS 1
#define DEFAULT_SHORT_DATE      1
#define DEFAULT_LONG_DATE       1
#define DEFAULT_CUSTOM_DATE     0
#define DEFAULT_DATE_FORMAT     _T("dd-MMM-yy HH:mm:ss")
#define DEFAULT_QUEUE_SIZE      10
#define DEFAULT_FORMAT_FLAGS    (FORMAT_TEXT | FORMAT_BITMAP | FORMAT_FILE)

static const TCHAR default_keys[NUM_KEY_COMMANDS] =
    {'v', 'f', 'f', 0, 0, 0, 0, 'b', 'b',
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0};

static const int default_key_mods[NUM_KEY_COMMANDS] = {
    MOD_CONTROL | MOD_ALT,
    MOD_CONTROL | MOD_ALT,
    MOD_CONTROL | MOD_ALT | MOD_SHIFT,
    0, 0, 0, 0,
    MOD_CONTROL | MOD_ALT,
    MOD_CONTROL | MOD_ALT | MOD_SHIFT,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const TCHAR* profile_key_strings[NUM_KEY_COMMANDS] = {
    _T("PopupKey"),
    _T("PopFrontKey"),
    _T("PeekFrontKey"),
    _T("Peek2Key"),
    _T("Peek3Key"),
    _T("Peek4Key"),
    _T("Peek5Key"),
    _T("PopBackKey"),
    _T("PeekBackKey"),
    _T("EmptyKey"),
    _T("OpenKey"),
    _T("SaveAsKey"),
    _T("SaveKey"),
    _T("LongDateKey"),
    _T("ShortDateKey"),
    _T("CustomDateKey"),
    _T("ToggleKey"),
    _T("DiscardFrontKey"),
    _T("DiscardBackKey"),
    _T("Custom1Key"),
    _T("Custom2Key"),
    _T("Custom3Key"),
    _T("Custom4Key"),
    _T("Custom5Key"),};

static const TCHAR* profile_format_strings[NUM_FORMAT_TYPES] = {
    _T("EnableText"),
    _T("EnableBitmap"),
    _T("EnableFiles"),
    _T("EnableWave"),
    _T("EnableMeta"),
    _T("EnableOther"),
    _T("EnablePrivate"),
    _T("EnableRegistered")};

typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;

static BOOL CALLBACK
SettingsHandler(HWND hwnd, UINT message, LPARAM lParam);

static BOOL WritePrivateProfileInt(const TCHAR* section_name,
    const TCHAR* key_name, int value, const TCHAR* profile_path);


/*******************************************************************
** SettingsHandler
** ===============
** Top-level message handler for the Settings property sheet.  This
** function does very little, as most funtionality it handled
** internally by the PropertySheet function.
**
** Inputs:
**      HWND hwnd           - handle to the key settings dialog
**      UINT message        - message ID
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      BOOL                - TRUE if the dialog handled this
**                            message
*******************************************************************/
BOOL CALLBACK
SettingsHandler(HWND hwnd, UINT message, LPARAM lParam)
{
    switch(message)
    {
        case PSCB_INITIALIZED:
            gv.settings_window = hwnd;
            break;

        case PSCB_PRECREATE:
            // Remove the DS_CONTEXTHELP style from the
            // dialog box template
            if (((LPDLGTEMPLATEEX) lParam)->signature == 0xffff)
            {
                ((LPDLGTEMPLATEEX) lParam)->style &= ~DS_CONTEXTHELP;
            }
            else
            {
                ((LPDLGTEMPLATE) lParam)->style &= ~DS_CONTEXTHELP;
            }
            break;
    }

    return TRUE;
}


/*******************************************************************
** OpenSettingsDialog
** ==================
** Display the settings property sheet if it is not already open;
** otherwise brings it to the foreground.  This function will save
** the application's settings when the property sheet closes.
**
** Outputs:
**      INT_PTR     - value returned by the PropertySheet function.
*******************************************************************/
INT_PTR OpenSettingsDialog()
{
    INT_PTR result;

    if(gv.settings_window)
    {
        SetForegroundWindow(gv.settings_window);
        result = 0;
    }
    else
    {
        INITCOMMONCONTROLSEX icc;
        PROPSHEETHEADER psh;
        PROPSHEETPAGE pages[NUM_PROPERTY_PAGES];
        Settings temp_settings = gv.settings;

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_HOTKEY_CLASS | ICC_UPDOWN_CLASS |
            ICC_STANDARD_CLASSES;

        InitCommonControlsEx(&icc);
    
        //General Settings page
        pages[GENERAL_PAGE].dwSize      = sizeof(PROPSHEETPAGE);
        pages[GENERAL_PAGE].dwFlags     = 0;
        pages[GENERAL_PAGE].hInstance   = GetModuleHandle(NULL);
        pages[GENERAL_PAGE].pszTemplate = MAKEINTRESOURCE(DIALOG_GENERAL_SETTINGS);
        pages[GENERAL_PAGE].pfnDlgProc  = GeneralSettingsHandler;
        pages[GENERAL_PAGE].lParam      = (LPARAM) &temp_settings;
    
        //Key Settings page
        pages[KEY_PAGE].dwSize          = sizeof(PROPSHEETPAGE);
        pages[KEY_PAGE].dwFlags         = 0;
        pages[KEY_PAGE].hInstance       = GetModuleHandle(NULL);
        pages[KEY_PAGE].pszTemplate     = MAKEINTRESOURCE(DIALOG_KEY_SETTINGS);
        pages[KEY_PAGE].pfnDlgProc      = KeySettingsHandler;
        pages[KEY_PAGE].lParam          = (LPARAM) &temp_settings;
    
        //Format Settings page
        pages[FORMAT_PAGE].dwSize       = sizeof(PROPSHEETPAGE);
        pages[FORMAT_PAGE].dwFlags      = 0;
        pages[FORMAT_PAGE].hInstance    = GetModuleHandle(NULL);
        pages[FORMAT_PAGE].pszTemplate  = MAKEINTRESOURCE(DIALOG_FORMAT_SETTINGS);
        pages[FORMAT_PAGE].pfnDlgProc   = FormatSettingsHandler;
        pages[FORMAT_PAGE].lParam       = (LPARAM) &temp_settings;
    
        psh.dwSize          = sizeof(PROPSHEETHEADER);
        psh.dwFlags         = PSH_PROPSHEETPAGE | PSH_USECALLBACK
                                | PSH_USEICONID;
        psh.hwndParent      = gv.main_window;
        psh.hInstance       = GetModuleHandle(NULL);
        psh.pszIcon         = MAKEINTRESOURCE(ICON_QCLIP);
        psh.pszCaption      = MAKEINTRESOURCE(STRING_SETTINGS_TITLE);
        psh.nPages          = NUM_PROPERTY_PAGES;
        psh.nStartPage      = 0;
        psh.ppsp            = (LPCPROPSHEETPAGE) &pages;
        psh.pfnCallback     = SettingsHandler;

        result = PropertySheet(&psh);
        SaveSettingsToDisk();
        gv.settings_window = NULL;
    }

    return result;
}


/*******************************************************************
** LoadSettingsFromDisk
** ====================
** Loads the application settings from an .ini file in the install
** path, settings defaults where needed.
*******************************************************************/
void LoadSettingsFromDisk()
{
    TCHAR profile_path[MAX_PATH];
    TCHAR profile_key[MAX_PROFILE_LENGTH];
    TCHAR recent_value[MAX_PATH];
    int i;

    GetFileInInstallPath(PROFILE_FILE_NAME, profile_path);

    //General Section
    //===============
    gv.settings.recent_files = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_RECENT_FILES,
        DEFAULT_RECENT_FILES, profile_path);

    if((gv.settings.recent_files < 1)
    || (gv.settings.recent_files > MAX_RECENT))
    {
        gv.settings.recent_files = DEFAULT_RECENT_FILES;
    }

    InitRecentFiles();

    //Add recent files in reverse order, so
    //file 0 ends up at the top of the queue
    //(i.e. most recent).
    if(gv.recent)
    {
        for(i = gv.settings.recent_files - 1; i >= 0; --i)
        {
            _sntprintf(profile_key, MAX_PROFILE_LENGTH-3,
                PROFILE_RECENT_BASE, i);

            if(GetPrivateProfileString(PROFILE_SECTION_GENERAL,
                profile_key, _T(""), recent_value, MAX_PATH,
                profile_path))
            {
                AddRecentFile(recent_value);
            }
        }
    }

    //Auto-save and restore
    gv.settings.load_previous = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_LOAD_PREVIOUS,
        DEFAULT_LOAD_PREVIOUS, profile_path);

    //Preview bitmaps in popup menu
    gv.settings.preview_bitmaps = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_PREVIEW_BITMAPS,
        DEFAULT_PREVIEW_BITMAPS, profile_path);

    //Unlimited (dynamic) queue
    gv.settings.dynamic_queue = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_DYNAMIC_QUEUE,
        DEFAULT_DYNAMIC_QUEUE, profile_path);

    //Command list index
    gv.settings.command_list_index = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_CMD_LIST_INDEX,
        0, profile_path);

    //Queue size
    gv.settings.queue_size = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_QUEUE_SIZE,
        DEFAULT_QUEUE_SIZE , profile_path);

    if((gv.settings.queue_size < 1)
    || (gv.settings.queue_size > MAX_QUEUE_SIZE))
    {
        gv.settings.queue_size = DEFAULT_QUEUE_SIZE;
    }

    //Long date
    gv.settings.show_long_date = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_LONG_DATE,
        DEFAULT_LONG_DATE, profile_path);

    //Short date
    gv.settings.show_short_date = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_SHORT_DATE,
        DEFAULT_SHORT_DATE, profile_path);

    //Custom date
    gv.settings.show_custom_date = GetPrivateProfileInt(
        PROFILE_SECTION_GENERAL, PROFILE_CUSTOM_DATE,
        DEFAULT_CUSTOM_DATE, profile_path);

    //Custom date format string
    GetPrivateProfileString(PROFILE_SECTION_GENERAL,
        PROFILE_DATE_FORMAT, DEFAULT_DATE_FORMAT,
        gv.settings.date_format, MAX_DATE_FORMAT_LENGTH,
        profile_path);

    //Common file name
    GetPrivateProfileString(PROFILE_SECTION_GENERAL,
        PROFILE_COMMON_FILE, _T(""), gv.settings.common_file,
        MAX_PATH, profile_path);

    //Keys Section
    //============
    for(i = 0; i < NUM_KEY_COMMANDS; ++i)
    {
        _sntprintf(profile_key, MAX_PROFILE_LENGTH-4, _T("%s"),
            profile_key_strings[i]);

        if(default_keys[i])
        {
            gv.settings.command_keys[i] = GetPrivateProfileInt(
                PROFILE_SECTION_KEYS, profile_key,
                VkKeyScan(default_keys[i]), profile_path);
        }
        else
        {
            gv.settings.command_keys[i] = GetPrivateProfileInt(
                PROFILE_SECTION_KEYS, profile_key,
                0, profile_path);
        }

        _tcscat(profile_key, _T("Mods"));
        gv.settings.command_mods[i] = GetPrivateProfileInt(
            PROFILE_SECTION_KEYS, profile_key,
            default_key_mods[i], profile_path);
     }

    //Formats Section
    //===============
    gv.settings.enable_all_formats = GetPrivateProfileInt(
        PROFILE_SECTION_FORMATS, PROFILE_ALL_FORMATS,
        0, profile_path);

    gv.settings.format_flags = 0;

    for(i = 0; i < NUM_FORMAT_TYPES; ++i)
    {
        gv.settings.format_flags |= (GetPrivateProfileInt(
            PROFILE_SECTION_FORMATS, profile_format_strings[i],
            (DEFAULT_FORMAT_FLAGS >> i) & 1,
            profile_path) & 1) << i;
    }
}


/*******************************************************************
** WritePrivateProfileInt
** ======================
** Companion function to the Windows API GetPrivateProfileInt
** (Windows is stupid).  Writes an integer value to an .ini file.
**
** Inputs:
**      const TCHAR* section_name   - profile section to write to
**      const TCHAR* key_name       - profile key to write to
**      int value                   - value to write
**      const TCHAR* profile_path   - path on disk to the .ini file
**
** Outputs:
**      BOOL    - TRUE on success
*******************************************************************/
BOOL WritePrivateProfileInt(const TCHAR* section_name, const TCHAR* key_name,
int value, const TCHAR* profile_path)
{
    TCHAR profile_val[MAX_PROFILE_LENGTH+1];

    _sntprintf(profile_val, MAX_PROFILE_LENGTH, _T("%d"), value);

    return WritePrivateProfileString(section_name, key_name,
        profile_val, profile_path);
}


/*******************************************************************
** SaveSettingsToDisk
** ==================
** Saves the application settings to an .ini file in the install
** path.
*******************************************************************/
void SaveSettingsToDisk()
{
    TCHAR profile_path[MAX_PATH];
    TCHAR profile_key[MAX_PROFILE_LENGTH+1];
    TCHAR* recent_value = NULL;
    unsigned int i;

    GetFileInInstallPath(PROFILE_FILE_NAME, profile_path);

    //General Section
    //===============
    //Number of recent files
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_RECENT_FILES,
        gv.settings.recent_files, profile_path);

    //Auto-save and restore
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_LOAD_PREVIOUS,
        gv.settings.load_previous, profile_path);

    //Preview bitmaps in popup menu
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_PREVIEW_BITMAPS,
        gv.settings.preview_bitmaps, profile_path);

    //Unlimited (dynamic) queue
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_DYNAMIC_QUEUE,
        gv.settings.dynamic_queue, profile_path);

    //Command list index (for the keys page)
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_CMD_LIST_INDEX,
        gv.settings.command_list_index, profile_path);

    //Queue size
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_QUEUE_SIZE,
        gv.settings.queue_size, profile_path);

    //Long date
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_LONG_DATE,
        gv.settings.show_long_date, profile_path);

    //Short date
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_SHORT_DATE,
        gv.settings.show_short_date, profile_path);

    //Custom date
    WritePrivateProfileInt(PROFILE_SECTION_GENERAL, PROFILE_CUSTOM_DATE,
        gv.settings.show_custom_date, profile_path);

    //Custom date format string
    WritePrivateProfileString(PROFILE_SECTION_GENERAL,
        PROFILE_DATE_FORMAT, gv.settings.date_format, profile_path);

    //Common file name
    WritePrivateProfileString(PROFILE_SECTION_GENERAL,
        PROFILE_COMMON_FILE, gv.settings.common_file, profile_path);

    //Recent files
    if(gv.recent)
    {
        for(i = 0; i < gv.recent_count; ++i)
        {
            _sntprintf(profile_key, MAX_PROFILE_LENGTH-3,
                PROFILE_RECENT_BASE, i);
    
            recent_value = GetRecentFileName(i);
    
            if(recent_value)
            {
                WritePrivateProfileString(PROFILE_SECTION_GENERAL,
                    profile_key, recent_value, profile_path);
            }
        }
    }

    //Keys Section
    //============
    for(i = 0; i < NUM_KEY_COMMANDS; ++i)
    {
        _sntprintf(profile_key, MAX_PROFILE_LENGTH-4, _T("%s"),
            profile_key_strings[i]);
        WritePrivateProfileInt(PROFILE_SECTION_KEYS, profile_key,
            gv.settings.command_keys[i], profile_path);

        _tcscat(profile_key, _T("Mods"));
        WritePrivateProfileInt(PROFILE_SECTION_KEYS, profile_key,
            gv.settings.command_mods[i], profile_path);
    }

    //Formats Section
    //===============
    WritePrivateProfileInt(PROFILE_SECTION_FORMATS, PROFILE_ALL_FORMATS,
        gv.settings.enable_all_formats, profile_path);

    for(i = 0; i < NUM_FORMAT_TYPES; ++i)
    {
        WritePrivateProfileInt(PROFILE_SECTION_FORMATS,
            profile_format_strings[i],
            (gv.settings.format_flags >> i) & 1,
            profile_path);
    }
}
