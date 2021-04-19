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

#include <windows.h>
#include <tchar.h>
#include "Clipboard.h"
#include "ClipQueue.h"
#include "ClipFile.h"
#include "QClip.h"
#include "RecentFiles.h"
#include "resource.h"

#define DEFAULT_SAVE_FILE   _T("autosave.qcl")

#define DATA_SIGNATURE      0x0abcd1234
#define ITEM_SIGNATURE      0x06789f5d4
#define FILE_SIGNATURE      0x02001ad00

#define FILE_VERSION        0

#define FORMAT_NAME_MAX     512

typedef struct
{
    unsigned int signature;
    unsigned int format;
    unsigned int size;
    unsigned int name_length;       //length in bytes, including terminator
    unsigned int reserved1;
}ClipDataHeader;

typedef struct
{
    unsigned int signature;
    unsigned int formats;
    unsigned int reserved1;
    unsigned int reserved2;
    unsigned int reserved3;
}ClipItemHeader;

typedef struct
{
    unsigned int signature;
    unsigned int version;
    unsigned int items;
    unsigned int reserved1;
    unsigned int reserved2;
    unsigned int reserved3;
    unsigned int reserved4;
    unsigned int reserved5;
    unsigned int reserved6;
}ClipFileHeader;


/*******************************************************************
** LoadQueueFromFile
** =================
** Loads a clipboard queue from an already opened .qcl file.  This
** function will allocate memory for the queue, so be sure to call
** DestroyQueue when finished.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to populate.
**      HANDLE fhand            - handle to a .qcl file open for
**                                reading
**
** Outputs:
**      BOOL                    - TRUE if loading succeeded.  If
**                                loading fails, memory will be
**                                cleaned up automatically.
*******************************************************************/
BOOL LoadQueueFromFile(ClipQueue* cq, HANDLE fhand)
{
    ClipFileHeader file_header;
    DWORD num_bytes;
    BOOL fail;
    wchar_t name[FORMAT_NAME_MAX+1];

    #ifndef UNICODE
    char ascii_name[FORMAT_NAME_MAX+1];
    #endif

    //First, read the file header - this will tell us
    //how many ClipItems are in this queue.
    fail = !ReadFile(fhand, &file_header,
        sizeof(ClipFileHeader), &num_bytes, NULL)
        || (num_bytes != sizeof(ClipFileHeader))
        || (file_header.signature != FILE_SIGNATURE);

    if(!fail)   //Got the file header successfully
    {
        ClipItemHeader item_header;
        ClipDataHeader data_header;
        unsigned int i, j;

        if(file_header.items < gv.settings.queue_size)
        {
            fail = !CreateQueue(cq, gv.settings.queue_size);
        }
        else
        {
            fail = !CreateQueue(cq, file_header.items);
        }

        //Now, each ClipItem has its own header, telling
        //how many individual formats are contained in it.
        for(i = 0; (i < file_header.items) && !fail; ++i)
        {
            fail = !ReadFile(fhand, &item_header,
                sizeof(ClipItemHeader), &num_bytes, NULL)
                || (num_bytes != sizeof(ClipItemHeader))
                || (item_header.signature != ITEM_SIGNATURE);

            if(!fail)   //Got the item header successfully
            {
                cq->clips[i].formats = item_header.formats;
                cq->clips[i].data = (ClipData*) HeapAlloc(GetProcessHeap(),
                    HEAP_ZERO_MEMORY, sizeof(ClipData) * item_header.formats);

                fail = (cq->clips[i].data == NULL);

                //Then there is another header for each format,
                //telling exactly how much data there is.
                //BUT, the data will be a little different for
                //standard formats and registered formats;
                //registered formats need a string to describe
                //them, which will precede the data.
                for(j = 0; (j < item_header.formats) && !fail; ++j)
                {
                    fail = !ReadFile(fhand, &data_header,
                        sizeof(ClipDataHeader), &num_bytes, NULL)
                        || (num_bytes != sizeof(ClipDataHeader))
                        || (data_header.signature != DATA_SIGNATURE)
                        || (data_header.name_length > FORMAT_NAME_MAX);

                    if(!fail)   //Got the data header successfully
                    {
                        if(IsAppFormat(data_header.format))
                        {
                            //In this case we've encountered a registered
                            //application format, which is identified by a
                            //string instead of a number (the number can
                            //change between sessions).  This means we have
                            //to query the system for the current number...

                            //Note the name is always stored in Unicode.

                            fail = !ReadFile(fhand, name,
                                data_header.name_length, &num_bytes, NULL)
                                || (num_bytes != data_header.name_length);

                            if(!fail)
                            {
                                name[data_header.name_length] = L'\0';

                                #ifdef UNICODE
                                cq->clips[i].data[j].format =
                                    RegisterClipboardFormat(name);

                                fail = (cq->clips[i].data[j].format == 0);

                                #else
                                WideCharToMultiByte(CP_ACP, 0, name, -1,
                                    ascii_name, FORMAT_NAME_MAX, NULL, NULL);

                                cq->clips[i].data[j].format =
                                    RegisterClipboardFormat(ascii_name);

                                fail = (cq->clips[i].data[j].format == 0);
                                #endif
                            }
                        }
                        else
                        {
                            cq->clips[i].data[j].format = data_header.format;
                        }

                        if(!fail)
                        {
                            cq->clips[i].data[j].size = data_header.size;

                            cq->clips[i].data[j].memory = HeapAlloc(
                                GetProcessHeap(), HEAP_ZERO_MEMORY,
                                data_header.size);

                            fail = (cq->clips[i].data[j].memory == NULL);

                            if(!fail)
                            {
                                fail = !ReadFile(fhand,
                                    cq->clips[i].data[j].memory,
                                    data_header.size, &num_bytes, NULL)
                                    || (num_bytes != data_header.size);
                            }
                        }
                    }
                }
            }
        }
    }

    if(fail)
    {
        DestroyQueue(cq);
    }
    else
    {
        cq->count = file_header.items;
    }

    return !fail;
}


/*******************************************************************
** SaveQueueToFile
** ===============
** Stores a clipboard queue in an already opened .qcl file.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to store.
**      HANDLE fhand            - handle to a .qcl file open for
**                                writing.
**
** Outputs:
**      BOOL                    - TRUE if saving succeeded.
*******************************************************************/
BOOL SaveQueueToFile(ClipQueue* cq, HANDLE fhand)
{
    ClipFileHeader file_header;
    DWORD num_bytes;
    BOOL fail;
    wchar_t name[FORMAT_NAME_MAX+1];

    #ifndef UNICODE
    char ascii_name[FORMAT_NAME_MAX+1];
    #endif

    ClipItemHeader item_header;
    ClipDataHeader data_header;
    ClipItem* item;
    unsigned int i, j;

    ZeroMemory(&file_header, sizeof(ClipFileHeader));
    file_header.signature = FILE_SIGNATURE;
    file_header.version = FILE_VERSION;
    file_header.items = GetQueueLength(cq);

    ZeroMemory(&item_header, sizeof(ClipItemHeader));
    ZeroMemory(&data_header, sizeof(ClipDataHeader));

    fail = !WriteFile(fhand, &file_header,
        sizeof(ClipFileHeader), &num_bytes, NULL);

    for(i = 0; (i < file_header.items) && !fail; ++i)
    {
        item = GetItem(cq, i);

        fail = (item == NULL);

        if(!fail)
        {
            item_header.signature = ITEM_SIGNATURE;
            item_header.formats = item->formats;

            fail = (item->data == NULL)
                ||!WriteFile(fhand, &item_header,
                    sizeof(ClipItemHeader), &num_bytes, NULL);

            for(j = 0; (j < item_header.formats) && !fail; ++j)
            {
                data_header.signature = DATA_SIGNATURE;
                data_header.format = item->data[j].format;

                //The size value saved to disk is 32-bit.  This
                //may cause problems...
				data_header.size = (unsigned int) item->data[j].size;

                if(IsAppFormat(data_header.format))
                {
                    #ifdef UNICODE
                    fail = (GetClipboardFormatName(data_header.format,
                        name, FORMAT_NAME_MAX) == 0);

                    if(!fail)
                    {
                        data_header.name_length =
                            (_tcslen(name) + 1) * sizeof(wchar_t);

                    #else
                    fail = (GetClipboardFormatName(data_header.format,
                        ascii_name, FORMAT_NAME_MAX) == 0);

                    if(!fail)
                    {
                        MultiByteToWideChar(CP_ACP, 0, ascii_name, -1,
                            name, FORMAT_NAME_MAX);

                        data_header.name_length =
                            (strlen(ascii_name) + 1) * sizeof(wchar_t);

                    #endif

                        fail = (item->data[j].memory == NULL)
                            || !WriteFile(fhand, &data_header,
                                sizeof(ClipDataHeader), &num_bytes, NULL)
                            || !WriteFile(fhand, name,
                                data_header.name_length, &num_bytes, NULL)
                            || !WriteFile(fhand, item->data[j].memory,
                                data_header.size, &num_bytes, NULL);
                    }
                }
                else
                {
                    data_header.name_length = 0;

                    fail = (item->data[j].memory == NULL)
                        || !WriteFile(fhand, &data_header,
                            sizeof(ClipDataHeader), &num_bytes, NULL)
                        || !WriteFile(fhand, item->data[j].memory,
                            data_header.size, &num_bytes, NULL);
                }
            }
        }
    }

    return !fail;
}


/*******************************************************************
** LoadFilterString
** ================
** Loads the filter text (i.e. "QClip Clipboards (.qcl)") that
** appears in all the file prompts.  This function exists
** because LoadString cannot load strings with '\0' in them
** (i.e. multiple null terminators).  Then why are they used
** throughout the API? Why?! WHY?! WHY?!!!!! *dies*
**
** Inputs:
**      TCHAR* buffer   - buffer to hold the string - must be at
**                        least FILTER_STRING_LENGTH
*******************************************************************/
void LoadFilterString(TCHAR* buffer)
{
    TCHAR* separator;

    LoadString(GetModuleHandle(NULL),
        STRING_TYPE_FILTER,
        buffer,
        TYPE_FILTER_LENGTH);

    separator = buffer;

    while(separator != NULL)
    {
        separator = _tcschr(separator, _T(';'));

        if(separator != NULL)
        {
            *separator = _T('\0');
            ++separator;
        }
    }

}


/*******************************************************************
** OpenQueue
** =========
** Prompts the user for a .qcl file and loads the clipboard
** queue from it.  The previous queue is destroyed in the process.
** This function will allocate memory for the queue, so be sure
** to call DestroyQueue when finished.
**
** Outputs:
**      BOOL                    - TRUE if loading succeeded.  If
**                                loading fails, memory will be
**                                cleaned up automatically.
*******************************************************************/
BOOL OpenQueue()
{
    BOOL success = FALSE;
    OPENFILENAME ofn;
    TCHAR file_name[MAX_PATH] = _T("");
	TCHAR type_filter[TYPE_FILTER_LENGTH+1];

    LoadFilterString(type_filter);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.lpstrFilter     = type_filter;
    ofn.lpstrFile       = file_name;
    ofn.nMaxFile        = MAX_PATH;
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = FILE_TYPE;

    if(GetRecentCount() > 0)
    {
        ofn.lpstrInitialDir = GetRecentFileName(0);
    }

	if(GetOpenFileName(&ofn))
	{
		HANDLE fhand = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if(fhand != INVALID_HANDLE_VALUE)
		{
            ClipQueue cq;

            if(LoadQueueFromFile(&cq, fhand))
            {
                TCHAR* relative_file_name = MakeRelativePath(file_name);

                AddRecentFile(relative_file_name);
                DestroyQueue(&gv.cq);
                gv.cq = cq;
                gv.cq.modified = FALSE;
                success = TRUE;
                gv.opened_file = TRUE;
            }

			CloseHandle(fhand);
		}

        if(!success)
        {
            ShowErrorMessage(STRING_ERROR_OPEN_FILE);
        }
	}

    return success;
}


/*******************************************************************
** SaveQueue
** =========
** Saves the clipboard queue under the most recently used file name,
** provided a file was accessed this session.  Otherwise, prompts
** the user for a file name.
**
** Outputs:
**      BOOL                    - TRUE on success.
*******************************************************************/
BOOL SaveQueue()
{
    BOOL success = FALSE;

    if(gv.opened_file)
    {
        HANDLE fhand = CreateFile(GetRecentFileName(0),
            GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if(fhand != INVALID_HANDLE_VALUE)
        {
            if(SaveQueueToFile(&gv.cq, fhand))
            {
                success = TRUE;
                gv.cq.modified = FALSE;
            }
            CloseHandle(fhand);
        }
    }
    else
    {
        success = SaveQueueAs();
    }

    return success;
}


/*******************************************************************
** SaveQueueAs
** ===========
** Prompts the user for a .qcl file and saves the clipboard
** queue to it.
**
** Outputs:
**      BOOL                    - TRUE on success.
*******************************************************************/
BOOL SaveQueueAs()
{
    BOOL success = FALSE;
    OPENFILENAME ofn;
    TCHAR file_name[MAX_PATH] = _T("");
	TCHAR type_filter[TYPE_FILTER_LENGTH+1];

    LoadFilterString(type_filter);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.lpstrFilter     = type_filter;
    ofn.lpstrFile       = file_name;
    ofn.nMaxFile        = MAX_PATH;
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = FILE_TYPE;

    if(GetRecentCount() > 0)
    {
        ofn.lpstrInitialDir = GetRecentFileName(0);
    }    

    if(GetSaveFileName(&ofn))
    {
        HANDLE fhand = CreateFile(file_name, GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if(fhand != INVALID_HANDLE_VALUE)
        {
            if(SaveQueueToFile(&gv.cq, fhand))
            {
                TCHAR* relative_file_name = MakeRelativePath(file_name);

                AddRecentFile(relative_file_name);
                success = TRUE;
                gv.opened_file = TRUE;
                gv.cq.modified = FALSE;
            }
            CloseHandle(fhand);
        }
    }
    
    return success;
}


/*******************************************************************
** OpenQueueFromDefault
** ====================
** Loads the clipboard queue from the autosave file.  This will
** exist if the user has the appropriate option checked under
** general settings.  This function will allocate memory for the
** queue, so be sure to call DestroyQueue when finished.
**
** Outputs:
**      BOOL                    - TRUE on success.
*******************************************************************/
BOOL OpenQueueFromDefault()
{
    BOOL success = FALSE;
    TCHAR file_path[MAX_PATH];
    HANDLE fhand;

    GetFileInInstallPath(DEFAULT_SAVE_FILE, file_path);

    fhand = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(fhand != INVALID_HANDLE_VALUE)
    {
        ClipQueue cq;

        if(LoadQueueFromFile(&cq, fhand))
        {
            DestroyQueue(&gv.cq);
            gv.cq = cq;
            success = TRUE;

            //The queue is modified from the last "real"
            //save file (which may not exist at this point).
            gv.cq.modified = TRUE;
        }

		CloseHandle(fhand);
    }

    return success;
}


/*******************************************************************
** SaveQueueAsDefault
** ==================
** Saves the clipboard queue to the autosave file.  This is done
** when exiting the program, if the user has the appropriate option
** checked in general settings.
**
** Outputs:
**      BOOL                    - TRUE on success.
*******************************************************************/
BOOL SaveQueueAsDefault()
{
    BOOL success = FALSE;
    TCHAR file_path[MAX_PATH];
    HANDLE fhand;

    GetFileInInstallPath(DEFAULT_SAVE_FILE, file_path);

    fhand = CreateFile(file_path, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(fhand != INVALID_HANDLE_VALUE)
    {
        success = SaveQueueToFile(&gv.cq, fhand);
        CloseHandle(fhand);
    }

    return success;
}



/*******************************************************************
** OpenCommonItems
** ===============
** Loads the common items (which is just another clipboard queue)
** from a file specified in the settings.
**
** Inputs:
**      BOOL show_error - TRUE if the function should show an
**                        error message on failure.  This is
**                        appropriate if e.g. the function is called
**                        immediately when the user changes the
**                        common file in the settings.
**
** Outputs:
**      BOOL            - TRUE on success.  This function will return
**                        FALSE if the file name is empty (i.e. the
**                        user cleared the edit control)
*******************************************************************/
BOOL OpenCommonItems(BOOL show_error)
{
    BOOL success = FALSE;

    if(_tcslen(gv.settings.common_file) > 0)
    {
        HANDLE fhand = CreateFile(gv.settings.common_file,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(fhand != INVALID_HANDLE_VALUE)
        {
            ClipQueue cq;

            if(LoadQueueFromFile(&cq, fhand))
            {
                DestroyQueue(&gv.common);
                gv.common = cq;
                success = TRUE;
            }

            CloseHandle(fhand);
        }

        if(!success && show_error)
        {
            ShowErrorMessage(STRING_ERROR_OPEN_FILE);
        }
    }
    else
    {
        DestroyQueue(&gv.common);
    }

    return success;
}
