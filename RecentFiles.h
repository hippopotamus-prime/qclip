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

#ifndef __RECENT_FILES__
#define __RECENT_FILES__


extern BOOL InitRecentFiles();
extern BOOL AddRecentFile(TCHAR* file_name);
extern void DestroyRecentFiles();
extern BOOL PopulateRecentMenu(HMENU menu, unsigned int position);
extern BOOL OpenRecentFile(unsigned int offset);

#define GetRecentFileName(offset)   (gv.recent[(gv.recent_front + offset) \
                                        % gv.settings.recent_files])
#define GetRecentCount()            (gv.recent_count)

#endif
