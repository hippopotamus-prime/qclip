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

#include <windows.h>
#include <prsht.h>
#include <stdio.h>
#include <shlobj.h>
#include "QClip.h"
#include "Clipboard.h"
#include "ClipQueue.h"
#include "ClipFile.h"
#include "Settings.h"
#include "RecentFiles.h"
#include "About.h"
#include "DateTimeWrapper.h"
#include "resource.h"

#define TRAY_ID         666
#define TRAY_MESSAGE    WM_USER

#define ERROR_LENGTH    200

Globals gv;

static BOOL HandleHotKey(WPARAM wParam, LPARAM lParam);
static void SimulatePaste();
static void ShowPopupMenu();
static BOOL HandleTrayMessage(HWND hwnd, WPARAM wParam, LPARAM lParam);
static BOOL CreateTrayIcon(HWND hwnd);
static BOOL DestroyTrayIcon(HWND hwnd);
static BOOL FindShellFormats();

static int AddClipItemsToMenu(HMENU menu, ClipQueue* cq,
    BOOL use_indexes, unsigned int menu_start);
static void AddMenuSeparator(HMENU menu, unsigned int item_id);
static int AddCommonItems(HMENU menu);
static BOOL CopyDateToClipboard(DWORD format);
static BOOL CopyCustomDateToClipboard();


/*******************************************************************
** QClipHandler
** ============
** Message handler for the application's main window (which is
** invisible and doesn't really do anything).  All system tray
** messages, menu commands, and hot keys are routed through here.
**
** Inputs:
**      HWND hwnd           - handle to the main application window
**      UINT message        - message ID
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      LRESULT             - result of message processing
*******************************************************************/
LRESULT CALLBACK
QClipHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // By default, do not pass messages on to the default handler
    BOOL eat = TRUE;

    switch(message)
    {
        case WM_COMMAND:
        {
            unsigned int command = LOWORD(wParam);

            if((command >= RECENT_MENU_START)
            && (command < RECENT_MENU_START + GetRecentCount()))
            {
                OpenRecentFile(command - RECENT_MENU_START);
            }
            else
            {
                switch(command)
                {
                    case IDM_EXIT:
                        PostMessage(hwnd, WM_DESTROY, 0, 0);
                        break;

                    case IDM_SETTINGS:
                        OpenSettingsDialog();
                        break;

                    case IDM_OPEN:
                        OpenQueue();
                        break;
        
                    case IDM_SAVEAS:
                        SaveQueueAs();
                        break;

                    case IDM_SAVE:
                        SaveQueue();
                        break;

                    case IDM_ENABLE:
                        gv.enable_monitoring = !gv.enable_monitoring;
                        break;

                    case IDM_EMPTY:
                        EmptyQueueAndResize(&gv.cq);
                        break;

                    case IDM_HELP:
                    {
                        //Decided to go with online help instead of local files.
                        //The code is very similar...
                        /*
                        TCHAR readme_file[MAX_PATH];
                        TCHAR readme_path[MAX_PATH];

                        LoadString(GetModuleHandle(NULL), STRING_README,
                            readme_file, MAX_PATH);
                        GetFileInInstallPath(readme_file, readme_path);
                        ShellExecute(NULL, _T("open"), readme_path,
                            NULL, readme_path, SW_SHOW);
                        */

                        TCHAR readme_url[MAX_PATH];
                        LoadString(GetModuleHandle(NULL), STRING_README,
                            readme_url, MAX_PATH);
                        ShellExecute(NULL, _T("open"), readme_url,
                            NULL, readme_url, SW_SHOW);
                        break;
                    }

                    case IDM_ABOUT:
                        DialogBox(GetModuleHandle(NULL),
                            MAKEINTRESOURCE(DIALOG_ABOUT),
                            hwnd, AboutHandler);
                        break;
                }
            }
            break;
        }

        case WM_HOTKEY:
            HandleHotKey(wParam, lParam);
            break;

        case WM_CREATE:
            LoadSettingsFromDisk();
            FindShellFormats();

            if(gv.settings.load_previous
            && OpenQueueFromDefault())
            {
                gv.ignore = 1;
            }
            else
            {
                //If no queue is loaded at startup, we'll
                //try to grab whatever's on the clipboard
                //at startup instead - hence no ignore.
                gv.ignore = 0;
                CreateQueue(&gv.cq, gv.settings.queue_size);
            }

            InitQueue(&gv.common);
            OpenCommonItems(FALSE);

            gv.main_window = hwnd;
            gv.settings_window = NULL;
            gv.opened_file = FALSE;
            gv.enable_monitoring = TRUE;

            //this must come after CreateQueue
            gv.next_viewer = SetClipboardViewer(hwnd);

            CreateTrayIcon(hwnd);
            RegisterAllHotKeys(hwnd);
            break;

        case WM_CHANGECBCHAIN:
            if((HWND) wParam == gv.next_viewer)
            {
                gv.next_viewer = (HWND) lParam;
            }
            else if(gv.next_viewer != NULL)
            {
                SendMessage(gv.next_viewer, message, wParam, lParam);
            }
            break;

        case WM_DRAWCLIPBOARD:
            if(gv.ignore > 0)
            {
                --gv.ignore;
            }
            else if(gv.enable_monitoring)
            {
                PushFront(&gv.cq);
            }

            SendMessage(gv.next_viewer, message, wParam, lParam);
            break;

        case TRAY_MESSAGE:
            HandleTrayMessage(hwnd, wParam, lParam);
            break;

        case WM_QUERYENDSESSION:
            // Windows XP sends this message during shutdown;
            // it does NOT send WM_DESTROY.
            eat = FALSE;

        case WM_DESTROY:
            SaveSettingsToDisk();
            if(gv.settings.load_previous)
            {
                SaveQueueAsDefault();
            }
            DestroyQueue(&gv.cq);
            DestroyQueue(&gv.common);
            UnregisterAllHotKeys(hwnd);
            DestroyTrayIcon(hwnd);
            DestroyRecentFiles();
            ChangeClipboardChain(hwnd, gv.next_viewer);

            //Reminder - all MessageBox calls will be suppressed
            //after this point, so no, PostQuitMessage doesn't
            //crash.  It's something else.
            PostQuitMessage(0);
            break;

        default:
            eat = FALSE;
    }

    if(eat) return 0;
    else    return DefWindowProc(hwnd, message, wParam, lParam);
}

/*******************************************************************
** FindShellFormats
** ================
** Records the IDs of several application-defined formats known to
** be used by the Windows Explorer shell.  These are necessary for 
** some file cut & paste operations, and so should be allowed if
** CF_HDROP is enabled.
**
** Outputs:
**      BOOL                - TRUE on success
*******************************************************************/
BOOL FindShellFormats()
{
    unsigned int i;
    BOOL success = TRUE;

    //These are all of the "formats for transferring file system objects"
    //according to MSDN, plus some others.  I have generally not included
    //formats used for virtual objects or other transfers.

    //Making sure here to account for both ASCII and Unicode versions...

    gv.shell_formats[0] = RegisterClipboardFormat(CFSTR_FILECONTENTS);
    gv.shell_formats[1] = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
    gv.shell_formats[2] = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
    gv.shell_formats[3] = RegisterClipboardFormat(CFSTR_FILENAMEA);
    gv.shell_formats[4] = RegisterClipboardFormat(CFSTR_FILENAMEW);
    gv.shell_formats[5] = RegisterClipboardFormat(CFSTR_FILENAMEMAPA);
    gv.shell_formats[6] = RegisterClipboardFormat(CFSTR_FILENAMEMAPW);
    gv.shell_formats[7] = RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
    gv.shell_formats[8] = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    gv.shell_formats[9] = RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET);
    gv.shell_formats[10] = RegisterClipboardFormat(CFSTR_INDRAGLOOP);
    gv.shell_formats[11] = RegisterClipboardFormat(CFSTR_LOGICALPERFORMEDDROPEFFECT);
    gv.shell_formats[12] = RegisterClipboardFormat(CFSTR_PASTESUCCEEDED);
    gv.shell_formats[13] = RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
    gv.shell_formats[14] = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);

    for(i = 0; i < NUM_SHELL_FORMATS; ++i)
    {
        success = (success && gv.shell_formats[i]);
    }

    return success;
}


/*******************************************************************
** UnregisterAllHotKeys
** ====================
** Removes all hot keys registered by RegisterAllHotKeys.
**
** Inputs:
**      HWND hwnd   - handle to the window receiving hot key
**                    messages
*******************************************************************/
void UnregisterAllHotKeys(HWND hwnd)
{
    unsigned int i;

    for(i = 0; i < NUM_KEY_COMMANDS; ++i)
    {
        if(gv.settings.command_keys[i] != 0)
        {
            UnregisterHotKey(hwnd, COMMAND_KEY_START + i);
        }
    }
}


/*******************************************************************
** RegisterAllHotKeys
** ==================
** Registers all the hot keys defined in the settings.  This
** function does not check that said hot keys are valid.
**
** Inputs:
**      HWND hwnd   - handle to the window that will receive hot
**                    key messages
*******************************************************************/
void RegisterAllHotKeys(HWND hwnd)
{
    unsigned int i;

    for(i = 0; i < NUM_KEY_COMMANDS; ++i)
    {
        if(gv.settings.command_keys[i] != 0)
        {
            RegisterHotKey(hwnd,
                COMMAND_KEY_START + i,
                gv.settings.command_mods[i],
                gv.settings.command_keys[i]);
        }
    }
}



/*******************************************************************
** HandleHotKey
** ============
** Processes hot key messages.  This function should be called
** directly from the window message handler.
**
** Inputs:
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL HandleHotKey(WPARAM wParam, LPARAM lParam)
{
    if((wParam >= KEY_PEEK_START)
    && (wParam <= KEY_PEEK_END))
    {
        if(PeekAt(&gv.cq, wParam - KEY_PEEK_START) > 0)
        {
            SimulatePaste();
        }
    }
    else if((wParam >= KEY_COMMON_START)
    && (wParam <= KEY_COMMON_END))
    {
        if(PeekAt(&gv.common, wParam - KEY_COMMON_START) > 0)
        {
            SimulatePaste();
        }
    }
    else
    {
        switch(wParam)
        {
            case KEY_POP_FRONT:
                if(PopFront(&gv.cq))
                {
                    SimulatePaste();
                }
                break;

            case KEY_POP_BACK:
                if(PopBack(&gv.cq))
                {
                    SimulatePaste();
                }
                break;
        
            case KEY_PEEK_BACK:
                if(PeekBack(&gv.cq))
                {
                    SimulatePaste();
                }
                break;

            case KEY_DISCARD_FRONT:
                DiscardFront(&gv.cq);
                break;

            case KEY_DISCARD_BACK:
                DiscardBack(&gv.cq);
                break;

            case KEY_EMPTY:
                EmptyQueueAndResize(&gv.cq);
                break;

            case KEY_POPUP:
                ShowPopupMenu();
                break;

            case KEY_OPEN:
                OpenQueue();
                break;

            case KEY_SAVEAS:
                SaveQueueAs();
                break;

            case KEY_SAVE:
                SaveQueue();
                break;

            case KEY_TOGGLE:
                gv.enable_monitoring = !gv.enable_monitoring;
                break;

            case KEY_PASTE_LONG_DATE:
                if(CopyDateToClipboard(DATE_LONGDATE))
                {
                    SimulatePaste();
                }
                break;

            case KEY_PASTE_SHORT_DATE:
                if(CopyDateToClipboard(DATE_SHORTDATE))
                {
                    SimulatePaste();
                }
                break;

            case KEY_PASTE_CUSTOM_DATE:
                if(CopyCustomDateToClipboard())
                {
                    SimulatePaste();
                }
                break;
        }
    }

    return TRUE;
}


/*******************************************************************
** AddClipItemsToMenu
** ==================
** Adds text or bitmap descriptions of ClipItems to the end of a
** menu.  ClipItems are drawn in order from a ClipQueue.
**
** Inputs:
**      HMENU menu          - menu to append to
**      ClipQueue* cq       - queue to draw ClipItems from
**      BOOL use_indexes    - if TRUE, all text menu items added will
**                            be prefixed with an accelerator key,
**                            starting at 1.
**      unsigned int menu_start - menu ID of the first item to be
**                            added, which will be appear in
**                            WM_COMMAND messages.  Subsequent items
**                            will use menu_start +1, +2, etc.
**
** Outputs:
**      int                 - menu ID of the first item added
**                            successfully, or -1 if no items were
**                            added.
*******************************************************************/
int AddClipItemsToMenu(HMENU menu, ClipQueue* cq, BOOL use_indexes,
unsigned int menu_start)
{
    int start_id = -1;

    if(cq)
    {
        BOOL success;
        unsigned int i;
        TCHAR indexes[INDEX_CHARS_LENGTH+1];

        LoadString(GetModuleHandle(NULL), STRING_INDEX_CHARS,
            indexes, INDEX_CHARS_LENGTH+1);

        for(i = 0; i < GetQueueLength(cq); ++i)
        {
            if(use_indexes && (i < INDEX_CHARS_LENGTH))
            {
                success = AddClipItemToMenu(GetItem(cq, i), menu,
                    menu_start + i, &indexes[i]);
            }
            else
            {
                success = AddClipItemToMenu(GetItem(cq, i), menu,
                    menu_start + i, NULL);
            }

            if(success && (start_id == -1))
            {
                start_id = menu_start + i;
            }
        }
    }

    return start_id;
}


/*******************************************************************
** ShowPopupMenu
** =============
** Displays a popup menu containing all the items currently in the
** queue, and optionally common items defined by the user.
** Selecting items from the menu pastes them into whatever
** application the user has open.
*******************************************************************/
void ShowPopupMenu()
{
    HMENU menu = CreatePopupMenu();

    if(menu)
    {
        HWND foreground_window = GetForegroundWindow();
        POINT point;
        int section1, section2, section3;
        unsigned int command;

        GetCursorPos(&point);
        //GetCaretPos(&point);
        //ClientToScreen(foreground_window, &point);

        section1 = AddClipItemsToMenu(menu, &gv.cq,
            TRUE, POPUP_MENU_START);
        section2 = AddCommonItems(menu);
        section3 = AddClipItemsToMenu(menu, &gv.common,
            FALSE, COMMON_MENU_START);

        if((section3 != -1)
        && ((section1 != -1) || (section2 != -1)))
        {
            AddMenuSeparator(menu, section3);
        }
        if((section2 != -1) && (section1 != -1))
        {
            AddMenuSeparator(menu, section2);
        }

        SetForegroundWindow(gv.main_window);
        command = TrackPopupMenu(menu,
            TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
            point.x,
            point.y,
            0,
            gv.main_window,
            NULL);
        PostMessage(gv.main_window, WM_NULL, 0, 0);

        SetForegroundWindow(foreground_window);
        DestroyMenu(menu);

        //Keyboard input will not immediately be directed to the
        //new foreground window under WinXP.  We have to wait a
        //bit for it to catch up.  I don't know of any elegant
        //way to synchronize this :/

        Sleep(100);

        if((command >= POPUP_MENU_START)
        && (command < POPUP_MENU_START + GetQueueLength(&gv.cq)))
        {
            PeekAt(&gv.cq, command - POPUP_MENU_START);
            SimulatePaste();
        }
        else if((command >= COMMON_MENU_START)
        && (command < COMMON_MENU_START + GetQueueLength(&gv.common)))
        {
            PeekAt(&gv.common, command - COMMON_MENU_START);
            SimulatePaste();
        }
        else
        {
            switch(command)
            {
                case COMMON_MENU_LONG_DATE:
                    if(CopyDateToClipboard(DATE_LONGDATE))
                    {
                        SimulatePaste();
                    }
                    break;

                case COMMON_MENU_SHORT_DATE:
                    if(CopyDateToClipboard(DATE_SHORTDATE))
                    {
                        SimulatePaste();
                    }
                    break;

                case COMMON_MENU_CUSTOM_DATE:
                    if(CopyCustomDateToClipboard())
                    {
                        SimulatePaste();
                    }
                    break;
            }
        }
    }
}


/*******************************************************************
** CopyCustomDateToClipboard
** =========================
** Copies the current date (as text) to the clipboard, using the
** custom date format in the settings.
**
** Outputs:
**      BOOL            - TRUE on success.
*******************************************************************/
BOOL CopyCustomDateToClipboard()
{
    BOOL success = FALSE;
    SYSTEMTIME time;
    TCHAR* buffer;

    GetLocalTime(&time);
    buffer = AllocateFullDateTime(gv.settings.date_format, &time);

    if(buffer)
    {
        success = CopyStringToClipboard(buffer);
        HeapFree(GetProcessHeap(), 0, buffer);
    }

    return success;
}


/*******************************************************************
** CopyDateToClipboard
** ===================
** Copies the current date (as text) to the clipboard.
**
** Inputs:
**      DWORD format    - either DATE_LONGDATE or DATE_SHORTDATE
**
** Outputs:
**      BOOL            - TRUE on success.
*******************************************************************/
BOOL CopyDateToClipboard(DWORD format)
{
    SYSTEMTIME time;
    TCHAR date[POPUP_TEXT_LENGTH+1];

    GetLocalTime(&time);
    GetDateFormat(LOCALE_USER_DEFAULT, format, &time,
        NULL, date, POPUP_TEXT_LENGTH);

    return CopyStringToClipboard(date);
}


/*******************************************************************
** AddMenuSeparator
** ================
** Inserts a separator into a menu at a given position.
**
** Inputs:
**      HMENU menu              - menu to insert into
**      unsigned int item_id    - the separator will be inserted
**                                before the menu item with this ID
*******************************************************************/
void AddMenuSeparator(HMENU menu, unsigned int item_id)
{
    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize          = sizeof(MENUITEMINFO);

    mii.fMask           = MIIM_TYPE | MIIM_STATE;
    mii.fType           = MFT_SEPARATOR;
    mii.fState          = MFS_ENABLED;
    InsertMenuItem(menu, item_id, FALSE, &mii);
}


/*******************************************************************
** AddCommonItems
** ==============
** Adds common items to the end of menu.  Here, "common items" does
** not refer to user-defined items, but rather pre-defined things
** like the date.  Only items that are checked in the settings are
** added, so this function may do nothing.
**
** Inputs:
**      HMENU menu          - menu to add items to
**
** Outputs:
**      int                 - menu ID of the first item added
**                            successfully, or -1 if no items were
**                            added.
*******************************************************************/
int AddCommonItems(HMENU menu)
{
    MENUITEMINFO mii;
    SYSTEMTIME time;
    TCHAR long_date[POPUP_TEXT_LENGTH+1];
    TCHAR short_date[POPUP_TEXT_LENGTH+1];
    TCHAR custom_date[POPUP_TEXT_LENGTH+1];
    int start_id = -1;

    GetLocalTime(&time);

    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize          = sizeof(MENUITEMINFO);
    mii.fMask           = MIIM_TYPE | MIIM_STATE | MIIM_ID;
    mii.fType           = MFT_STRING;
    mii.fState          = MFS_ENABLED;

    if(gv.settings.show_long_date)
    {
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &time,
            NULL, long_date, POPUP_TEXT_LENGTH);

        mii.wID             = COMMON_MENU_LONG_DATE;
        mii.dwTypeData      = long_date;
        mii.cch             = (UINT) _tcslen(long_date);
        if(InsertMenuItem(menu, GetMenuItemCount(menu), TRUE, &mii))
        {
            start_id = COMMON_MENU_LONG_DATE;
        }
    }

    if(gv.settings.show_short_date)
    {
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time,
            NULL, short_date, POPUP_TEXT_LENGTH);

        mii.wID             = COMMON_MENU_SHORT_DATE;
        mii.dwTypeData      = short_date;
        mii.cch             = (UINT) _tcslen(short_date);
        if(InsertMenuItem(menu, GetMenuItemCount(menu), TRUE, &mii)
        && (start_id == -1))
        {
            start_id = COMMON_MENU_SHORT_DATE;
        }
    }

    if(gv.settings.show_custom_date)
    {
        GetPartialDateTime(gv.settings.date_format, custom_date,
            POPUP_TEXT_LENGTH, &time);

        mii.wID             = COMMON_MENU_CUSTOM_DATE;
        mii.dwTypeData      = custom_date;
        mii.cch             = (UINT) _tcslen(custom_date);
        if(InsertMenuItem(menu, GetMenuItemCount(menu), TRUE, &mii)
        && (start_id == -1))
        {
            start_id = COMMON_MENU_CUSTOM_DATE;
        }
    }

    return start_id;
}


/*******************************************************************
** SimulatePaste
** =============
** Simulates the user pressing Ctrl-V.  This is done through
** actual keypresses; there is no "paste" API in Windows.  This
** function accounts for cases where the keys are already held
** down.
*******************************************************************/
void SimulatePaste()
{
    BOOL alt_down = FALSE;
    BOOL ctrl_down = FALSE;
    BOOL shift_down = FALSE;
    BOOL v_down = FALSE;

    //If the Shift or Alt keys are down, we'll release
    //them temporarily.
    if(GetAsyncKeyState(VK_MENU) & 0x8000)
    {
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        alt_down = TRUE;
    }
    if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
    {
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
        shift_down = TRUE;
    }

    //If Control is already down, make a note that we
    //don't need to press it again.
    if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
    {
        ctrl_down = TRUE;
    }

    //If V is already down, we'll release it and press
    //it again to get a new key event.
    if(GetAsyncKeyState(VkKeyScan('v')) & 0x8000)
    {
        keybd_event((BYTE) VkKeyScan('v'), 0, KEYEVENTF_KEYUP, 0);
        v_down = TRUE;
    }

    //If Control was not down, we'll have to press it.
    if(!ctrl_down)
    {
        keybd_event(VK_CONTROL, 0, 0, 0);
    }

    //Now press V to simulate Paste.
    keybd_event((BYTE) VkKeyScan('v'), 0, 0, 0);

    //If we started out with Ctrl or V up, put them back
    //in that state.
    if(!v_down)
    {
        keybd_event((BYTE) VkKeyScan('v'), 0, KEYEVENTF_KEYUP, 0);
    }
    if(!ctrl_down)
    {
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    }

    //Similarly, if we started out with Shift or Alt down,
    //put them back in that state.
    if(alt_down)
    {
        keybd_event(VK_MENU, 0, 0, 0);
    }
    if(shift_down)
    {
        keybd_event(VK_SHIFT, 0, 0, 0);
    }
}


/*******************************************************************
** HandleTrayMessage
** =================
** Handles messages sent to the system tray icon, e.g. right-click
** to show the application menu, or double-click to open the
** settings dialog.
**
** Inputs:
**      HWND hwnd           - handle to the main application window
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL HandleTrayMessage(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch((UINT) lParam)
    {
        case WM_LBUTTONDBLCLK:
            OpenSettingsDialog();
            break;

        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
        {
            POINT point;
            HMENU menu, popup_menu;
            MENUITEMINFO mii;
            ZeroMemory(&mii, sizeof(MENUITEMINFO));
            mii.cbSize = sizeof(MENUITEMINFO);

            menu = LoadMenu(GetModuleHandle(NULL),
                MAKEINTRESOURCE(MENU_TRAY));

            popup_menu = GetSubMenu(menu, 0);
            PopulateRecentMenu(popup_menu, IDM_RECENT);

            if(gv.cq.modified)
            {
                mii.fMask   = MIIM_STATE;
                mii.fState  = MFS_ENABLED;
                SetMenuItemInfo(popup_menu, IDM_SAVE, FALSE, &mii);
            }

            if(gv.enable_monitoring)
            {
                mii.fMask   = MIIM_STATE;
                mii.fState  = MFS_CHECKED;
                SetMenuItemInfo(popup_menu, IDM_ENABLE, FALSE, &mii);
            }

            GetCursorPos(&point);

            SetForegroundWindow(hwnd);
            TrackPopupMenu(popup_menu,
                TPM_LEFTBUTTON,
                point.x,
                point.y,
                0,
                hwnd,
                NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);

            DestroyMenu(menu);
            break;
        }
    }

    return TRUE;
}


/*******************************************************************
** CreateTrayIcon
** ==============
** Adds an icon to the system tray.
**
** Inputs:
**      HWND hwnd           - handle to the window that will
**                            receive system tray messages.  This
**                            should be the main application window.
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL CreateTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA nid;

    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));

    nid.cbSize              = sizeof(NOTIFYICONDATA);
    nid.hWnd                = hwnd;
    nid.uID                 = TRAY_ID;
    nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage    = TRAY_MESSAGE;
    nid.hIcon               = (HICON) LoadImage(GetModuleHandle(NULL),
                                MAKEINTRESOURCE(ICON_QCLIP),
                                IMAGE_ICON, 16, 16,
                                LR_LOADTRANSPARENT | LR_MONOCHROME);

    LoadString(GetModuleHandle(NULL),
        STRING_TRAY_TIP, nid.szTip, 63);

    return Shell_NotifyIcon(NIM_ADD, &nid);
}


/*******************************************************************
** DestroyTrayIcon
** ===============
** Removes the system tray icon created by CreateTrayIcon.
**
** Inputs:
**      HWND hwnd           - handle to the window that receives
**                            the system tray messages.
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL DestroyTrayIcon(HWND hwnd)
{
    NOTIFYICONDATA nid;

    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));

    nid.cbSize              = sizeof(NOTIFYICONDATA);
    nid.hWnd                = hwnd;
    nid.uID                 = TRAY_ID;

    return Shell_NotifyIcon(NIM_DELETE, &nid);
}


/*******************************************************************
** MakeRelativePath
** ================
** Given a path to some file, checks if it is within the install
** path.  If so, the function returns a pointer to the portion of
** the path relative to the install path.
**
** Inputs:
**      TCHAR* path         - path to some file.
**
** Outputs:
**      TCHAR*              - pointer to relative path; if the path
**                            was not relative to the exe, returns
**                            the full path as originally given.
*******************************************************************/
TCHAR* MakeRelativePath(TCHAR* path)
{
    TCHAR* relative_path = path;
    TCHAR install_path[MAX_PATH+1];

	if(GetModuleFileName(GetModuleHandle(NULL), install_path, MAX_PATH))
	{
        TCHAR* install_path_end = _tcsrchr(install_path, _T('\\'));

        if(install_path_end)
        {
            TCHAR path_copy[MAX_PATH+1];

            _tcsncpy(path_copy, path, MAX_PATH);
            *install_path_end = _T('\0');

            CharUpper(install_path);
            CharUpper(path_copy);

            if(_tcsstr(path_copy, install_path))
            {
                //The +1 is to omit the leading '\', which we
                //removed from the end of the install path above.
                relative_path += _tcslen(install_path) + 1;
            }            
        }
    }

    return relative_path;
}


/*******************************************************************
** GetFileInInstallPath
** ====================
** Generates the full path to a file in the same directory as
** QClip.exe, given the name of the file.
**
** Inputs:
**      TCHAR* file_name    - name of the file
**      TCHAR* path         - buffer to hold the generated path,
**                            should be of size MAX_PATH
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL GetFileInInstallPath(TCHAR* file_name, TCHAR* path)
{
    BOOL result = FALSE;

	if(GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH))
	{
        TCHAR* path_end = _tcsrchr(path, _T('\\'));

        if(path_end)
        {
            _tcsncpy(path_end + 1, file_name, MAX_PATH - (path_end + 1 - path) - 1);
            result = TRUE;
        }
    }

    return result;
}


/*******************************************************************
** ShowErrorMessage
** ================
** Displays an error message to the user based on a resource ID.
**
** Inputs:
**      int resource_id     - resource ID of the error message
*******************************************************************/
void ShowErrorMessage(int resource_id)
{
    TCHAR error_message[ERROR_LENGTH+1];
    LoadString(GetModuleHandle(NULL),
        resource_id,
        error_message,
        ERROR_LENGTH);
    MessageBox(NULL, error_message, NULL,
        MB_OK | MB_ICONERROR);
}





