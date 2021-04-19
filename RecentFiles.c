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
#include <tchar.h>
#include <stdio.h>
#include "RecentFiles.h"
#include "QClip.h"
#include "ClipFile.h"
#include "resource.h"

static void RemoveRecentFile(unsigned int offset);
static void MoveRecentToTop(unsigned int offset);
static int FindRecentFile(TCHAR* file_name);


/*******************************************************************
** FindRecentFile
** ==============
** Given a file name, return its postion in the recent files queue,
** or -1 if not found.
**
** Inputs:
**      TCHAR* file_name    - file name to search for
**
** Outputs:
**      int                 - index of file name in the queue,
**                            where 0 is the most recent.
*******************************************************************/
int FindRecentFile(TCHAR* file_name)
{
    unsigned int i;
    int result = -1;

    for(i = 0; i < gv.recent_count; ++i)
    {
        if(_tcscmp(file_name, GetRecentFileName(i)) == 0)
        {
            result = i;
            break;
        }
    }

    return result;
}


/*******************************************************************
** OpenRecentFile
** ==============
** Loads the clipboard queue from a file in the recent files queue.
** This function will also move the file name to the top of the
** recent files queue, or remove it if the load failed.
**
** Inputs:
**      unsigned int offset - index of file name in the queue,
**                            where 0 is the most recent.
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL OpenRecentFile(unsigned int offset)
{
    BOOL success = FALSE;

	HANDLE fhand = CreateFile(GetRecentFileName(offset),
        GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(fhand != INVALID_HANDLE_VALUE)
    {
        ClipQueue cq;

        if(LoadQueueFromFile(&cq, fhand))
        {
            DestroyQueue(&gv.cq);
            gv.cq = cq;
            MoveRecentToTop(offset);
            success = TRUE;
            gv.opened_file = TRUE;
        }

		CloseHandle(fhand);
    }

    if(!success)
    {
        ShowErrorMessage(STRING_ERROR_OPEN_FILE);
        RemoveRecentFile(offset);
    }

    return success;
}


/*******************************************************************
** RemoveRecentFile
** ================
** Removes a file name from the recent files queue.  Call this
** e.g. if a file name is found to be invalid.
**
** Inputs:
**      unsigned int offset - index of file name in the queue,
**                            where 0 is the most recent.
*******************************************************************/
void RemoveRecentFile(unsigned int offset)
{
    if(offset < gv.recent_count)
    {
        unsigned int i;
        HeapFree(GetProcessHeap(), 0, GetRecentFileName(offset));

        for(i = offset; i < gv.recent_count - 1; ++i)
        {
            gv.recent[(gv.recent_front + i) % gv.settings.recent_files] =
                GetRecentFileName(i + 1);
        }

        gv.recent[(gv.recent_front + gv.recent_count - 1)
            % gv.settings.recent_files] = NULL;
        --gv.recent_count;
    }
}


/*******************************************************************
** MoveRecentToTop
** ===============
** Moves a file name in the recent files queue to the front of the
** queue, making it the most recent.
**
** Inputs:
**      unsigned int offset - index of file name in the queue,
**                            where 0 is the most recent.
*******************************************************************/
void MoveRecentToTop(unsigned int offset)
{
    if(offset < gv.recent_count)
    {
        TCHAR* file_name = GetRecentFileName(offset);
        int i;
    
        for(i = offset; i > 0; --i)
        {
            gv.recent[(gv.recent_front + i) % gv.settings.recent_files] =
                GetRecentFileName(i - 1);
        }

        gv.recent[gv.recent_front] = file_name;
    }
}


/*******************************************************************
** PopulateRecentMenu
** ==================
** Creates a popup menu listing all the recent files and attaches
** it to a given menu at some position.  Each menu item will be
** given an ID equal to its position in the queue plus
** RECENT_MENU_START.
**
** Inputs:
**      HMENU menu          - parent menu to attach the recent
**                            files popup menu to
**      int resource_id     - identifier in the parent menu to
**                            attach at
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL PopulateRecentMenu(HMENU menu, unsigned int resource_id)
{
    BOOL enabled = FALSE;

    //Under WinXP it is possible to right-click on the tray icon
    //as the program is shutting down, even after the icon is
    //removed by Shell_NotifyIcon.  At that point the recent files
    //list will also have been destroyed, so it's important to
    //check for that here.

    if(gv.recent)
    {
        HMENU recent_menu = CreatePopupMenu();

        if(recent_menu)
        {
            unsigned int i;
            TCHAR file_name[MAX_PATH+4];
            TCHAR indexes[INDEX_CHARS_LENGTH+1];
            unsigned int name_length;
            unsigned int items = 0;
            MENUITEMINFO mii;
    
            LoadString(GetModuleHandle(NULL), STRING_INDEX_CHARS,
                indexes, INDEX_CHARS_LENGTH+1);
    
            ZeroMemory(&mii, sizeof(MENUITEMINFO));
            mii.cbSize  = sizeof(MENUITEMINFO);
            mii.fType   = MFT_STRING;
            mii.fState  = MFS_ENABLED;
            mii.fMask   = MIIM_TYPE | MIIM_STATE | MIIM_ID;
    
            for(i = 0; i < gv.recent_count; ++i)
            {
                if(i < INDEX_CHARS_LENGTH)
                {
                    _sntprintf(file_name, MAX_PATH+3, _T("&%c %s"),
                        indexes[i], GetRecentFileName(i));
                }
                else
                {
                    _sntprintf(file_name, MAX_PATH+3, _T("   %s"),
                        GetRecentFileName(i));
                }
    
                name_length = (unsigned int) _tcslen(file_name);

                if(name_length > 3)
                {    
                    mii.wID             = RECENT_MENU_START + i;
                    mii.dwTypeData      = file_name;
                    mii.cch             = name_length;
        
                    if(InsertMenuItem(recent_menu, items, TRUE, &mii))
                    {
                        ++items;
                    }
                }
            }
        
            if(items > 0)
            {
                mii.fMask       = MIIM_STATE | MIIM_SUBMENU;
                mii.fState      = MFS_ENABLED;
                mii.hSubMenu    = recent_menu;
        
                enabled = SetMenuItemInfo(menu, resource_id, FALSE, &mii);
            }
            else
            {
                DestroyMenu(recent_menu);
            }
        }
    }

    return enabled;
}


/*******************************************************************
** InitRecentFiles
** ===============
** Allocates memory to store a queueu of recent file names,
** according to gv.settings.recent_files.  The queue is initially
** empty.  Memory is freed by DestroyRecentFiles.
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL InitRecentFiles()
{
    gv.recent = (TCHAR**) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        sizeof(TCHAR*) * gv.settings.recent_files);

    gv.recent_front = 0;
    gv.recent_count = 0;

    return (gv.recent != NULL);
}


/*******************************************************************
** AddRecentFile
** =============
** Adds a new file name to the recent files queue.  The added file
** will become the most recent, so would be accessed via
** GetRecentFile(0).  This function copies the file name into
** newly allocated memory, which is freed by DestroyRecentFiles.
** If the file name already exists in the queue, it will instead
** be moved to the front.
**
** Inputs:
**      TCHAR* file_name     - file name to add
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL AddRecentFile(TCHAR* file_name)
{
    BOOL success = FALSE;

    if(gv.recent)
    {
        int find_result = FindRecentFile(file_name);

        if(find_result != -1)
        {
            MoveRecentToTop(find_result);
            success = TRUE;
        }
        else
        {
            TCHAR* buffer = (TCHAR*) HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, sizeof(TCHAR) * (_tcslen(file_name) + 1));

            if(buffer)
            {
                //The queue grows backwards, so gv.recent_front always
                //refers to the most recent item.
                gv.recent_front =
                    (gv.recent_front - 1 + gv.settings.recent_files)
                    % gv.settings.recent_files;

                _tcscpy(buffer, file_name);

                if(gv.recent[gv.recent_front])
                {
                    HeapFree(GetProcessHeap(), 0, gv.recent[gv.recent_front]);
                }

                gv.recent[gv.recent_front] = buffer;

                if(gv.recent_count < gv.settings.recent_files)
                {
                    ++gv.recent_count;
                }

                success = TRUE;
            }
        }
    }

	return success;
}


/*******************************************************************
** DestroyRecentFiles
** ==================
** Frees all memory allocated by AddRecentFile and InitRecentFiles.
** Simple, eh?
*******************************************************************/
void DestroyRecentFiles()
{
    unsigned int i;

    if(gv.recent)
    {
        for(i = 0; i < gv.settings.recent_files; ++i)
        {
            if(gv.recent[i])
            {
                HeapFree(GetProcessHeap(), 0, gv.recent[i]);
                gv.recent[i] = NULL;
            }
        }

        HeapFree(GetProcessHeap(), 0, gv.recent);
        gv.recent = NULL;
    }
}
