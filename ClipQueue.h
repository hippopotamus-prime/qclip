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

#ifndef __CLIPQUEUE__
#define __CLIPQUEUE__

#include "Clipboard.h"

typedef struct
{
    unsigned int    front;
    unsigned int    count;
    unsigned int    size;
    ClipItem*       clips;
    unsigned int    last_item;  //index of the last inserted item
    unsigned int    last_time;  //tick count for the last insertion
    BOOL            modified;
}ClipQueue;

extern unsigned int PeekAt(ClipQueue* cq, unsigned int offset);
extern void PushFront(ClipQueue* cq);
extern unsigned int PopFront(ClipQueue* cq);
extern unsigned int PeekFront(ClipQueue* cq);
extern void PushBack(ClipQueue* cq);
extern unsigned int PopBack(ClipQueue* cq);
extern unsigned int PeekBack(ClipQueue* cq);
extern void EmptyQueueAndResize(ClipQueue* cq);
extern void DiscardFront(ClipQueue* cq);
extern void DiscardBack(ClipQueue* cq);

extern void InitQueue(ClipQueue* cq);
extern BOOL CreateQueue(ClipQueue* cq, unsigned int queue_size);
extern void DestroyQueue(ClipQueue* cq);
extern BOOL ResizeQueue(ClipQueue* cq, unsigned int new_size);

#define GetItem(cq, offset) (&(cq)->clips[((cq)->front + offset) % (cq)->size])
#define GetQueueLength(cq)  ((cq)->count)
#define IsQueueEmpty(cq)    ((cq)->count == 0)

#endif
