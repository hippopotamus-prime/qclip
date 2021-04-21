# QClip Changelog

## 0.9.4 - 2021-04-20
### New Features
* Project uploaded to GitHub. References to the defunct cadae.net have
been removed.
* Updated the license to GPL v3.
* QClip now compiles with Visual Studio 2019.
* Updated popup menu bitmap rendering for Windows 10.
### Fixes
* Cleared hotkeys are now saved properly.

## 0.9.3.1 - 2008-05-30
### Fixes
* The default queue is now saved during Windows shutdown.

## 0.9.3 - 2007-12-29
### New Features
* Added customizable date / time format to the common items
available under general preferences.
* Added logic to block programs that place data on the clipboard
multiple times for a single copy.
* Added possible key shortcuts to access the five most recent
items in the queue, and the first five custom items. These are
unassigned by default.
### Fixes
* Improved handling of bitmaps. Fixes the strange three-pixel shift
problem that occurred when using Print Screen on WinXP.
* Now using local time for dates (was always GMT before).

## 0.9.2 - 2006-11-28
### New Features
* QClip now tries to use relative file paths when appropriate (e.g.
when selecting common items from the application directory).
* Added the ability to empty the queue from the tray menu.
* "Files" in the format settings now includes various registered
application formats used by Windows Explorer to augment the
standard "file" clipboard format. Explorer would occasionally do
strange things when these were left out.
### Fixes
* Made the Shortcut Key box in the key settings wider.
* The apply button under format settings functions correctly now.
* When copying files on Unicode systems, the number of items copied
was displayed incorrectly in the popup menu. Fixed.
* On WinXP, clicking the tray icon after exiting the program would
cause a crash. Fixed.

## 0.9.1 - 2006-11-16
### New Features
* Added unlimited queue option.
### Fixes
* Shortcut keys for long/short date work now.
* Menu commands to paste custom items were ignored. Fixed.
* Dates were pasted in the wrong text format (ANSI/Unicode). Fixed.
* Pasting from the popup menu should be somewhat more reliable now.

## 0.9 - 2006-10-20
* First release