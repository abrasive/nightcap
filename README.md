Nightcap lets you run old Windows screensavers under XScreensaver, using wine.

This uses the preview mode of the screensaver - where it draws into that little monitor in Windows' display settings.
Screensavers that behave differently in preview mode probably won't work very well.

Tested with various MS/Plus screensavers, including the all-important 3D Maze.

# Usage

0. Make sure `wine` is installed and you can run `winegcc`.

1. Compile `nightcap.exe`:

    ```
    make
    ```

2. Make sure `nightcap.exe` is on your `PATH`. Note that the `xscreensaver` daemon needs to see this as well.

3. Place your screensavers in the directory with `nightcap.exe`.

4. Edit `~/.xscreensaver` to add your new screensavers! You need to add them to the end of the `programs:` entry, which is a multiline adventure. Make sure you don't leave a blank line between the existing entries and the new ones! The end of my programs list looks like this:

    ```
    - GL: 				squirtorus --root			    \n\
      GL: 				hextrail --root				    \n\
               "3D Flower Box"      nightcap.exe "3D Flower Box.scr"        \n\
               "3D Flying Objects"  nightcap.exe "3D Flying Objects.scr"    \n\
               "3D Maze"            nightcap.exe "3D Maze.scr"              \n\

    ```

    The first quoted string is the title displayed in the XScreensaver config tool; the second is the filename of the screensaver itself.

Unfortunately you can't access the screensaver configurations through the XScreensaver UI.
Instead, run `wine screensavername.scr` to access each saver's config.


# How

Windows' screensaver preview mode works by supplying a HWND on the commandline; the screensaver is responsible for creating a child window and drawing its output there.
Meanwhile, XScreensaver provides its own X11 window, giving us the window ID to draw into.

So all we have to do is we create a Win32 window, find the X11 window that Wine created for it, reparent the X11 window into XScreensaver's X11 window, and then ask the screensaver to create itself under the Win32 window.

Easy when you live in the cursēd half-light of `winelib`, one foot firmly on the UNIX plane, and the other dipped in the Gatesian river...
