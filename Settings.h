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

#ifndef __SETTINGS__
#define __SETTINGS__

#include <windows.h>

#define NUM_KEY_COMMANDS            24
#define COMMAND_KEY_START           700
#define KEY_POPUP                   700
#define KEY_POP_FRONT               701
#define KEY_PEEK_FRONT              702
#define KEY_PEEK_2                  703
#define KEY_PEEK_3                  704
#define KEY_PEEK_4                  705
#define KEY_PEEK_5                  706
#define KEY_POP_BACK                707
#define KEY_PEEK_BACK               708
#define KEY_EMPTY                   709
#define KEY_OPEN                    710
#define KEY_SAVEAS                  711
#define KEY_SAVE                    712
#define KEY_PASTE_LONG_DATE         713
#define KEY_PASTE_SHORT_DATE        714
#define KEY_PASTE_CUSTOM_DATE       715
#define KEY_TOGGLE                  716
#define KEY_DISCARD_FRONT           717
#define KEY_DISCARD_BACK            718
#define KEY_COMMON_1                719
#define KEY_COMMON_2                720
#define KEY_COMMON_3                721
#define KEY_COMMON_4                722
#define KEY_COMMON_5                723

#define KEY_PEEK_START              KEY_PEEK_FRONT
#define KEY_PEEK_END                KEY_PEEK_5

#define KEY_COMMON_START            KEY_COMMON_1
#define KEY_COMMON_END              KEY_COMMON_5

#define NUM_PROPERTY_PAGES          3
#define GENERAL_PAGE                0
#define KEY_PAGE                    1
#define FORMAT_PAGE                 2

#define MAX_QUEUE_SIZE              UD_MAXVAL
#define MAX_RECENT                  99
#define MAX_DATE_FORMAT_LENGTH      128

typedef struct
{
    int             command_keys[NUM_KEY_COMMANDS];
    int             command_mods[NUM_KEY_COMMANDS];
    unsigned int    command_list_index;
    unsigned int    queue_size;
    unsigned int    format_flags;
    unsigned int    recent_files;
    TCHAR           common_file[MAX_PATH];
    TCHAR           date_format[MAX_DATE_FORMAT_LENGTH];
    BOOL            enable_all_formats;
    BOOL            load_previous;
    BOOL            preview_bitmaps;
    BOOL            show_long_date;
    BOOL            show_short_date;
    BOOL            show_custom_date;
    BOOL            dynamic_queue;
}Settings;

INT_PTR OpenSettingsDialog();
void LoadSettingsFromDisk();
void SaveSettingsToDisk();

//This BS is brought to you by a bug in MSVC 2005
#ifdef _WIN64
#define GetTempSettings(dlg_window) \
    ((Settings*) GetWindowLongPtr(dlg_window, GWLP_USERDATA))
#else
#define GetTempSettings(dlg_window) \
    ((Settings*) LongToPtr(GetWindowLongPtr(dlg_window, GWLP_USERDATA)))
#endif

#endif




