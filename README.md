# DVDStyler AppImage

[![Build AppImage](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/drDOOM69GAMING/dvdstyler-appimage/actions/workflows/build.yml)

A custom build of DVDStyler packaged as a portable AppImage for Linux.
Based on DVDStyler 3.3b4 with improvements, released here as version 3.3b10.

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
    (fontconfig, freetype, fribidi, zlib) that linuxdeploy normally skips,
    so it runs on more distributions

## Changes in version 3.3b7

21. VA-API hardware video **encoding** (h264_vaapi) added as an optional
    toggle in Settings > Generate ("Use hardware video encoding (VA-API)").
    Faster than the CPU x264 encode on capable GPUs, but not strictly
    Blu-ray compliant, so it is disabled by default. Hardware video
    decoding (VA-API) remains automatic when a render node is present
22. Encoding progress bar fixed: it now parses modern ffmpeg output
    (`frame=NNN` without a space), so progress advances during encoding
    instead of staying frozen
23. Fixed authoring failure on re-runs: the build output directory was
    cleaned but a leftover BDMV/CERTIFICATE from a previous run was not
    removed, so tsMuxeR overwrote the same playlist file and the authoring
    step reported "did not add a playlist"; BDMV and CERTIFICATE are now
    deleted (recursively) before a fresh authoring run
24. Version bumped from 3.3b6 to 3.3b7

## Later changes in this build

25. HD auto-bitrate: a new "Auto video bitrate (fit to media size)" checkbox
    in Settings > Core (default on) computes the video bitrate so the output
    fills the selected media size (DVD-5/DVD-9 for AVCHD, BD-R for Blu-ray)
    instead of using a fixed bitrate. No more "over sized" warnings unless the
    content physically cannot fit at the minimum bitrate; the AVCHD/Blu-ray
    bitrate spinners are disabled while auto-fit is on
26. The Blu-ray/AVCHD ISO is now built with a bundled pure-Python UDF 2.50
    builder (`udf250`), producing a genuine UDF 2.50 image (no ISO9660 layer)
    in the same layout as the reference PS4-proven discs. `mkisofs` is kept
    only for DVD-Video images
27. Fixed directory iteration bugs (wxDir::GetFirst/GetNext misuse) in the
    cache cleaner, temp-dir cleaner, DVD filesystem cleanup and the BDMV
    playlist counter that could cause one or more entries to be skipped
28. Child processes (ffmpeg, dvdauthor, mkisofs, growisofs, tsMuxeR, etc.)
    left running by the app are killed when the window is closed, so they do
    not linger after exit
29. The FFmpeg/libavformat "can't seek on file descriptor / can't find length
    of file on file descriptor" lines that flooded the log when probing
    non-seekable streams are filtered out, and the harmless wxFile
    seek/length warnings emitted during shutdown are suppressed, so closing
    the app no longer spams the console
30. New **File > Burn ISO...** menu item: pick an existing ISO image and it is
    burned straight to the configured drive with the default burn tool
    (growisofs) at the default speed, so no re-encoding or project is required
31. The Settings > Core tab is now scrollable and the command fields are wider,
    so all options stay reachable on smaller windows
32. Version bumped to 3.3b8
33. HD output is always authored at 1920x1080 with the NTSC/PAL standards
    frame rate: film 23.976, 29.97/59.94 recordings 29.97, 25/50 sources 25.
    The non-standard 720p50/60 AVCHD branch was removed (those 720p59.94
    streams were rejected by the PS4 and standalone players), and the
    Blu-ray/AVCHD ISO step now uses the bundled `udf250` UDF 2.50 builder and
    excludes the tsMuxeR CERTIFICATE tree for AVCHD playback compatibility
34. Version bumped to 3.3b9
35. Fixed Blu-ray/AVCHD authoring of projects with more than one movie: every
    title now gets its own playlist, clip and stream (tsMuxeR `--mplsOffset` /
    `--m2tsOffset`, title 0 → `00000`, title 1 → `00001`, ...). Previously every
    title was multiplexed as `00000`, so the second title overwrote the first
    one's files and the build aborted with "tsMuxeR did not add a playlist for
    this title"
36. Fixed the disc's first-play object: tsMuxeR leaves `BDMV/MovieObject.bdmv`
    pointing at the last title it muxed, so a multi-title disc started with the
    last movie added; it is now reset to the first title (this matches the
    single-title output byte for byte)
37. Fixed the build against FFmpeg 9 (libavcodec 63), which removed the
    `AVCodec.sample_fmts` field that the unused `hasSampleFmt()` helper (left
    over upstream) still read; on that version the supported sample formats are
    now queried with `avcodec_get_supported_config()`
38. Fixed AVCHD projects being muxed as full Blu-ray: `SaveTsMuxeRMeta()` always
    passed `--blu-ray`, and tsMuxeR checks for `--blu-ray` before `--avchd`, so
    every AVCHD build got Blu-ray v2/v3 descriptors plus the META/BDJO/JAR
    directories that AVCHD does not allow, one of the reasons players reported
    "unsupported disc" for AVCHD. AVCHD now emits `--avchd` and gets the AVCHD
    (v1) index and descriptors, verified against tsMuxeR 2.7.0
39. Fixed chapter marks being dropped: the mux option used `--chapter=`, which
    tsMuxeR does not know and silently ignores, so chapters were never written
    to the playlist. It is now `--custom-chapters=` with `;`-separated
    `hh:mm:ss.mmm` values; verified against tsMuxeR 2.7.0 (1 mark before, 2
    marks after)
40. Fixed A/V desync of muxed menu and slideshow audio: the video path rescaled
    timestamps to the stream time base but the audio path wrote them in codec
    units (1/48000) into a 1/90000 stream, so the audio stream ended after
    only 48000/90000 (53%) of the video length
41. Fixed silent audio frames being only half cleared for packed (non planar)
    formats, and several FFmpeg resource leaks (one AVPacket and the video
    AVFrame per frame, the output format context freed with `av_freep()` on its
    streams instead of `avformat_free_context()`)
42. Fixed AVCHD/Blu-ray ISO images with 39 or more titles being written corrupt:
    every directory was given a single sector for its File Identifier
    Descriptors, which overflowed and pushed the rest of the image out of place
    (the image was written larger than the size it declared, so readers saw an
    empty disc). Directories now get as many sectors as they need
43. Fixed the Blu-ray temp directory `<temp>/bd-tmp` never being cleaned up:
    it kept accumulating the transcoded `.264`/`.ac3` files of every run
44. Stored disc mode and HD audio bitrate are now validated when read, like they
    already were when written
45. Version bumped to 3.3b10
46. Hardware (VA-API) encoding is now detected automatically: when the
    application starts it runs a short probe encode on the GPU and, if the
    probe passes, the "Video encoding:" choice in the New Project / DVD
    Properties panel is preselected to GPU. The choice itself is never
    disabled, so CPU can be picked by hand, and a driver that fails the
    probe is reported as a tooltip on the GPU option
47. Fixed VA-API detection: the render node `/dev/dri/renderD128` is a
    character device and the check tested for a regular file, so hardware
    support was never found. Hardware decoding stayed off and GPU encoding
    was always reported as unavailable
48. A GPU encode that fails during a build now retries that movie with the
    CPU encoder and logs the switch, instead of stopping the build
49. The generation window is sized after its button labels are final, so the
    whole button row (minimize, cancel/close) is visible without resizing,
    and the window is titled "Generate Blu-ray" or "Generate AVCHD" for HD
    builds instead of always "Generate DVD"
50. Repeated chapter marks in a source (the same moment reported several
    times, as MP4 chapter tracks often do) are collapsed, so the key frame
    list no longer contains duplicates
51. Fixed two AppImage bundling defects that only showed up on machines with
    a newer desktop stack. The bundled C++ runtime was older than what host
    libraries ask for (libjack on current distributions needs
    GLIBCXX_3.4.32), so the loader refused to start the bundled ffmpeg at
    all. The bundled libva was older than the host's VA-API driver, so device
    creation failed with "Device creation failed: -5" and hardware encoding
    never started. Both now come from the host, like libdrm already did

## Blu-ray / AVCHD playback status

The disc authored by this build (1920x1080, standards-rate H.264/AC-3, UDF 2.50
BDMV) plays fully on a standalone Blu-ray player and mounts/decodes cleanly in
software. Two defects that made AVCHD discs unplayable on consoles are fixed in
this version: AVCHD projects were muxed with `--blu-ray`, i.e. with Blu-ray
v2/v3 descriptors and META/BDJO/JAR directories, and ISOs with 39 or more
titles were written corrupt. AVCHD projects now get the AVCHD (v1) index and
descriptors. The Sony PlayStation 4 still needs to be retested - the PS4-proven
reference discs are authored at 1080p23.976, so if a 29.97 Hz encode is still
rejected the remaining suspect is the frame rate, not the disc structure.

## Hardware (VA-API) encoding

Hardware encoding runs the decode, the scaling and the H.264 encode on the
GPU render node and uses a fraction of the CPU load of the x264 software
encode. When the application starts it probes the GPU by encoding a few
frames with `h264_vaapi`; if the probe passes, GPU encoding is preselected for
HD builds and the option is documented in the "Video encoding:" choice of the
New Project / DVD Properties panel. The probe is repeated on every start, so a
driver or kernel update is picked up without a setting to hunt for. When the
probe fails the CPU option simply stays selected and the tooltip on the GPU
option shows the error the probe reported. A GPU encode that fails in the
middle of a build falls back to the CPU encoder for that movie. Hardware
decoding of source files is automatic when a render node is present.

## Bundled tools

The AppImage ships with its own copies of:

* dvdauthor (spumux/dvdauthor) for DVD subtitle multiplexing and authoring
* mjpegtools (mplex) for DVD multiplexing
* mkisofs for DVD-Video ISO images
* udf250 (pure-Python UDF 2.50 builder) for Blu-ray/AVCHD ISO images
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
  do not ship `libfuse2` by default, so install it
  (`sudo apt install libfuse2` / `sudo dnf install fuse2-libs`), or run the
  AppImage with `--appimage-extract-and-run` to use it without FUSE.
* **An X11 or XWayland session** with a desktop font set and a running D-Bus
  session (present on any standard desktop install).
* **A VA-API capable GPU** for hardware video decoding (optional; falls back
  to software encoding/decoding automatically).

## Download and run

Grab the AppImage from the Releases page, make it executable, and run it:

    chmod +x DVDStyler-3.3b10-x86_64.AppImage
    ./DVDStyler-3.3b10-x86_64.AppImage

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
