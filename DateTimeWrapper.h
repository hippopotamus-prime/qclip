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

#ifndef __DATE_TIME_WRAPPER__
#define __DATE_TIME_WRAPPER__

extern void GetPartialDateTime(TCHAR* format, TCHAR* buffer,
    size_t buffer_size, SYSTEMTIME* time);

extern TCHAR* AllocateFullDateTime(TCHAR* format, SYSTEMTIME* time);

extern size_t FormatDateTime(TCHAR* format, TCHAR* buffer,
    size_t buffer_size, SYSTEMTIME* time);

#endif

