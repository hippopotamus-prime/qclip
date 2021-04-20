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
#include <tchar.h>
#include "About.h"
#include "resource.h"

static LRESULT CALLBACK
LinkHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*******************************************************************
** LinkHandler
** ===========
** Message handler for the static "link" controls.
**
** Inputs:
**      HWND hwnd           - handle to the control window
**      UINT message        - message ID
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      LRESULT             - result of message processing
*******************************************************************/
LRESULT CALLBACK
LinkHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC default_handler =
        (WNDPROC) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(message)
    {
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
    }

    if(default_handler)
    {
        return CallWindowProc(default_handler, hwnd, message, wParam, lParam);
    }
    else
    {
        return TRUE;
    }
}

/*******************************************************************
** AboutHandler
** ============
** Message handler for the about box.
**
** Inputs:
**      HWND hwnd           - handle to the about box window
**      UINT message        - message ID
**      WPARAM wParam       - message parameter populated by Windows
**      LPARAM lParam       - message parameter populated by Windows
**
** Outputs:
**      INT_PTR             - result of message processing
*******************************************************************/
INT_PTR CALLBACK
AboutHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL return_value = FALSE;

    switch(message)
    {
        case WM_INITDIALOG:
        {
            HFONT font = CreateFont(8, 0, 0, 0, FW_NORMAL, FALSE,
                TRUE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, _T("Ms Sans Serif"));

            /*
            HWND email_window = GetDlgItem(hwnd, IDC_ABOUT_EMAIL);
            WNDPROC default_email_handler = (WNDPROC) SetWindowLongPtr(
                email_window, GWLP_WNDPROC, (LONG_PTR) LinkHandler);
            SetWindowLongPtr(email_window, GWLP_USERDATA,
                (LONG_PTR) default_email_handler);
            SendMessage(email_window, WM_SETFONT, (WPARAM) font, 0);
            */

            HWND web_window = GetDlgItem(hwnd, IDC_ABOUT_WEB);
            WNDPROC default_web_handler = (WNDPROC) SetWindowLongPtr(
                web_window, GWLP_WNDPROC, (LONG_PTR) LinkHandler);

            SetWindowLongPtr(web_window, GWLP_USERDATA,
                (LONG_PTR) default_web_handler);

            SendMessage(web_window, WM_SETFONT, (WPARAM) font, 0);
            break;
        }

        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case STN_CLICKED:
                {
                    TCHAR url[MAX_PATH];
                    memset(url, 0, sizeof(url));

                    switch(LOWORD(wParam))
                    {
                        /*
                        case IDC_ABOUT_EMAIL:
                            LoadString(GetModuleHandle(NULL),
                                STRING_EMAIL, url, MAX_PATH);
                            break;
                        */
                        case IDC_ABOUT_WEB:
                            LoadString(GetModuleHandle(NULL),
                                STRING_WEB, url, MAX_PATH);
                            break;
                    }

                    if(url[0] != _T('\0'))
                    {
                        ShellExecute(NULL, _T("open"), url,
                            NULL, _T(""), SW_SHOW);
                    }
                    break;
                }
            }

            switch(LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                {
                    HFONT font = (HFONT) SendDlgItemMessage(hwnd,
                        IDC_ABOUT_EMAIL, WM_GETFONT, 0, 0);

                    /*
                    HWND email_window = GetDlgItem(hwnd, IDC_ABOUT_EMAIL);
                    WNDPROC default_email_handler = (WNDPROC) GetWindowLongPtr(
                        email_window, GWLP_USERDATA);
                    SetWindowLongPtr(email_window, GWLP_WNDPROC,
                        (LONG_PTR) default_email_handler);
                    */

                    HWND web_window = GetDlgItem(hwnd, IDC_ABOUT_WEB);

                    WNDPROC default_web_handler = (WNDPROC) GetWindowLongPtr(
                        web_window, GWLP_USERDATA);

                    SetWindowLongPtr(web_window, GWLP_WNDPROC,
                        (LONG_PTR) default_web_handler);

                    DeleteObject(font);

                    EndDialog(hwnd, LOWORD(wParam));
                    return_value = TRUE;
                    break;
                }
            }
            break;
    }

    return return_value;
}






