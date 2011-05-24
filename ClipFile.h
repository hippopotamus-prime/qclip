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

#ifndef __CLIPFILE__
#define __CLIPFILE__

#include <windows.h>
#include <tchar.h>

#define TYPE_FILTER_LENGTH	50
#define FILE_TYPE           _T("qcl")

extern BOOL LoadQueueFromFile(ClipQueue* cq, HANDLE fhand);
extern BOOL SaveQueueToFile(ClipQueue* cq, HANDLE fhand);

extern void LoadFilterString(TCHAR* buffer);
extern BOOL OpenQueue();
extern BOOL SaveQueueAs();
extern BOOL SaveQueue();

extern BOOL OpenQueueFromDefault();
extern BOOL SaveQueueAsDefault();

extern BOOL OpenCommonItems();

#endif
