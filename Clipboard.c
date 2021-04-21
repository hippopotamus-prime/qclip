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
#define _WIN32_WINNT 0x0600

#ifndef CF_DIBV5
#define CF_DIBV5 17     //MinGW's includes leave this out for some reason...
#endif

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <shlobj.h>
#include "Clipboard.h"
#include "QClip.h"
#include "resource.h"

#define MENU_BMP_HEIGHT 100

static BOOL IsFormatSupported(UINT format);
static BOOL IsShellFormat(UINT format);
static HBITMAP CreateBitmapFromClipboard(BYTE* memory);


/*******************************************************************
** AddClipItemToMenu
** =================
** Generates a text or image description of a ClipItem and appends
** it to the end of the given menu.  Descriptions of some formats
** may be rather vague.
**
** Inputs:
**      ClipItem* item      - address of the item to describe
**      HMENU menu          - the menu to append to
**      int item_id         - ID to apply to the new menu item
**      TCHAR* prefix       - pointer to a character to use as an
**                            accelerator for the new menu item
**                            (text items only); may be NULL
**
** Outputs:
**      BOOL                - TRUE on success
*******************************************************************/
BOOL AddClipItemToMenu(ClipItem* item, HMENU menu, int item_id, TCHAR* prefix)
{
    BOOL success = FALSE;

    if(item && item->data)
    {
        MENUITEMINFO mii;
        unsigned int i;
        int format = -1;
        TCHAR text[POPUP_TEXT_LENGTH + 4] = _T("");
        TCHAR* text_start = text;

        if(prefix)
        {
            _stprintf_s(text, 4, _T("&%c "), *prefix);
            text_start += 3;
        }

        ZeroMemory(&mii, sizeof(MENUITEMINFO));
        mii.cbSize  = sizeof(MENUITEMINFO);
        mii.fMask   = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_ID;
        mii.fState  = MFS_ENABLED;
        mii.wID     = item_id;
        mii.fType   = MFT_STRING;

        for(i = 0; i < item->formats; ++i)
        {           
            if((item->data[i].format == CF_TEXT)
            || (item->data[i].format == CF_UNICODETEXT)
            || (item->data[i].format == CF_DIB)
            || (item->data[i].format == CF_DIBV5)
            || (item->data[i].format == CF_HDROP))
            {
                format = item->data[i].format;
                break;
            }
        }

        switch(format)
        {
            case CF_UNICODETEXT:
                #ifdef UNICODE
                _tcscpy_s(text_start, POPUP_TEXT_LENGTH, item->data[i].memory);
                #else
                WideCharToMultiByte(CP_ACP, 0, item->data[i].memory,
                    -1, text_start, POPUP_TEXT_LENGTH, NULL, NULL);
                #endif

                mii.dwTypeData      = text;
                mii.cch             = (UINT) _tcslen(text);
                break;

            case CF_TEXT:
                #ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, item->data[i].memory,
                    -1, text_start, POPUP_TEXT_LENGTH, NULL, NULL);
                #else
                _tcscpy_s(text_start, POPUP_TEXT_LENGTH, item->data[i].memory);
                #endif

                mii.dwTypeData      = text;
                mii.cch             = (UINT) _tcslen(text);
                break;

            case CF_HDROP:
            {
                DROPFILES* drop = (DROPFILES*) item->data[i].memory;
                TCHAR file_text[POPUP_TEXT_LENGTH + 1];

                int length;
                int count = 0;

                LoadString(GetModuleHandle(NULL),
                    STRING_FILE_POPUP,
                    file_text,
                    POPUP_TEXT_LENGTH);

                if(drop->fWide)
                {
                    wchar_t* names = (wchar_t*)(((BYTE*) drop) + drop->pFiles);

                    while((length = wcslen(names)) > 0)
                    {
                        ++count;
                        names += length + 1;
                    }
                }
                else
                {
                    char* names = (char*)(((BYTE*) drop) + drop->pFiles);

                    while((length = strlen(names)) > 0)
                    {
                        ++count;
                        names += length + 1;
                    }
                }

                _stprintf_s(text_start, POPUP_TEXT_LENGTH,
                    file_text, count);

                mii.dwTypeData      = text;
                mii.cch             = (UINT) _tcslen(text);
                break;
            }

            case CF_DIBV5:
            case CF_DIB:
                if(gv.settings.preview_bitmaps)
                {
                    // The Windows API documentation claims that using
                    // MFT_BITMAP and dwTypeData is equivalent to hbmpItem,
                    // but they're rendered differently. MFT_BITMAP is inline
                    // with text items while hbmpItem is a separate column on
                    // the left. In Win98/XP MFT_BITMAP looked better, but
                    // Win10 stretches it match the menu width, which looks
                    // really bad.

                    /*
                    mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
                    mii.fType       = MFT_BITMAP;
                    mii.dwTypeData  = (TCHAR*)
                        CreateBitmapFromClipboard(item->data[i].memory);
                    */
                    
                    mii.fMask |= MIIM_BITMAP;
                    mii.hbmpItem =
                        CreateBitmapFromClipboard(item->data[i].memory);
                }

                BITMAPINFOHEADER* header = (BITMAPINFOHEADER*)
                    item->data[i].memory;
                TCHAR bmp_text[POPUP_TEXT_LENGTH + 1];

                LoadString(GetModuleHandle(NULL),
                    STRING_BITMAP_POPUP,
                    bmp_text,
                    POPUP_TEXT_LENGTH);

                _stprintf_s(text_start, POPUP_TEXT_LENGTH,
                    bmp_text,
                    header->biWidth,
                    header->biHeight,
                    header->biBitCount);

                mii.dwTypeData      = text;
                mii.cch             = (UINT) _tcslen(text);
                break;

            default:
                LoadString(GetModuleHandle(NULL),
                    STRING_DEFAULT_POPUP,
                    text_start,
                    POPUP_TEXT_LENGTH);

                mii.dwTypeData      = text;
                mii.cch             = (UINT) _tcslen(text);
                break;
        }
 
        success = InsertMenuItem(menu, GetMenuItemCount(menu),
            TRUE, &mii);
    }

    return success;
}


/*******************************************************************
** CreateBitmapFromClipboard
** =========================
** Generates a bitmap in memory based on data from the Windows
** clipboard.  Data must be in either CF_DIB or CF_DIBV5 format.
** The bitmap is scaled to be no taller than MENU_BMP_HEIGHT.
**
** Inputs:
**      void* memory    - pointer to raw clipboard data
**
** Outputs:
**      HBITMAP         - handle of the newly created bitmap
*******************************************************************/
HBITMAP CreateBitmapFromClipboard(BYTE* memory)
{
    BITMAPINFO* info = (BITMAPINFO*) memory;

    BYTE* bits = memory + info->bmiHeader.biSize +
        (info->bmiHeader.biClrUsed) * sizeof(RGBQUAD);

    HDC display_dc = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    //HDC bmp_dc = CreateCompatibleDC(display_dc);
    HDC scaled_dc = CreateCompatibleDC(display_dc);

    int width, height;
    //HGDIOBJ original1;
    HGDIOBJ sdc_original;
    //HBITMAP bmp;
    HBITMAP scaled_bmp;

    if(info->bmiHeader.biHeight > MENU_BMP_HEIGHT)
    {
        height = MENU_BMP_HEIGHT;
        width = info->bmiHeader.biWidth *
            MENU_BMP_HEIGHT / info->bmiHeader.biHeight;
    }
    else if(info->bmiHeader.biHeight < -MENU_BMP_HEIGHT)
    {
        height = -MENU_BMP_HEIGHT;
        width = info->bmiHeader.biWidth *
            (-MENU_BMP_HEIGHT) / info->bmiHeader.biHeight;
    }
    else 
    {
        height = info->bmiHeader.biHeight;
        width = info->bmiHeader.biWidth;
    }

    //bmp = CreateDIBitmap(display_dc, &info->bmiHeader,
    //    CBM_INIT, bits, info, DIB_RGB_COLORS);
    //original1 = SelectObject(bmp_dc, bmp);

    scaled_bmp = CreateCompatibleBitmap(display_dc, width, height);
    sdc_original = SelectObject(scaled_dc, scaled_bmp);

    //The default stretching mode looks like crap when using
    //this method.  Oddly, when using StretchBlt (i.e. the
    //commented out stuff), it seems impossible to NOT
    //use HALFTONE on some systems.  Driver issue maybe...
    SetStretchBltMode(scaled_dc, HALFTONE);
    SetBrushOrgEx(scaled_dc, 0, 0, NULL);

    StretchDIBits(scaled_dc, 0, 0, width, height, 0, 0,
        info->bmiHeader.biWidth, info->bmiHeader.biHeight,
        bits, info, DIB_RGB_COLORS, SRCCOPY);

    //StretchBlt(scaled_dc, 0, 0, width, height, bmp_dc, 0, 0,
    //    info->bmiHeader.biWidth, info->bmiHeader.biHeight,
    //    SRCCOPY);

    SelectObject(scaled_dc, sdc_original);
    DeleteDC(scaled_dc);
    //SelectObject(bmp_dc, original1);
    //DeleteDC(bmp_dc);
    DeleteDC(display_dc);
    //DeleteObject(bmp);

    return scaled_bmp;
}



/*******************************************************************
** CopyToClipboard
** ===============
** Copies data from a ClipItem structure to the clipboard in all
** available formats.
**
** Inputs:
**      ClipItem* item      - structure to be copied
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int CopyToClipboard(ClipItem* item)
{
    unsigned int successes = 0;

    if(item && item->data && (item->formats != 0)
    && OpenClipboard(gv.main_window))
    {
        if(EmptyClipboard())
        {
            unsigned int i;
            HGLOBAL clipboard_handle;
            void* clipboard_pointer;

            for(i = 0; i < item->formats; ++i)
            {
                if(item->data[i].memory)
                {
                    clipboard_handle = GlobalAlloc(
                        GMEM_MOVEABLE | GMEM_DDESHARE,
                        item->data[i].size);

                    if(clipboard_handle)
                    {
                        clipboard_pointer = GlobalLock(clipboard_handle);

                        if(clipboard_pointer)
                        {
                            CopyMemory(clipboard_pointer,
                                item->data[i].memory,
                                item->data[i].size);

                            GlobalUnlock(clipboard_pointer);

                            SetClipboardData(item->data[i].format,
                                clipboard_handle);

                            ++successes;
                        }
                    }
                }
            }
        }

        if(successes)
        {
            //Calling CloseClipboard will send further
            //WM_DRAWCLIPBOARD messages (the function
            //returns /after/ they're processed).  We
            //want to ignore these.
            ++gv.ignore;
        }

        CloseClipboard();
    }

    return successes;
}



/*******************************************************************
** CopyStringToClipboard
** =====================
** Specialized function for copying text to the Windows clipboard.
** Used for common items (date, etc).
**
** Inputs:
**      TCHAR* text         - text to copy.
**
** Outputs:
**      BOOL                - TRUE on success.
*******************************************************************/
BOOL CopyStringToClipboard(TCHAR* text)
{
    BOOL success = FALSE;

    if(text && OpenClipboard(gv.main_window))
    {
        int text_length = (int) _tcslen(text);

        if(EmptyClipboard() && (text_length > 0))
        {
            HGLOBAL clipboard_handle;
            TCHAR* clipboard_pointer;

            clipboard_handle = GlobalAlloc(
                GMEM_MOVEABLE | GMEM_DDESHARE,
                (text_length + 1) * sizeof(TCHAR));

            if(clipboard_handle)
            {
                clipboard_pointer = (TCHAR*) GlobalLock(clipboard_handle);

                if(clipboard_pointer)
                {
                    _tcscpy(clipboard_pointer, text);

                    GlobalUnlock(clipboard_pointer);

                    #ifdef UNICODE
                    SetClipboardData(CF_UNICODETEXT, clipboard_handle);
                    #else
                    SetClipboardData(CF_TEXT, clipboard_handle);
                    #endif

                    success = TRUE;
                }
            }
        }

        if(success)
        {
            ++gv.ignore;
        }

        CloseClipboard();
    }

    return success;
}



/*******************************************************************
** DestroyClipItem
** ===============
** Releases memory allocated by PopulateClipItem.  This function
** will check if data has actually been allocated, so calling it
** on an empty ClipItem is not an error.
**
** Inputs:
**      ClipItem* item      - structure to be deallocated
*******************************************************************/
void DestroyClipItem(ClipItem* item)
{
    if(item)
    {
        if(item->data)
        {
            unsigned int i;
        
            for(i = 0; i < item->formats; ++i)
            {
                if(item->data[i].memory)
                {
                    HeapFree(GetProcessHeap(), 0,
                        item->data[i].memory);
                }
            }

            HeapFree(GetProcessHeap(), 0, item->data);
            item->data = NULL;
        }
    
        item->formats = 0;
    }
}



/*******************************************************************
** PopulateClipItem
** ================
** Copies data from the clipboard to a ClipItem structure in all
** available formats.  Memory is dynamically allocated in the
** process, so be sure to call DestroyClipItem when finished.
**
** Inputs:
**      ClipItem* item      - structure to be populated
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int PopulateClipItem(ClipItem* item)
{
    item->formats = 0;

    if(OpenClipboard(gv.main_window))
    {
        //First, get an estimate of how many different
        //formats we'll need to store.  Some of these
        //will almost certainly fail...
        unsigned int clipboard_max_size = CountClipboardFormats();
    
        if(clipboard_max_size > 0)
        {
            //Allocate an array of pointers and meta-info
            //for each potential clipboard format.
            item->data = (ClipData*) HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, sizeof(ClipData) * clipboard_max_size);

            if(item->data)
            {
                UINT format = EnumClipboardFormats(0);
                HANDLE clipboard_handle;
                void* clipboard_pointer;
                void* mem_pointer;
                size_t data_size;

                //Now we'll try to get each format off the clipboard
                //and store it, stopping when we either run out of
                //space, or when we run out of formats.
                while(format && (item->formats < clipboard_max_size))
                {
                    if(IsFormatSupported(format))
                    {
                        clipboard_handle = GetClipboardData(format);
        
                        if(clipboard_handle)
                        {
                            //This will fail for CF_BITMAPs - they're
                            //not HGLOBAL handles.
                            clipboard_pointer = GlobalLock(clipboard_handle);
             
                            if(clipboard_pointer)
                            {
                                //We've successfully accessed one data format
                                //on the clipboard, so now we'll allocate
                                //some memory to make a copy of it.
                                data_size = GlobalSize(clipboard_pointer);
                                mem_pointer = HeapAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY, data_size);
        
                                if(mem_pointer)
                                {
                                    CopyMemory(mem_pointer,
                                        clipboard_pointer,
                                        data_size);
            
                                    item->data[item->formats].memory =
                                        mem_pointer;  
                                    item->data[item->formats].format = format;
                                    item->data[item->formats].size = data_size;
            
                                    ++(item->formats);
                                }
                   
                                GlobalUnlock(clipboard_pointer);
                            }
                        }

                    }

                    format = EnumClipboardFormats(format);
                }

                //If we did not successfully store any clipboard
                //data, we might as well just forget the whole
                //thing...
                if(item->formats == 0)
                {
                    HeapFree(GetProcessHeap(), 0, item->data);
                    item->data = NULL;
                }
            }
        }

        CloseClipboard();
    }

    return item->formats;
}


/*******************************************************************
** IsShellFormat
** =============
** Determines if a given clipboard format is one of several
** application-defined formats known to be used by the Windows
** Explorer shell.  These are necessary for some file cut & paste
** operations, and so should be allowed if CF_HDROP is enabled.
**
** Inputs:
**      UINT format         - the clipboard format in question
**
** Outputs:
**      BOOL                - TRUE if format is a shell format
*******************************************************************/
BOOL IsShellFormat(UINT format)
{
    BOOL shell_format = FALSE;
    unsigned int i;

    for(i = 0; !shell_format && (i < NUM_SHELL_FORMATS); ++i)
    {
        shell_format = (format == gv.shell_formats[i]);
    }

    return shell_format;
}


/*******************************************************************
** IsFormatSupported
** =================
** Determines if a given clipboard format should be saved in the
** queue, based on user settings and known redundancies in the
** standard formats.
**
** Inputs:
**      UINT format         - the clipboard format in question
**
** Outputs:
**      BOOL                - TRUE if the format should be copied
*******************************************************************/
BOOL IsFormatSupported(UINT format)
{
    BOOL supported = FALSE;

    //CF_BITMAP is always redundant with CF_DIB and CF_DIBV5,
    //so we discard it.  CF_BITMAP data is also allocated
    //differently and cannot be handled by the current code.

    //CF_PALETTE is only useful in combination with CF_BITMAP;
    //CF_DIB[V5] contains its own palette information.

    //CF_OWNERDISPLAY is useless to us, as no data is actually
    //placed on the clipboard for it.

    //CF_OEMTEXT converts to CF_[UNICODE]TEXT  ...I think...
    //without loss of data.

    //Also ...I think... CF_METAFILEPICT converts to
    //CF_ENHMETAFILE without data loss.

    if((format != CF_BITMAP)
    && (format != CF_PALETTE)
    && (format != CF_OWNERDISPLAY)
    && (format != CF_OEMTEXT)
    && (format != CF_METAFILEPICT))
    {
        if(gv.settings.enable_all_formats)
        {
            supported = TRUE;
        }
        else if(((format >= CF_PRIVATEFIRST) && (format <= CF_PRIVATELAST))
        || ((format >= CF_GDIOBJFIRST) && (format <= CF_GDIOBJLAST)))
        {
            supported = gv.settings.format_flags & FORMAT_PRIVATE;
        }
        else if(format >= 0x0C000)
        {
            if(gv.settings.format_flags & FORMAT_REGISTERED)
            {
                supported = TRUE;
            }
            else if(gv.settings.format_flags & FORMAT_FILE)
            {
                supported = IsShellFormat(format);
            }
        }
        else
        {
            switch(format)
            {
                //TIFFs are lumped together with bitmaps, the logic
                //being that if users wants one, they probably want
                //the other.  These maybe should go under "other
                //standard formats" instead...
                case CF_TIFF:
                   supported = gv.settings.format_flags & FORMAT_BITMAP;
                   break;

                //Windows will automatically convert between CF_DIB
                //and CF_DIBV5, but the conversion is buggy.  We
                //want to only support whichever one is available
                //"natively", so that Windows never does any conversion.
                case CF_DIB:
                case CF_DIBV5:
                {
                    UINT dib_formats[] = {CF_DIB, CF_DIBV5};
                    int native_format = GetPriorityClipboardFormat(
                        dib_formats, 2);

                    supported = (format == native_format)
                        && (gv.settings.format_flags & FORMAT_BITMAP);
                    break;
                }

                case CF_UNICODETEXT:
                    supported = gv.settings.format_flags & FORMAT_TEXT;
                    break;

                //CF_TEXT is redundant with CF_UNICODETEXT, but the
                //latter is not available on all systems, so we enable
                //CF_TEXT only in the case when CF_UNICODETEXT is
                //unavailable.
                case CF_TEXT:
                    supported = (gv.settings.format_flags & FORMAT_TEXT)
                        && !IsClipboardFormatAvailable(CF_UNICODETEXT);
                    break;

                //CF_LOCALE is only useful in combination with CF_TEXT,
                //so we apply the same rules.
                case CF_LOCALE:
                    supported = (gv.settings.format_flags & FORMAT_TEXT)
                        && !IsClipboardFormatAvailable(CF_UNICODETEXT);
                    break;                    

                case CF_HDROP:
                    supported = gv.settings.format_flags & FORMAT_FILE;
                    break;

                case CF_ENHMETAFILE:
                    supported = gv.settings.format_flags & FORMAT_META;
                    break;

                //See comment on TIFFs
                case CF_RIFF:
                case CF_WAVE:
                    supported = gv.settings.format_flags & FORMAT_WAVE;
                    break;

                default:
                    supported = gv.settings.format_flags & FORMAT_OTHERS;
            }
        }
    }

    return supported;
}


/*******************************************************************
** CompareClipItems
** ================
** Compares two ClipItems, returns TRUE if they're identical.
**
** Inputs:
**      ClipItem* item1
**      ClipItem* item2
**
** Outputs:
**      BOOL - TRUE if the ClipItems are identical
*******************************************************************/
BOOL CompareClipItems(ClipItem* item1, ClipItem* item2)
{
    BOOL identical = FALSE;

    if(!item1 && !item2)
    {
        identical = TRUE;
    }
    else if(item1 && item2)
    {
        if(item1->formats == item2->formats)
        {
            if(item1->formats == 0)
            {
                identical = TRUE;
            }
            else if(item1->data && item2->data)
            {
                BOOL maybe_identical = TRUE;
                unsigned int i;

                for(i = 0; (i < item1->formats) && maybe_identical; ++i)
                {
                    if((item1->data[i].format == item2->data[i].format)
                    && (item1->data[i].size == item2->data[i].size))
                    {
                        maybe_identical = (memcmp(
                            item1->data[i].memory,
                            item2->data[i].memory,
                            item1->data[i].size) == 0);
                    }
                    else
                    {
                        maybe_identical = FALSE;
                    }
                }

                identical = maybe_identical;
            }
        }
    }

    return identical;
}
