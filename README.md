# DVDStyler AppImage

[![Build Status](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/verify.yml/badge.svg)](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/verify.yml)

A custom build of DVDStyler packaged as a portable AppImage for Linux.
Based on DVDStyler 3.3b4 with improvements, released here as version 3.3b5.

## What is this

DVDStyler is a cross platform authoring system for Video DVD production.
This AppImage bundles everything so it runs on any x86_64 Linux distribution
without a manual install.

## Changes in version 3.3b5

1. Default temp directory is now ~/.dvdstyler instead of /tmp
2. New "Create DVD without menus" option
3. New prompt after encoding so you can burn the DVD, create an ISO image, burn and create ISO, or do nothing
4. VA API hardware video decoding of source files (toggle in Settings)
5. Dark mode aware launcher that follows the system color scheme
6. Updated About dialog with a special thanks credit
7. The "No template" button in the template dialog now reads "No menu"
8. Version bumped from 3.3b4 to 3.3b5

## System requirements

* Linux x86_64
* A VA API capable GPU for hardware decoding (optional)

## Download and run

Grab the AppImage from the Releases page, make it executable, and run it:

    chmod +x DVDStyler-3.3b5-x86_64.AppImage
    ./DVDStyler-3.3b5-x86_64.AppImage

## Build

Built from the DVDStyler git source with the changes listed above.

## License

DVDStyler is free software distributed under the GNU General Public License
version 2 (GPL v2). See the COPYING file for the full license text.
