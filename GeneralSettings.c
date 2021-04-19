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

#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <prsht.h>
#include <commctrl.h>
#include <tchar.h>
#include "Settings.h"
#include "GeneralSettings.h"
#include "QClip.h"
#include "ClipFile.h"
#include "RecentFiles.h"
#include "resource.h"

#define BoolToCheck(x) ((x) ? BST_CHECKED : BST_UNCHECKED)

static void InitCheckboxes(HWND dlg_window);
static void InitSizeControl(HWND dlg_window);
static void InitCommonFileName(HWND dlg_window);
static BOOL RecordSizeControl(HWND dlg_window);
static BOOL SaveGeneralSettings(HWND dlg_window);
static void GetCommonFileName(HWND dlg_window);
static BOOL HandleDynamicQueueCheckbox(HWND dlg_window);


/*******************************************************************
** InitCheckboxes
** ==============
** Sets the state of all the checkboxes in the dialog according
** to the values in temp_settings.  Should be called on dialog
** initialization.
**
** Inputs:
**      HWND dlg_window     - handle to the general settings dialog
*******************************************************************/
void InitCheckboxes(HWND dlg_window)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        CheckDlgButton(dlg_window, IDCB_DYNAMIC_QUEUE,
            BoolToCheck(temp_settings->dynamic_queue));

        CheckDlgButton(dlg_window, IDCB_LOAD_PREVIOUS,
            BoolToCheck(temp_settings->load_previous));

        CheckDlgButton(dlg_window, IDCB_PREVIEW_BITMAPS,
            BoolToCheck(temp_settings->preview_bitmaps));

        CheckDlgButton(dlg_window, IDCB_LONG_DATE,
            BoolToCheck(temp_settings->show_long_date));

        CheckDlgButton(dlg_window, IDCB_SHORT_DATE,
            BoolToCheck(temp_settings->show_short_date));

        CheckDlgButton(dlg_window, IDCB_CUSTOM_DATE,
            BoolToCheck(temp_settings->show_custom_date));
    }
}


/*******************************************************************
** HandleDynamicQueueCheckbox
** ==========================
** Called in response to a change to the "Unlimited queue"
** checkbox.  If the box is checked, disables the queue size
** controls.  Saves the checkbox state to temp_settings.
**
** Inputs:
**      HWND dlg_window     - handle to the format settings dialog
**
** Outputs:
**      BOOL                - TRUE if the checkbox setting was
**                            recorded as TRUE.
*******************************************************************/
BOOL HandleDynamicQueueCheckbox(HWND dlg_window)
{
    BOOL result = FALSE;
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        temp_settings->dynamic_queue =
            IsDlgButtonChecked(dlg_window, IDCB_DYNAMIC_QUEUE);

        result = temp_settings->dynamic_queue;

        EnableWindow(GetDlgItem(dlg_window, IDC_SIZE_BOX), !result);
        EnableWindow(GetDlgItem(dlg_window, IDC_SIZE_UPDOWN), !result);
    }

    return result;
}


/*******************************************************************
** InitSizeControl
** ===============
** Sets the state of the queue size up-down control.  Should be
** called on dialog initialization.
**
** Inputs:
**      HWND dlg_window     - handle to the general settings dialog
*******************************************************************/
void InitSizeControl(HWND dlg_window)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        SendDlgItemMessage(dlg_window,
            IDC_SIZE_UPDOWN,
            UDM_SETRANGE, 0,
            (LPARAM) MAKELONG(UD_MAXVAL, 1));

        SendDlgItemMessage(dlg_window,
            IDC_SIZE_UPDOWN,
            UDM_SETPOS, 0,
            (LPARAM) MAKELONG(temp_settings->queue_size, 0));
    }
}


/*******************************************************************
** RecordSizeControl
** =================
** Called in response to a change to the queue size up-down control.
** Records the state of the control in temp_settings.
**
** Inputs:
**      HWND dlg_window     - handle to the general settings dialog
**
** Outputs:
**      BOOL                - TRUE if recording succeeded
*******************************************************************/
BOOL RecordSizeControl(HWND dlg_window)
{
    BOOL success = FALSE;

    Settings* temp_settings = GetTempSettings(dlg_window);

    LRESULT updown_pos = SendDlgItemMessage(
        dlg_window, IDC_SIZE_UPDOWN,
        UDM_GETPOS, 0, 0);

    if(temp_settings && (HIWORD(updown_pos) == 0))
    {
        temp_settings->queue_size = LOWORD(updown_pos);
        PropSheet_Changed(GetParent(dlg_window), dlg_window);
        success = TRUE;
    }

    return success;
}


/*******************************************************************
** InitCommonFileName
** ==================
** Sets the state of the "Custom items" text box.  Should be
** called on dialog initialization.
**
** Inputs:
**      HWND dlg_window     - handle to the general settings dialog
*******************************************************************/
void InitCommonFileName(HWND dlg_window)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        SendDlgItemMessage(dlg_window, IDC_COMMON_FILE,
            WM_SETTEXT, 0, (LPARAM) temp_settings->common_file);            
    }

    SendDlgItemMessage(dlg_window, IDC_COMMON_FILE,
        EM_LIMITTEXT, (WPARAM) MAX_PATH, 0);
}


/*******************************************************************
** InitDateFormat
** ==============
** Sets the state of the date / time format text box.  Should be
** called on dialog initialization.
**
** Inputs:
**      HWND dlg_window     - handle to the general settings dialog
*******************************************************************/
void InitDateFormat(HWND dlg_window)
{
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        SendDlgItemMessage(dlg_window, IDC_DATE_FORMAT,
            WM_SETTEXT, 0, (LPARAM) temp_settings->date_format);            
    }

    SendDlgItemMessage(dlg_window, IDC_DATE_FORMAT,
        EM_LIMITTEXT, (WPARAM) MAX_DATE_FORMAT_LENGTH, 0);
}


/*******************************************************************
** GetCommonFileName
** =================
** Prompts the user for a file and populates the "Custom items"
** text box according to the user's selection.  Should be called
** when the Browse button is clicked.
**
** Inputs:
**      HWND dlg_window     - handle to the general settings dialog
*******************************************************************/
void GetCommonFileName(HWND dlg_window)
{
    TCHAR file_name[MAX_PATH] = _T("");
	TCHAR type_filter[TYPE_FILTER_LENGTH+1];
    OPENFILENAME ofn;

    LoadFilterString(type_filter);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.lpstrFilter     = type_filter;
    ofn.lpstrFile       = file_name;
    ofn.nMaxFile        = MAX_PATH;
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = FILE_TYPE;

    if(GetRecentCount() > 0)
    {
        ofn.lpstrInitialDir = GetRecentFileName(0);
    }

	if(GetOpenFileName(&ofn))
    {
        TCHAR* final_file_name = MakeRelativePath(file_name);

        SendDlgItemMessage(dlg_window, IDC_COMMON_FILE,
            WM_SETTEXT, 0, (LPARAM) final_file_name);            
        PropSheet_Changed(GetParent(dlg_window), dlg_window);
    }
}


/*******************************************************************
** GeneralSettingsHandler
** ======================
** Top-level message handler for the General Settings dialog.
**
** Inputs:
**      HWND dlg_window     - handle to the key settings dialog
**      UINT message        - message ID
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      INT_PTR             - TRUE if the dialog handled this
**                            message
*******************************************************************/
INT_PTR CALLBACK
GeneralSettingsHandler(HWND dlg_window,
UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL return_value = FALSE;

    switch(message)
    {
        case WM_NOTIFY:
            switch(((NMHDR*) lParam)->code)
            {
                case PSN_APPLY:
                    return_value = SaveGeneralSettings(dlg_window);
                    break;  
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_SIZE_BOX:
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            RecordSizeControl(dlg_window);
                            break;
                    }
                    break;

                case IDC_COMMON_BROWSE:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        GetCommonFileName(dlg_window);
                    }
                    break;

                case IDCB_DYNAMIC_QUEUE:
                    HandleDynamicQueueCheckbox(dlg_window);
                    //flow into the next case...

                //The following controls are recorded on-demand
                //when the user presses Apply or OK:
                case IDCB_LOAD_PREVIOUS:
                case IDCB_PREVIEW_BITMAPS:
                case IDCB_LONG_DATE:
                case IDCB_SHORT_DATE:
                case IDCB_CUSTOM_DATE:
                    PropSheet_Changed(GetParent(dlg_window), dlg_window);
                    break;

                case IDC_DATE_FORMAT:
                case IDC_COMMON_FILE:
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            PropSheet_Changed(GetParent(dlg_window),
                                dlg_window);
                            break;
                    }
                    break;
            }
            break;

        case WM_INITDIALOG:
            //Save the address of the temporary settings
            //structure created when the property sheet
            //is opened.
            SetWindowLongPtr(dlg_window, GWL_USERDATA,
                (LONG) ((PROPSHEETPAGE*) lParam)->lParam);

            InitSizeControl(dlg_window);
            InitCheckboxes(dlg_window);
            InitCommonFileName(dlg_window);
            InitDateFormat(dlg_window);
            HandleDynamicQueueCheckbox(dlg_window);
            break;
    }

    return return_value;
}


/*******************************************************************
** SaveGeneralSettings
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
BOOL SaveGeneralSettings(HWND dlg_window)
{
    BOOL return_value = PSNRET_INVALID_NOCHANGEPAGE;
    Settings* temp_settings = GetTempSettings(dlg_window);

    if(temp_settings)
    {
        if(temp_settings->queue_size != gv.settings.queue_size)
        {
            ResizeQueue(&gv.cq, temp_settings->queue_size);
            gv.settings.queue_size = temp_settings->queue_size;
        }

        temp_settings->dynamic_queue =
            IsDlgButtonChecked(dlg_window, IDCB_DYNAMIC_QUEUE);
        gv.settings.dynamic_queue = temp_settings->dynamic_queue;

        temp_settings->load_previous =
            IsDlgButtonChecked(dlg_window, IDCB_LOAD_PREVIOUS);
        gv.settings.load_previous = temp_settings->load_previous;

        temp_settings->preview_bitmaps =
            IsDlgButtonChecked(dlg_window, IDCB_PREVIEW_BITMAPS);
        gv.settings.preview_bitmaps = temp_settings->preview_bitmaps;

        temp_settings->show_long_date =
            IsDlgButtonChecked(dlg_window, IDCB_LONG_DATE);
        gv.settings.show_long_date = temp_settings->show_long_date;

        temp_settings->show_short_date =
            IsDlgButtonChecked(dlg_window, IDCB_SHORT_DATE);
        gv.settings.show_short_date = temp_settings->show_short_date;

        temp_settings->show_custom_date =
            IsDlgButtonChecked(dlg_window, IDCB_CUSTOM_DATE);
        gv.settings.show_custom_date = temp_settings->show_custom_date;

        SendDlgItemMessage(dlg_window, IDC_COMMON_FILE, WM_GETTEXT,
            (WPARAM) MAX_PATH, (LPARAM) temp_settings->common_file);
        _tcscpy(gv.settings.common_file, temp_settings->common_file);
        OpenCommonItems(TRUE);

        SendDlgItemMessage(dlg_window, IDC_DATE_FORMAT,
            WM_GETTEXT, (WPARAM) MAX_DATE_FORMAT_LENGTH,
            (LPARAM) temp_settings->date_format);
        _tcscpy(gv.settings.date_format, temp_settings->date_format);

        return_value = PSNRET_NOERROR;
        PropSheet_UnChanged(GetParent(dlg_window), dlg_window);
    }

    SetWindowLongPtr(dlg_window, DWL_MSGRESULT, return_value);
    return return_value;
}
