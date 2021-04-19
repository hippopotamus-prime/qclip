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





