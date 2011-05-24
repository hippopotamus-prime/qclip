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

#ifndef __QCLIP__
#define __QCLIP__

#include <windows.h>
#include "Clipboard.h"
#include "ClipQueue.h"
#include "Settings.h"

#define NUM_SHELL_FORMATS 15

typedef struct
{
    HWND            next_viewer;
    HWND            main_window;
    HWND            settings_window;
    Settings        settings;
    ClipQueue       cq;
    ClipQueue       common;
    UINT            shell_formats[NUM_SHELL_FORMATS];
    unsigned int    ignore;         //ignores the next N copy events
    TCHAR**         recent;
    unsigned int    recent_front;
    unsigned int    recent_count;
    BOOL            opened_file;
    BOOL            enable_monitoring;
}Globals;

extern Globals gv;

LRESULT CALLBACK
QClipHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

extern void RegisterAllHotKeys(HWND hwnd);
extern void UnregisterAllHotKeys(HWND hwnd);

extern TCHAR* MakeRelativePath(TCHAR* path);
extern BOOL GetFileInInstallPath(TCHAR* file_name, TCHAR* path);
extern void ShowErrorMessage(int ID);

#endif







