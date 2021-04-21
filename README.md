# QClip
> Queue-based clipboard extender for Windows

## About
QClip is a clipboard extender for Windows that monitors the standard Windows
clipboard and copies any data it sees there into a queue. You can then paste
the data into other applications at any later time using hotkeys defined by
the program. QClip works with data in any format, including text, graphics,
files, etc.

QClip is designed with a minimalist approach - it has no UI to speak of,
uses few system resources, and does not impact the registry.

## Free Software
Interlopers is free software as described by the GNU General Public
License (v3). See LICENSE.md for details.

## Installation
QClip requires no installation; simply place the executable in your favorite
directory and run it. The program will happily run from flash drives and the
like.

## Basic Usage
When launched, QClip will place an icon in the system tray. Right-clicking
the icon will allow you to exit the program or access the Preferences dialog.
Most other interaction is done through keyboard shortcuts.

Whenever you copy data to the Windows clipboard, QClip will add it to a queue.
Pressing **Ctrl-Alt-V** will open a popup menu showing every item currently
in the queue, as well as any user-defined "common items". Selecting an item
from the popup menu will paste it.

You can also "pop" data from either end of the queue by pressing **Ctrl-Alt-F**
(for the most recent item) or **Ctrl-Alt-B** (for the oldest). This will remove
the data from the queue and paste it into whatever application has focus.

For example: Highlight some text and press **Ctrl-C**, then highlight some more
and press **Ctrl-C** again. Repeat this a few times. Now open Notepad and press
**Ctrl-Alt-B** a few times. You'll see the various text items appear in the
order you copied them. Pressing **Ctrl-Alt-F** would have pasted the text in
reverse order. 

For more detail, refer to the Help page accessible from the system tray icon.

## Building
As of version 0.9.4, QClip releases are built with Visual Studio 2019.
Use qclip.sln.

Previous versions were developed with [Dev-C++](http://bloodshed.net) and
MinGW.
