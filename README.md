# DVDStyler AppImage

[![Build Status](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/build.yml/badge.svg)](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/build.yml)

A custom build of DVDStyler packaged as a portable AppImage for Linux.
Based on DVDStyler 3.3b4 with improvements, released here as version 3.3b6.

## What is this

DVDStyler is a cross platform authoring system for Video DVD production.
This AppImage bundles the application, its libraries, and every authoring
tool so it runs on any x86_64 Linux distribution without a manual install.

## Changes in version 3.3b6

1. Default temp directory is now ~/.dvdstyler instead of /tmp
2. New "Create DVD without menus" option
3. New prompt after encoding so you can burn the DVD, create an ISO image, burn and create ISO, or do nothing
4. VA API hardware video decoding of source files (toggle in Settings)
5. Dark mode aware launcher that follows the system color scheme
6. Updated About dialog with a special thanks credit
7. The "No template" button in the template dialog now reads "No menu"
8. Blu-ray (BD-R/BD-RE) and AVCHD (DVD-R/RW) authoring: a new "HD output"
   mode in Settings > System core switches the pipeline to full 1080p
   H.264 encodes, authored to BDMV with tsMuxeR and packaged as a UDF 2.50
   ISO (mkisofs). AVCHD uses Blu-ray High@4.0 constraints with a 1s GOP and
   18 Mbit/s max rate so it fits on DVD media (high-fps sources become
   720p50/60); Blu-ray uses High@4.1 with a 2s GOP and 40 Mbit/s max rate
9. HD quality profiles (Standard/High/High+) trade encode speed for quality
   by adjusting the B-frame and reference-frame counts
10. Settings refactored into a validated, layered model (app/video/audio/
    subtitles/menu/disc/encode/output sections) with clamping validation and
    dependency handling: switching the HD mode re-clamps the applicable
    bitrates and enables only the controls that apply
11. The resolution warning icon in the New Project/DVD Properties dialogs is
    now mode-aware: it shows an info tooltip in Blu-ray/AVCHD mode instead of
    the non-standard-DVD warning
12. The AppImage bundles all authoring tools and libraries, so the DVD/
    Blu-ray pipeline works out of the box on any distribution
13. Version bumped from 3.3b4 to 3.3b6

## Later changes in this build

14. VSO-style default folders: projects and build output now live in
    `~/Documents/DVDStyler`, ISO images in `~/Videos/`, and the burn temp
    dir in `~/Documents/DVDStyler/temp`. Saved paths from earlier installs
    are migrated once on the first run
15. Auto-named projects: adding the first video to a new project names it
    (and the "Save As" / burn-dialog defaults) after the video file
16. Settings > Interface "Default video format:" now lists all 14 formats,
    including the HD formats (Half HD, HDV and Full HD, in PAL and NTSC),
    instead of only the SD ones
17. Media-size presets for HD output in Settings > Core: DVD-5 / DVD-9
    (AVCHD) and BD-R 25 / 50 / 100 GB (Blu-ray); the Blu-ray/AVCHD
    capacity check and burn dialog use the selected preset
18. The HD controls (quality profile, Blu-ray/AVCHD video bitrate, audio
    bitrate, tsMuxeR and Blu-ray ISO commands) are dependency-wired to the
    selected output mode (DVD/Blu-ray/AVCHD), and the Core tab grid was
    re-aligned so labels and controls never stagger
19. Clarified Settings labels: "Extra FFmpeg options:", "Burn DVD-Video
    command:", "Add ECC (error correction) command:", "Format disc
    command:", "Use mplex (MPEG multiplexer):", "HD audio bitrate:" and
    "allow HD resolutions"
20. Bundling hardened: the AppImage force-deploys core runtime libraries
    (fontconfig, freetype, fribidi, zlib, libstdc++, libgcc) that
    linuxdeploy normally skips, so it runs on more distributions

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

The AppImage bundles the application, its GUI libraries, and every authoring
tool (only the glibc core runtime is left to the host, since it can never be
bundled). Everything else is self-contained. What is still needed on the
host:

* **Linux x86_64 with glibc >= 2.35** (Ubuntu 22.04 or newer, Debian 12+,
  Fedora 39+, Arch, etc.). Builds produced by the GitHub Actions workflow are
  compiled on Ubuntu 22.04 so they run on any distro with that glibc floor.
* **FUSE** to launch an AppImage file. Recent Ubuntu (22.04/24.04) and Fedora
  do not ship `libfuse2` by default — install it
  (`sudo apt install libfuse2` / `sudo dnf install fuse2-libs`), or run the
  AppImage with `--appimage-extract-and-run` to use it without FUSE.
* **An X11 or XWayland session** with a desktop font set and a running D-Bus
  session (present on any standard desktop install).
* **A VA-API capable GPU** for hardware video decoding (optional; falls back
  to software encoding/decoding automatically).

## Download and run

Grab the AppImage from the Releases page, make it executable, and run it:

    chmod +x DVDStyler-3.3b6-x86_64.AppImage
    ./DVDStyler-3.3b6-x86_64.AppImage

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
