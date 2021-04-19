/****************************************************************************
** QClip
** Copyright 2006 Aaron Curtis
**
** This file is part of QClip.
**
** QClip is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** QClip is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with QClip. If not, see <https://www.gnu.org/licenses/>.
****************************************************************************/

#include <windows.h>
#include <prsht.h>
#include <commctrl.h>
#include "Settings.h"
#include "FormatSettings.h"
#include "QClip.h"
#include "Clipboard.h"
#include "resource.h"

static BOOL SaveFormatSettings(HWND dlg_window);
static void InitCheckboxes(HWND dlg_window);
static BOOL HandleAllFormatsCheckbox(HWND dlg_window);
static void RecordFormatCheckbox(HWND dlg_window, int checkbox_id);

/*******************************************************************
** FormatSettingsHandler
** =====================
** Top-level message handler for the Format Settings dialog.
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
FormatSettingsHandler(HWND dlg_window,
UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL return_value = FALSE;

    switch(message)
    {
        case WM_NOTIFY:
            switch(((NMHDR*) lParam)->code)
            {
                case PSN_APPLY:
                    return_value = SaveFormatSettings(dlg_window);
                    break;
            }
            break;

        case WM_COMMAND:
            if((LOWORD(wParam) >= FORMAT_TYPE_CB_START)
            && (LOWORD(wParam) < FORMAT_TYPE_CB_START + NUM_FORMAT_TYPES))
            {
                PropSheet_Changed(GetParent(dlg_window), dlg_window);
                RecordFormatCheckbox(dlg_window, LOWORD(wParam));
            }
            else if(LOWORD(wParam) == IDCB_ALL_FORMATS)
            {
                PropSheet_Changed(GetParent(dlg_window), dlg_window);
                HandleAllFormatsCheckbox(dlg_window);
            }
            break;

        case WM_INITDIALOG:
            //Save the address of the temporary settings
            //structure created when the property sheet
            //is opened.
            SetWindowLongPtr(dlg_window, GWL_USERDATA,
                (LONG) ((PROPSHEETPAGE*) lParam)->lParam);

            InitCheckboxes(dlg_window);
            HandleAllFormatsCheckbox(dlg_window);
            break;
    }

    return return_value;
}


/*******************************************************************
** HandleAllFormatsCheckbox
** ========================
** Called in response to a change to the "Enable All Formats"
** checkbox.  If the box is checked, disables the individual format
** checkboxes.  Saves the checkbox state to temp_settings.
**
** Inputs:
**      HWND dlg_window     - handle to the format settings dialog
**
** Outputs:
**      BOOL                - TRUE if the checkbox setting was
**                            recorded as TRUE.
*******************************************************************/
BOOL HandleAllFormatsCheckbox(HWND dlg_window)
{
    BOOL result = FALSE;
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        unsigned int i;

        temp_settings->enable_all_formats =
            IsDlgButtonChecked(dlg_window, IDCB_ALL_FORMATS);

        result = temp_settings->enable_all_formats;

        for(i = 0; i < NUM_FORMAT_TYPES; ++i)
        {
            EnableWindow(GetDlgItem(dlg_window, FORMAT_TYPE_CB_START + i),
                !result);
        }
    }

    return result;
}


/*******************************************************************
** RecordFormatCheckbox
** ====================
** Called in response to a change to one of the individual format
** checkboxes.  Saves the checkbox state to temp_settings.
**
** Inputs:
**      HWND dlg_window     - handle to the format settings dialog
**      int checkbox_id     - ID of the changed checkbox
*******************************************************************/
void RecordFormatCheckbox(HWND dlg_window, int checkbox_id)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        if(IsDlgButtonChecked(dlg_window, checkbox_id))
        {
            temp_settings->format_flags |=
                (1 << (checkbox_id - FORMAT_TYPE_CB_START));
        }
        else
        {
            temp_settings->format_flags &=
                ~(1 << (checkbox_id - FORMAT_TYPE_CB_START));        
        }
    }    
}


/*******************************************************************
** InitCheckboxes
** ==============
** Sets the state of all the checkboxes in the dialog according
** to the values in temp_settings.  Should be called on dialog
** initialization.
**
** Inputs:
**      HWND dlg_window     - handle to the format settings dialog
*******************************************************************/
void InitCheckboxes(HWND dlg_window)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        unsigned int i;

        for(i = 0; i < NUM_FORMAT_TYPES; ++i)
        {
            if(temp_settings->format_flags & (1 << i))
            {
                CheckDlgButton(dlg_window, FORMAT_TYPE_CB_START + i,
                    BST_CHECKED);
            }
            else
            {
                CheckDlgButton(dlg_window, FORMAT_TYPE_CB_START + i,
                    BST_UNCHECKED);
            }
        }

        if(temp_settings->enable_all_formats)
        {
            CheckDlgButton(dlg_window, IDCB_ALL_FORMATS,
                BST_CHECKED);
        }
        else
        {
            CheckDlgButton(dlg_window, IDCB_ALL_FORMATS,
                BST_UNCHECKED);
        }
    }
}



/*******************************************************************
** SaveFormatSettings
** ===================
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
BOOL SaveFormatSettings(HWND dlg_window)
{
    BOOL return_value = PSNRET_INVALID_NOCHANGEPAGE;
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        gv.settings.format_flags = temp_settings->format_flags;
        gv.settings.enable_all_formats = temp_settings->enable_all_formats;

        return_value = PSNRET_NOERROR;
        PropSheet_UnChanged(GetParent(dlg_window), dlg_window);
    }

    SetWindowLongPtr(dlg_window, DWL_MSGRESULT, return_value);
    return return_value;
}
