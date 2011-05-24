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

#define _WIN32_IE 0x0600

#include <windows.h>
#include <prsht.h>
#include <commctrl.h>
#include <tchar.h>
#include "Settings.h"
#include "KeySettings.h"
#include "QClip.h"
#include "resource.h"

#define KEY_DESC_LENGTH             50      //full name of a hotkey, e.g. "Ctrl+Alt+K"
#define KEY_SUFFIX_LENGTH           20      //key name with no modifiers, e.g. "Space"
#define MOD_NAME_LENGTH             8       //key modifier name, e.g. "Alt"
#define MAX_COMMAND_STRING_LENGTH   50

static void PopulateCommandList(HWND dlg_window);
static void PopulateHotKeyBox(HWND dlg_window);
static BOOL SaveKeySettings(HWND dlg_window);
static int ModsToControl(int mods);
static int ModsFromControl(int control_mods);


/*******************************************************************
** PopulateCommandList
** ===================
** Loads the names of commands from string resources and adds them
** to the command list box.
**
** Inputs:
**      HWND dlg_window     - handle to the key settings dialog
*******************************************************************/
void PopulateCommandList(HWND dlg_window)
{
    unsigned int i;
    unsigned int position;
    TCHAR command_string[MAX_COMMAND_STRING_LENGTH+1];

    /*
    LV_COLUMN column;
    LV_ITEM item;

    SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
        LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP |
        LVS_EX_LABELTIP);

    ZeroMemory(&column, sizeof(LV_COLUMN));
    column.mask     = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    column.fmt      = LVCFMT_LEFT;

    column.cx = 180;
    column.pszText = "Command";
    SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
        LVM_SETCOLUMN, 0, (LPARAM) &column);

    column.cx = 80;
    column.pszText = "Assigned";
    SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
        LVM_INSERTCOLUMN, 1, (LPARAM) &column);

    ZeroMemory(&item, sizeof(LV_ITEM));
    item.mask       = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    item.pszText    = command_string;
    item.cchTextMax = MAX_COMMAND_STRING_LENGTH;
    item.stateMask  = LVIS_SELECTED;
    */

    //for(i = COMMAND_STRING_START;
    //i < COMMAND_STRING_START + NUM_KEY_COMMANDS; ++i)
    for(i = 0; i < NUM_KEY_COMMANDS; ++i)
    {
        LoadString(GetModuleHandle(NULL), COMMAND_STRING_START + i,
            command_string, MAX_COMMAND_STRING_LENGTH);

        /*
        item.iItem = i;
        item.state = 0;
        item.lParam = (LPARAM) i;

        if(i == gv.settings.command_list_index)
        {
            item.state = LVIS_SELECTED;
        }
        
        SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
            LVM_INSERTITEM, 0, (LPARAM) &item);
        */

        position = (unsigned int) SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
            LB_ADDSTRING, 0, (LPARAM) command_string);

        SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
            LB_SETITEMDATA, (WPARAM) position, (LPARAM) i);
    }

    SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
        LB_SETCURSEL, (WPARAM) gv.settings.command_list_index, 0);    
}


/*******************************************************************
** PopulateHotKeyBox
** =================
** Fills in the dialog's edit box with a hot key (e.g. "Ctrl+Alt+V")
** based on the current selection in the command list box.
**
** Inputs:
**      HWND dlg_window     - handle to the key settings dialog
*******************************************************************/
void PopulateHotKeyBox(HWND dlg_window)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    HWND edit_window = GetDlgItem(dlg_window, IDC_HOTKEY);

    if(temp_settings && edit_window)
    {
        int position = (int) SendDlgItemMessage(dlg_window,
            IDC_COMMAND_LIST, LB_GETCURSEL, 0, 0);

        int command_index = (int) SendDlgItemMessage(dlg_window,
            IDC_COMMAND_LIST, LB_GETITEMDATA, (WPARAM) position, 0);

        /*
        LV_ITEM item;
        int command_index;

        item.iItem = SendDlgItemMessage(dlg_window,
            IDC_COMMAND_LIST, LVM_GETNEXTITEM, -1,
            LVNI_ALL | LVNI_SELECTED);
        item.iSubItem = 0;
        item.mask = LVIF_PARAM;

        SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
            LVM_GETITEM, 0, (LPARAM) &item);

        command_index = item.lParam;
        */

        if((command_index == LB_ERR)
        || (command_index >= NUM_KEY_COMMANDS))
        //if((command_index == -1)
        //|| (command_index >= NUM_KEY_COMMANDS))
        {
            SendMessage(edit_window, HKM_SETHOTKEY, 0, 0);
            EnableWindow(edit_window, FALSE);
        }
        else
        {
            if(temp_settings->command_keys[command_index] != 0)
            {
                SendMessage(edit_window, HKM_SETHOTKEY,
                    MAKEWORD(temp_settings->command_keys[command_index],
                    ModsToControl(temp_settings->command_mods[command_index])),
                    0);
            }
            else
            {
                SendMessage(edit_window, HKM_SETHOTKEY, 0, 0);
            }

            temp_settings->command_list_index = position;
            EnableWindow(edit_window, TRUE);
        }
    }
}


/*******************************************************************
** ModsToControl
** =============
** Converts key modifier codes from the format used by the
** RegisterHotKey function to the format used by the Windows
** hotkey control.  Windows is stupid.
**
** Inputs:
**      int mods        - key modifiers used by RegisterHotKey
**
** Outputs:
**      int             - key modifiers to pass to the hotkey
**                        control
*******************************************************************/
int ModsToControl(int mods)
{
    int control_mods = 0;

    if(mods & MOD_ALT)
    {
        control_mods |= HOTKEYF_ALT;
    }
    if(mods & MOD_CONTROL)
    {
        control_mods |= HOTKEYF_CONTROL;
    }
    if(mods & MOD_SHIFT)
    {
        control_mods |= HOTKEYF_SHIFT;
    }

    return control_mods;
}


/*******************************************************************
** ModsFromControl
** ===============
** Converts key modifier codes from the format used by the Windows
** hotkey control to the format used by the RegisterHotKey function.
** Windows is stupid.
**
** Inputs:
**      int control_mods    - key modifiers returned from the hotkey
**                            control
** Outputs:
**      int             - key modifiers to pass to RegisterHotKey
*******************************************************************/
int ModsFromControl(int control_mods)
{
    int mods = 0;

    if(control_mods & HOTKEYF_ALT)
    {
        mods |= MOD_ALT;
    }
    if(control_mods & HOTKEYF_CONTROL)
    {
        mods |= MOD_CONTROL;
    }
    if(control_mods & HOTKEYF_SHIFT)
    {
        mods |= MOD_SHIFT;
    }

    return mods;
}


/*******************************************************************
** HandleHotKeyChange
** ==================
** Called in response to changes being made to the hotkey control.
** Saves the hotkey according to the current item selected in the
** command list box and enables the Apply button.
**
** Inputs:
**      HWND dlg_window     - handle to the key settings dialog
*******************************************************************/
void HandleHotKeyChange(HWND dlg_window)
{
    int position = (int) SendDlgItemMessage(dlg_window,
        IDC_COMMAND_LIST, LB_GETCURSEL, 0, 0);

    int command_index = (int) SendDlgItemMessage(dlg_window,
        IDC_COMMAND_LIST, LB_GETITEMDATA, (WPARAM) position, 0);

    Settings* temp_settings = GetTempSettings(dlg_window);

    /*
    LV_ITEM item;
    int command_index;

    item.iItem = SendDlgItemMessage(dlg_window,
        IDC_COMMAND_LIST, LVM_GETNEXTITEM, -1,
        LVNI_ALL | LVNI_SELECTED);
    item.iSubItem = 0;
    item.mask = LVIF_PARAM;

    SendDlgItemMessage(dlg_window, IDC_COMMAND_LIST,
        LVM_GETITEM, 0, (LPARAM) &item);

    command_index = item.lParam;
    */

    if(temp_settings && (command_index != LB_ERR)
    && (command_index < NUM_KEY_COMMANDS))
    //if(temp_settings && (command_index != -1)
    //&& (command_index < NUM_KEY_COMMANDS))
    {
        int key = (int) SendDlgItemMessage(dlg_window,
            IDC_HOTKEY, HKM_GETHOTKEY, 0, 0);

        temp_settings->command_keys[command_index] = LOBYTE(key);
        temp_settings->command_mods[command_index] =
            ModsFromControl(HIBYTE(key));

        PropSheet_Changed(GetParent(dlg_window), dlg_window);
    }
}


/*******************************************************************
** KeySettingsHandler
** ==================
** Top-level message handler for the Key Settings dialog.
**
** Inputs:
**      HWND dlg_window     - handle to the key settings dialog
**      UINT message        - message ID
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      BOOL                - TRUE if the dialog handled this
**                            message
*******************************************************************/
INT_PTR CALLBACK
KeySettingsHandler(HWND dlg_window, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL return_value = FALSE;

    switch(message)
    {
        case WM_NOTIFY:
            switch(((NMHDR*) lParam)->code)
            {
                case PSN_APPLY:
                    return_value = SaveKeySettings(dlg_window);
                    break;  
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_COMMAND_LIST:
                    switch(HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                            PopulateHotKeyBox(dlg_window);
                            break;
                    }
                    break;

                case IDC_HOTKEY:
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            HandleHotKeyChange(dlg_window);
                            break;
                    }
                    break;

                case IDC_CLEARKEY:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        SendDlgItemMessage(dlg_window,
                            IDC_HOTKEY, HKM_SETHOTKEY, 0, 0);
                    }
                    break;
            }
            break;

        case WM_INITDIALOG:
        {
            //Save the address of the temporary settings
            //structure created when the property sheet
            //is opened.
            SetWindowLongPtr(dlg_window, GWL_USERDATA,
                (LONG) ((PROPSHEETPAGE*) lParam)->lParam);

            //Populate the various controls...
            SendDlgItemMessage(dlg_window, IDC_HOTKEY,
                HKM_SETRULES, HKCOMB_NONE | HKCOMB_S,
                MAKELPARAM(HOTKEYF_CONTROL, 0));

            PopulateCommandList(dlg_window);
            PopulateHotKeyBox(dlg_window);
            break;
        }
    }

    return return_value;
}


/*******************************************************************
** SaveKeySettings
** ===============
** Called in response to the user pressing the OK or Apply buttons
** on the property sheet.  Saves the sheet's temporary settings.
**
** Inputs:
**      HWND dlg_window     - handle to the current property dialog
**
** Outputs:
**      BOOL                - a PSNRET value used by the property
**                            sheet.
*******************************************************************/
BOOL SaveKeySettings(HWND dlg_window)
{
    BOOL return_value = PSNRET_INVALID_NOCHANGEPAGE;
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        unsigned int i;

        UnregisterAllHotKeys(gv.main_window);

        for(i = 0; i < NUM_KEY_COMMANDS; ++i)
        {
            gv.settings.command_keys[i] = temp_settings->command_keys[i];
            gv.settings.command_mods[i] = temp_settings->command_mods[i];
        }
        gv.settings.command_list_index = temp_settings->command_list_index;

        RegisterAllHotKeys(gv.main_window);

        return_value = PSNRET_NOERROR;
        PropSheet_UnChanged(GetParent(dlg_window), dlg_window);
    }

    SetWindowLongPtr(dlg_window, DWL_MSGRESULT, return_value);
    return return_value;
}
