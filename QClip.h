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







