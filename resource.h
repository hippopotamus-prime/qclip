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


#define MENU_TRAY                   1000
#define IDM_EXIT                    1001
#define IDM_SETTINGS                1002
#define IDM_OPEN                    1003
#define IDM_SAVEAS                  1004
#define IDM_SAVE                    1005
#define IDM_RECENT                  1006
#define IDM_ENABLE                  1007
#define IDM_MANAGE                  1008
#define IDM_HELP                    1009
#define IDM_ABOUT                   1010
#define IDM_EMPTY                   1011

#define ICON_QCLIP                  1100

#define DIALOG_GENERAL_SETTINGS     2000
#define IDC_SIZE_BOX                2001
#define IDC_SIZE_UPDOWN             2002
#define IDCB_LOAD_PREVIOUS          2003
#define IDCB_LONG_DATE              2004
#define IDCB_SHORT_DATE             2005
#define IDC_COMMON_FILE             2006
#define IDC_COMMON_BROWSE           2007
#define IDCB_PREVIEW_BITMAPS        2008
#define IDCB_DYNAMIC_QUEUE          2009
#define IDCB_CUSTOM_DATE            2010
#define IDC_DATE_FORMAT             2011

#define DIALOG_KEY_SETTINGS         2100
#define IDC_COMMAND_LIST            2102
#define IDC_HOTKEY                  2103
#define IDC_CLEARKEY                2104

#define DIALOG_FORMAT_SETTINGS      2200
#define IDCB_ALL_FORMATS            2201
#define FORMAT_TYPE_CB_START        2202
#define IDCB_TEXT                   2202
#define IDCB_BITMAP                 2203
#define IDCB_FILES                  2204
#define IDCB_META                   2205
#define IDCB_WAVE                   2206
#define IDCB_OTHER                  2207
#define IDCB_PRIVATE                2208
#define IDCB_REGISTERED             2209

#define DIALOG_ABOUT                2300
#define IDC_ABOUT_EMAIL             2301
#define IDC_ABOUT_TITLE             2302
#define IDC_ABOUT_WEB               2303

#define POPUP_MENU_START            3000
#define COMMON_MENU_LONG_DATE       4000
#define COMMON_MENU_SHORT_DATE      4001
#define COMMON_MENU_CUSTOM_DATE     4002
#define COMMON_MENU_START           4010
#define RECENT_MENU_START           5000

#define STRING_SETTINGS_TITLE       10000
#define STRING_INDEX_CHARS          10001
#define INDEX_CHARS_LENGTH          35
#define STRING_TRAY_TIP             10002
#define STRING_TYPE_FILTER			10003
#define STRING_DEFAULT_POPUP        10004
#define STRING_BITMAP_POPUP         10005
#define STRING_README               10006
#define STRING_FILE_POPUP           10007
#define STRING_EMAIL                10008
#define STRING_WEB                  10009

#define COMMAND_STRING_START        10051

#define STRING_POPUP                10051
#define STRING_POP_FRONT            10052
#define STRING_PEEK_FRONT           10053
#define STRING_PEEK_2               10054
#define STRING_PEEK_3               10055
#define STRING_PEEK_4               10056
#define STRING_PEEK_5               10057
#define STRING_POP_BACK             10058
#define STRING_PEEK_BACK            10059
#define STRING_EMPTY                10060
#define STRING_OPEN                 10061
#define STRING_SAVEAS               10062
#define STRING_SAVE                 10063
#define STRING_PASTE_LONG_DATE      10064
#define STRING_PASTE_SHORT_DATE     10065
#define STRING_PASTE_CUSTOM_DATE    10066
#define STRING_TOGGLE               10067
#define STRING_DISCARD_FRONT        10068
#define STRING_DISCARD_BACK         10069
#define STRING_COMMON_1             10070
#define STRING_COMMON_2             10071
#define STRING_COMMON_3             10072
#define STRING_COMMON_4             10073
#define STRING_COMMON_5             10074

#define STRING_ERROR_OPEN_FILE      10100


