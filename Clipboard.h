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

#ifndef __CLIPBOARD__
#define __CLIPBOARD__

#include <windows.h>

#define POPUP_TEXT_LENGTH   50

#define NUM_FORMAT_TYPES    8
#define FORMAT_TEXT         1
#define FORMAT_BITMAP       2
#define FORMAT_FILE         4
#define FORMAT_META         8
#define FORMAT_WAVE         16
#define FORMAT_OTHERS       32
#define FORMAT_PRIVATE      64
#define FORMAT_REGISTERED   128

typedef struct
{
    void*   memory;
    size_t  size;
    UINT    format;
}ClipData;

typedef struct
{
    ClipData*       data;       //dynamically allocated array
    unsigned int    formats;    //number of formats stored
}ClipItem;

extern BOOL AddClipItemToMenu(ClipItem* item,
    HMENU menu, int item_id, TCHAR* prefix);
extern unsigned int CopyToClipboard(ClipItem* item);
extern void DestroyClipItem(ClipItem* item);
extern unsigned int PopulateClipItem(ClipItem* item);
extern BOOL CopyStringToClipboard(TCHAR* text);
extern BOOL CompareClipItems(ClipItem* item1, ClipItem* item2);

#define IsAppFormat(format) (format >= 0x0C000)

#endif





