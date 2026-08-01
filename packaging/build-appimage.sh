#!/usr/bin/env bash
# Builds DVDStyler from this repository and packages a self-contained
# AppImage with all authoring/burning tools (dvdauthor, mjpegtools,
# cdrtools/genisoimage, dvd+rw-tools, tsMuxeR, ffmpeg) bundled.
#
# Intended to run on Ubuntu (GitHub Actions, ubuntu-22.04). Run as root
# inside the CI runner. See .github/workflows/build.yml.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

VERSION="${VERSION:-3.3b9}"
APPDIR="$ROOT/DVDStyler.AppDir"
TOOLS="$ROOT/.appimage-tools"
rm -rf "$APPDIR"
mkdir -p "$TOOLS" "$APPDIR"
export NO_STRIP=1

fetch() {
	local url="$1" out="$2"
	[ -f "$out" ] || curl -fsSL -o "$out" "$url"
	chmod +x "$out"
}

# ---------------------------------------------------------------------------
# Build DVDStyler
# ---------------------------------------------------------------------------
./autogen.sh
./configure --prefix=/usr
make -j"$(nproc)"
make install DESTDIR="$APPDIR"

# ---------------------------------------------------------------------------
# Bundle extra runtime tools (binary + libraries via linuxdeploy)
# ---------------------------------------------------------------------------
EXTRA_BINARIES=(
	/usr/bin/ffmpeg
	/usr/bin/ffprobe
	/usr/bin/dvdauthor
	/usr/bin/spumux
	/usr/bin/mplex
	/usr/bin/mkisofs
	/usr/bin/growisofs
	/usr/bin/dvd+rw-mediainfo
	/usr/bin/dvd+rw-format
	/usr/bin/dvd-ram-control
)

TOOL_ARGS=()
for bin in "${EXTRA_BINARIES[@]}"; do
	if [ -x "$bin" ]; then
		TOOL_ARGS+=(-e "$bin")
	fi
done

# tsMuxeR (statically linked)
TSMUXER_VERSION=2.7.0
TSMUXER_ZIP="$TOOLS/tsMuxeR-$TSMUXER_VERSION-linux.zip"
fetch "https://github.com/justdan96/tsMuxer/releases/download/$TSMUXER_VERSION/tsMuxer-$TSMUXER_VERSION-linux.zip" "$TSMUXER_ZIP"
unzip -o -q "$TSMUXER_ZIP" -d "$APPDIR/usr/bin"
chmod +x "$APPDIR/usr/bin/tsMuxeR"

# ---------------------------------------------------------------------------
# UDF 2.50 image builder (pure-Python, replaces mkisofs for the Blu-ray/AVCHD
# ISO step; see src/Config.h DEF_BLURAY_ISO_CMD). The `udf250` wrapper is
# found via PATH (AppRun adds usr/bin); it locates udf250.py itself and runs
# it with python3, which must exist on the host (present by default on all
# supported distros).
# ---------------------------------------------------------------------------
install -m 0755 packaging/udf250.py "$APPDIR/usr/bin/udf250.py"
install -m 0755 packaging/udf250 "$APPDIR/usr/bin/udf250"

# ---------------------------------------------------------------------------
# Bundle every remaining host dependency that is legal to ship (everything
# except the glibc core: libc/libm/libdl/libpthread/ld-linux). linuxdeploy
# skips some of these via its excludelist, so we force-deploy them explicitly;
# linuxdeploy also pulls in their transitive dependencies.
# ---------------------------------------------------------------------------
find_lib() {
	local name="$1" p
	for p in "/usr/lib/$name" "/usr/lib/x86_64-linux-gnu/$name" \
			"/lib/$name" "/lib/x86_64-linux-gnu/$name" "/usr/lib64/$name"; do
		if [ -e "$p" ]; then
			echo "$p"
			return 0
		fi
	done
	return 1
}

EXTRA_LIBS=(
	libfontconfig.so.1
	libfreetype.so.6
	libfribidi.so.0
	libz.so.1
	libstdc++.so.6
	libgcc_s.so.1
)
LIB_ARGS=()
for lib in "${EXTRA_LIBS[@]}"; do
	p="$(find_lib "$lib")"
	if [ -n "$p" ]; then
		LIB_ARGS+=(-l "$p")
	else
		echo "WARNING: could not locate $lib, leaving it to the host" >&2
	fi
done

# ---------------------------------------------------------------------------
# Assemble the AppDir with linuxdeploy + gtk plugin
# ---------------------------------------------------------------------------
LD="$TOOLS/linuxdeploy-x86_64.AppImage"
GTK="$TOOLS/linuxdeploy-plugin-gtk"
AT="$TOOLS/appimagetool-x86_64.AppImage"
fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" "$LD"
fetch "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh" "$GTK"
fetch "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" "$AT"

export GTK_USE_UTF8=1
export VERSION="$VERSION"

"$LD" --appimage-extract-and-run \
	--appdir "$APPDIR" \
	-e "$APPDIR/usr/bin/dvdstyler" \
	"${TOOL_ARGS[@]}" \
	"${LIB_ARGS[@]}" \
	--plugin gtk \
	--desktop-file data/dvdstyler.desktop \
	--icon-file data/dvdstyler.png

# ---------------------------------------------------------------------------
# Replace the generated AppRun with our own (adds bundled tools to PATH)
# and package with appimagetool (which does not touch AppRun)
# ---------------------------------------------------------------------------
cp packaging/AppRun "$APPDIR/AppRun"
chmod +x "$APPDIR/AppRun"
cp data/dvdstyler.png "$APPDIR/dvdstyler.png"
cp data/dvdstyler.desktop "$APPDIR/dvdstyler.desktop"

"$AT" --no-appstream "$APPDIR" "$ROOT/DVDStyler-$VERSION-x86_64.AppImage"
ls -lh "$ROOT"/*.AppImage
echo "AppImage built: $ROOT/DVDStyler-$VERSION-x86_64.AppImage"
