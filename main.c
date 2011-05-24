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

#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "QClip.h"

#define APP_NAME _T("QClip")

/*******************************************************************
** WinMain
** =======
** The entry point of the program, obviously.  This is mostly
** boilerplate from Dev-C++; all the real program code is
** elsewhere...
*******************************************************************/
int WINAPI _tWinMain (HINSTANCE hThisInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpszArgument,
                      int nFunsterStil)

{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = APP_NAME;
    wincl.lpfnWndProc = QClipHandler;    /* This function is called by windows */
    wincl.style = 0;                        /* Catch double-clicks */
    wincl.cbSize = sizeof(WNDCLASSEX);

    wincl.hIcon = LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(ICON_QCLIP),
        IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    wincl.hIconSm = LoadImage(GetModuleHandle(NULL),
        MAKEINTRESOURCE(ICON_QCLIP),
        IMAGE_ICON, 16, 16, 0);

    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           APP_NAME,            /* Classname */
           APP_NAME,            /* Title Text */
           0,                   /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           1,                   /* The programs width */
           1,                   /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow(hwnd, SW_HIDE);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while(GetMessage(&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return (int) messages.wParam;
}





