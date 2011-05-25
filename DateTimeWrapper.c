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

#include <windows.h>
#include <tchar.h>
#include "DateTimeWrapper.h"

#define MAX_FORMAT_WORD_SIZE 5

#define IsEscapeChar(x)     (x == _T('%'))

#define IsDateChar(x)       ((x == _T('d')) || (x == _T('M')) || \
                             (x == _T('y')) || (x == _T('g')))

#define IsTimeChar(x)       ((x == _T('h')) || (x == _T('H')) || \
                             (x == _T('m')) || (x == _T('s')) || \
                             (x == _T('t')))

#define IsFormattingChar(x) (IsDateChar(x) || IsTimeChar(x))



/*******************************************************************
** GetPartialDateTime
** ==================
** Safe wrapper around FormatDateTime.  Finds the required buffer
** size to format the full date / time string, allocates it, then
** copies as much as will fit into the buffer provided by the
** caller.
**
** Inputs:
**  TCHAR* section_name     - the format string
**  TCHAR* buffer           - buffer to hold the output
**  size_t buffer_size      - size of the output buffer
**  SYSTEMTIME* time        - the time to convert
*******************************************************************/
void GetPartialDateTime(TCHAR* format, TCHAR* buffer,
    size_t buffer_size, SYSTEMTIME* time)
{
    if(buffer)
    {
        TCHAR* full_buffer = AllocateFullDateTime(format, time);

        if(full_buffer)
        {
            _tcsncpy_s(buffer, buffer_size, full_buffer, buffer_size);
            HeapFree(GetProcessHeap(), 0, full_buffer);
        }
    }
}


/*******************************************************************
** AllocateFullDateTime
** ====================
** Safe wrapper around FormatDateTime.  Finds the required buffer
** size to format the full date / time string and allocates it
** with HeapAlloc.
**
** Use HeapFree to free the pointer returned from this function.
**
** Inputs:
**  TCHAR* format           - the format string
**  SYSTEMTIME* time        - the time to convert
**
** Returns:
**  TCHAR* - Newly allocated string with the formatted date / time
*******************************************************************/
TCHAR* AllocateFullDateTime(TCHAR* format, SYSTEMTIME* time)
{
    TCHAR* full_buffer;
    size_t required_size = FormatDateTime(format, NULL, 0, time);

    full_buffer = (TCHAR*) HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY, sizeof(TCHAR) * required_size);

    FormatDateTime(format, full_buffer, required_size, time);

    return full_buffer;
}


/*******************************************************************
** FormatDateTime
** ==============
** Combines the Windows APIs GetDateFormat and GetTimeFormat.  This
** function converts a date and time stored in a SYSTEMTIME
** structure to a human-readable string, according a format string.
**
** The format string may contain both date and time format
** characters, e.g. "dd-MMM-yyy HH:mm:ss".  Other characters are
** copied to the output unchanged (quotes are not required like
** they are with GetDateFormat).
**
** Inputs:
**  TCHAR* format           - the format string
**  TCHAR* buffer           - buffer to hold the output. If NULL,
**                            the function will still calculate the
**                            required buffer size.
**  size_t buffer_size      - size of the output buffer
**  SYSTEMTIME* time        - the time to convert
**
** Outputs:
**  size_t  - the number of characters needed to format the
**            entire string.
*******************************************************************/
size_t FormatDateTime(TCHAR* format, TCHAR* buffer,
    size_t buffer_size, SYSTEMTIME* time)
{
    size_t required_size = 1;

    if(format)
    {
        TCHAR* src = format;
        TCHAR* dst = buffer;
        size_t format_length = _tcslen(format);
        size_t remaining_space;
        BOOL escape_next_char = FALSE;

        TCHAR current;

        while(src < format + format_length)
        {
            remaining_space = buffer_size - (dst - buffer);
            current = *src;

            if(!escape_next_char && IsFormattingChar(current))
            {
                TCHAR temp_format_buffer[MAX_FORMAT_WORD_SIZE+1];
                TCHAR* next = src;
                size_t word_length;
                size_t format_chunk_length;

                while(*next == current)
                {
                    ++next;
                }

                format_chunk_length = next - src;
                if(format_chunk_length > MAX_FORMAT_WORD_SIZE)
                {
                    format_chunk_length = MAX_FORMAT_WORD_SIZE;
                }

                _tcsncpy_s(temp_format_buffer, MAX_FORMAT_WORD_SIZE + 1,
                           src, format_chunk_length);
                temp_format_buffer[format_chunk_length] = _T('\0');

                if(IsDateChar(current))
                {
                    word_length = GetDateFormat(LOCALE_USER_DEFAULT,
                        0, time, temp_format_buffer, NULL, 0) - 1;

                    if(dst && (word_length < remaining_space))
                    {
                        GetDateFormat(LOCALE_USER_DEFAULT, 0, time,
                            temp_format_buffer, dst,
                            buffer_size - (dst - buffer));

                        dst += word_length;
                    }
                }
                else
                {
                    word_length = GetTimeFormat(LOCALE_USER_DEFAULT,
                        0, time, temp_format_buffer, NULL, 0) - 1;

                    if(dst && (word_length < remaining_space))
                    {
                        GetTimeFormat(LOCALE_USER_DEFAULT, 0, time,
                            temp_format_buffer, dst,
                            buffer_size - (dst - buffer));

                        dst += word_length;
                    }
                }

                required_size += word_length;
                src = next;
            }
            else if(!escape_next_char && IsEscapeChar(current))
            {
                escape_next_char = TRUE;
                ++src;
            }
            else
            {
                escape_next_char = FALSE;

                if(dst && (remaining_space > 1))
                {
                    *dst = current;
                    ++dst;
                }

                ++required_size;
                ++src;
            }
        }

        if(dst && (dst < buffer + buffer_size))
        {
            *dst = _T('\0');
        }
    }

    return required_size;
}






