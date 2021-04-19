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

extern BOOL OpenCommonItems(BOOL show_error);

#endif
