# DVDStyler AppImage

[![Build Status](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/build.yml/badge.svg)](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/build.yml)

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
8. Blu-ray (HD) output: encodes titles to Blu-ray compliant H.264 / AC-3 and
   authors a BDMV structure with tsMuxeR, then builds a UDF ISO with mkisofs
   (toggle "generate Blu-ray (HD) output" in Settings; Blu-ray and AVCHD modes
   selectable since the AVCHD profile adds 720p50/60 and 1 s GOP support)
9. The AppImage now bundles all authoring tools, so the DVD/Blu-ray pipeline
   works out of the box on any distribution
10. Version bumped from 3.3b4 to 3.3b5

## Bundled tools

The AppImage ships with its own copies of:

* dvdauthor (spumux/dvdauthor) for DVD subtitle multiplexing and authoring
* mjpegtools (mplex) for DVD multiplexing
* mkisofs for DVD ISO and Blu-ray/AVCHD UDF ISO images
* growisofs + dvd+rw-tools for burning
* tsMuxeR for Blu-ray BDMV authoring
* ffmpeg + ffprobe (with libx264) for transcoding

The launcher adds these to `PATH` automatically, so no system packages are
required.

## System requirements

* Linux x86_64
* A VA API capable GPU for hardware decoding (optional)

## Download and run

Grab the AppImage from the Releases page, make it executable, and run it:

    chmod +x DVDStyler-3.3b5-x86_64.AppImage
    ./DVDStyler-3.3b5-x86_64.AppImage

## Build

The GitHub Actions workflow (`.github/workflows/build.yml`) builds DVDStyler
from this repository, bundles the tools above with linuxdeploy, and uploads the
resulting AppImage as an artifact (and as a release for tags).

To build locally on an Ubuntu-compatible system, install the packages listed
in the workflow and run:

    bash packaging/build-appimage.sh

## License

DVDStyler is free software distributed under the GNU General Public License
version 2 (GPL v2). See the COPYING file for the full license text.
