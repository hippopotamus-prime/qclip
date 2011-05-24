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
#include "ClipQueue.h"
#include "Clipboard.h"
#include "QClip.h"
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#define MIN_DYNAMIC_SIZE 16
#define MAX_DUPLICATE_TIME 20

static unsigned int CheckDynamicSize(ClipQueue* cq);
static void EmptyQueue(ClipQueue* cq);
static BOOL IsDuplicate(ClipQueue* cq, ClipItem* item);

/*******************************************************************
** PeekAt
** ======
** Provides random access to the queue.  Copies a ClipItem to the
** Windows clipboard from an integer-indexed position.  Does not
** modify the queue.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
**      unsigned int offset - zero-based index in the queue of the
**                            item to copy (i.e. 0 is cq->front) 
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int PeekAt(ClipQueue* cq, unsigned int offset)
{  
    if(offset < GetQueueLength(cq))
    {
        return CopyToClipboard(GetItem(cq, offset));
    }
    else
    {
        return 0;
    }
}


/*******************************************************************
** PushFront
** =========
** Appends a ClipItem from Windows clipboard to the front of the
** queue, increasing the queue size by one.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
*******************************************************************/
void PushFront(ClipQueue* cq)
{
    ClipItem temp_item;

    if(PopulateClipItem(&temp_item) && !IsDuplicate(cq, &temp_item))
    {
        CheckDynamicSize(cq);

        cq->front = (cq->front - 1 + cq->size) % cq->size;
        DestroyClipItem(&cq->clips[cq->front]);
    
        cq->clips[cq->front] = temp_item;

        cq->last_item = cq->front;
        cq->last_time = GetTickCount();

        if(cq->count < cq->size)
        {
            ++(cq->count);
        }

        cq->modified = TRUE;
    }
}


/*******************************************************************
** PushBack
** ========
** Appends a ClipItem from Windows clipboard to the back of the
** queue, increasing the queue size by one.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
*******************************************************************/
void PushBack(ClipQueue* cq)
{
    ClipItem temp_item;

    if(PopulateClipItem(&temp_item) && !IsDuplicate(cq, &temp_item))
    {
        CheckDynamicSize(cq);

        DestroyClipItem(&cq->clips[(cq->front + cq->count) % cq->size]);
    
        cq->clips[(cq->front + cq->count) % cq->size] = temp_item;

        cq->last_item = (cq->front + cq->count) % cq->size;
        cq->last_time = GetTickCount();

        if(cq->count < cq->size)
        {
            ++(cq->count);
        }

        cq->modified = TRUE;
    }
}


/*******************************************************************
** PeekFront
** =========
** Copies a ClipItem from the front of the queue to the Windows
** clipboard.  Does not modify the queue.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int PeekFront(ClipQueue* cq)
{
    if(!IsQueueEmpty(cq))
    {
        return CopyToClipboard(GetItem(cq, 0));
    }
    else
    {
        return 0;
    }
}


/*******************************************************************
** PeekBack
** ========
** Copies a ClipItem from the back of the queue to the Windows
** clipboard.  Does not modify the queue.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int PeekBack(ClipQueue* cq)
{
    if(!IsQueueEmpty(cq))
    {
        return CopyToClipboard(GetItem(cq, cq->count - 1));
    }
    else
    {
        return 0;
    }
}


/*******************************************************************
** PopFront
** ========
** Moves a ClipItem from the front of the queue to the Windows
** clipboard.  Decreases the queue size by one.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int PopFront(ClipQueue* cq)
{
    unsigned int successes = 0;

    if(!IsQueueEmpty(cq))
    {
        successes = CopyToClipboard(GetItem(cq, 0));

        DestroyClipItem(&cq->clips[cq->front]);
        cq->front = (cq->front + 1) % cq->size;
        --(cq->count);

        cq->modified = TRUE;
    }

    CheckDynamicSize(cq);

    return successes;
}



/*******************************************************************
** PopBack
** =======
** Moves a ClipItem from the back of the queue to the Windows
** clipboard.  Decreases the queue size by one.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
**
** Outputs:
**      unsigned int        - number of formats successfully copied
**                            (may be zero)
*******************************************************************/
unsigned int PopBack(ClipQueue* cq)
{
    unsigned int successes = 0;

    if(!IsQueueEmpty(cq))
    {
        --(cq->count);
        successes = CopyToClipboard(GetItem(cq, cq->count));

        DestroyClipItem(&cq->clips[(cq->front + cq->count) % cq->size]);

        cq->modified = TRUE;
    }

    CheckDynamicSize(cq);

    return successes;
}


/*******************************************************************
** DiscardFront
** ============
** Removes the item at the front of the queue.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
*******************************************************************/
void DiscardFront(ClipQueue* cq)
{
    if(!IsQueueEmpty(cq))
    {
        DestroyClipItem(&cq->clips[cq->front]);
        cq->front = (cq->front + 1) % cq->size;
        --(cq->count);

        cq->modified = TRUE;
    }

    CheckDynamicSize(cq);
}


/*******************************************************************
** DiscardBack
** ===========
** Removes the item at the back of the queue.
**
** Inputs:
**      ClipQueue* cq       - address of the queue.
*******************************************************************/
void DiscardBack(ClipQueue* cq)
{
    if(!IsQueueEmpty(cq))
    {
        --(cq->count);

        DestroyClipItem(&cq->clips[(cq->front + cq->count) % cq->size]);

        cq->modified = TRUE;
    }

    CheckDynamicSize(cq);
}



/*******************************************************************
** CreateQueue
** ===========
** Initializes an existing queue by allocating an empty array of
** ClipItems, according to queue_size.
** Be sure to call DestroyQueue.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to
**                                initialize.
**      unsigned int queue_size - maximum size of the queue
**
** Outputs:
**      BOOL                    - TRUE if creation succeeded.
*******************************************************************/
BOOL CreateQueue(ClipQueue* cq, unsigned int queue_size)
{
    InitQueue(cq);
    cq->size = queue_size;
    cq->clips = (ClipItem*) HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(ClipItem) * queue_size);

    return (cq->clips != NULL);
}


/*******************************************************************
** DestroyQueue
** ============
** Frees the memory allocated by CreateQueue and destroys any
** ClipItems contained in the queue.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to
**                                destroy.
*******************************************************************/
void DestroyQueue(ClipQueue* cq)
{
    if(cq->clips)
    {
        EmptyQueue(cq);
        HeapFree(GetProcessHeap(), 0, cq->clips);
    }

    cq->size = 1;
    cq->count = 0;
    cq->clips = NULL;
}


/*******************************************************************
** EmptyQueueAndResize
** ===================
** Empties the queue and shrinks it if dynamic sizing is enabled.
** This function should be called in response to the Empty Queue
** UI command.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to empty.
*******************************************************************/
void EmptyQueueAndResize(ClipQueue* cq)
{
    EmptyQueue(cq);

    if(gv.settings.dynamic_queue)
    {
        ResizeQueue(cq, MIN_DYNAMIC_SIZE);
    }
}


/*******************************************************************
** EmptyQueue
** ==========
** Empties the queue and releases all memory used by ClipItems
** therein.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to empty.
*******************************************************************/
void EmptyQueue(ClipQueue* cq)
{
    unsigned int i;

    if(GetQueueLength(cq) > 0)
    {
        cq->modified = TRUE;
    }

    if(cq->clips)
    {
        for(i = 0; i < cq->size; ++i)
        {
            DestroyClipItem(&cq->clips[i]);
        }
    }

    cq->front = 0;
    cq->count = 0;
}


/*******************************************************************
** InitQueue
** =========
** Resets all queue structure values to defaults, e.g. zero length,
** memory pointing to NULL, etc.
**
** Don't call this on queues with allocated memory.
**
** Inputs:
**      ClipQueue* cq       - address of the queue to initialize.
*******************************************************************/
void InitQueue(ClipQueue* cq)
{
    cq->front       = 0;
    cq->count       = 0;

    cq->last_item   = 0;
    cq->last_time   = 0;

    //The size might logically be 0, but we don't want
    //stuff to crash with divide-by-zero errors...
    cq->size        = 1;
    cq->clips       = NULL;
    cq->modified    = FALSE;
}


/*******************************************************************
** CheckDynamicSize
** ================
** If dynaimc queue sizing is turned on (i.e. unlimited queue),
** expands the queue when it gets full and shrinks it when it goes
** under 1/4 capacity.
**
** This function should be called before adding and after removing
** any item to/from the queue.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to resize.
**
** Outputs:
**      unsigned int            - (possibly new) size of the queue.
*******************************************************************/
unsigned int CheckDynamicSize(ClipQueue* cq)
{
    if(gv.settings.dynamic_queue)
    {
        if(cq->count == cq->size)
        {
            ResizeQueue(cq, cq->size * 2);
        }
        else if(cq->count < cq->size / 4)
        {
            if(cq->size > MIN_DYNAMIC_SIZE * 2)
            {
                ResizeQueue(cq, cq->size / 2);
            }
            else if(cq->size > MIN_DYNAMIC_SIZE)
            {
                ResizeQueue(cq, MIN_DYNAMIC_SIZE);
            }
        }
    }

    return cq->size;
}


/*******************************************************************
** ResizeQueue
** ===========
** Reallocates memory for a queue according to a new size.
** Preserves as many items from the existing queue as possible,
** starting with the newest (i.e. front).  This function will set
** the queue size to the new value if successful.
**
** Inputs:
**      ClipQueue* cq           - address of the queue to resize.
**      unsigned int new_size   - new size of the queue
**
** Outputs:
**      BOOL                    - TRUE if resizing succeeded.
*******************************************************************/
BOOL ResizeQueue(ClipQueue* cq, unsigned int new_size)
{
    ClipItem* new_clips = HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(ClipItem) * new_size);

    unsigned int old_count = cq->count;

    if(new_clips)
    {
        unsigned int i;

        for(i = 0; (i < cq->count) && (i < new_size); ++i)
        {
            new_clips[i] = cq->clips[(cq->front + i) % cq->size];

            cq->clips[(cq->front + i) % cq->size].data = NULL;
            cq->clips[(cq->front + i) % cq->size].formats = 0;
        }

        DestroyQueue(cq);
        cq->clips   = new_clips;
        cq->count   = i;
        cq->front   = 0;
        cq->size    = new_size;

        if(i != old_count)
        {
            cq->modified = TRUE;
        }
    }       

    return (new_clips != NULL);
}


/*******************************************************************
** IsDuplicate
** ===========
** Checks if a new ClipItem is a duplicate of the last one added
** to a particular queue.
**
** Duplicates are only considered if they come within a few ms of
** each other - the idea is to block programs that inadvertently
** copy data to the clipboard multiple times.
**
** Inputs:
**      ClipItem* item      - the item to check
**
** Outputs:
**      BOOL                - TRUE if the item is a duplicate
*******************************************************************/
BOOL IsDuplicate(ClipQueue* cq, ClipItem* item)
{
    BOOL duplicate = FALSE;

    if(cq && cq->modified)
    {
        unsigned int now = GetTickCount();

        unsigned int relative_position =
            (cq->last_item - cq->front + cq->size) % cq->size;

        if((relative_position < cq->count)
        && (now - cq->last_time < MAX_DUPLICATE_TIME))
        {
            duplicate = CompareClipItems(item, cq->clips + cq->last_item);
        }
    }

    return duplicate;
}
