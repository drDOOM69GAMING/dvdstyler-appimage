/////////////////////////////////////////////////////////////////////////////
// Name:        HdProfile.cpp
// Purpose:     Blu-ray/AVCHD profile resolver (pipeline planner)
// Author:      drDOOM69GAMING
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "HdProfile.h"
#include <math.h>

namespace HdProfile {

/** Returns the x264 level string for the resolved level, e.g. "4.1". */
wxString Params::GetLevelStr() const {
	return wxString::Format(_T("4.%d"), level - 40);
}

/** Resolves the user's choices into the encoder parameter set.
  *
  * The VBV constraints (maxrate/bufsize) are fixed by the Blu-ray/AVCHD
  * specifications, not by the user's bitrate target: AVCHD is capped at
  * 18 Mbit/s, Blu-ray at 40 Mbit/s. The quality preset tunes the efficiency
  * parameters (reference frames, B-frames) inside those bounds. */
Params Resolve(Mode mode, Quality quality, int videoBitrateKbps) {
	Params p;
	p.mode = mode;
	p.quality = quality;
	bool avchd = mode == MODE_AVCHD;
	p.level = avchd ? 40 : 41;
	p.gopSeconds = avchd ? 1.0 : 2.0;
	p.maxrateKbps = avchd ? 18000 : 40000;
	p.bufsizeKbps = avchd ? 15000 : 30000;
	// never let the target bitrate exceed the VBV buffer (ffmpeg warns otherwise)
	if (p.bufsizeKbps < videoBitrateKbps)
		p.bufsizeKbps = videoBitrateKbps;
	switch (quality) {
	case QUALITY_STANDARD:
		p.bframes = 2;
		p.refs = 3;
		break;
	case QUALITY_HIGH_PLUS:
		p.bframes = 4;
		p.refs = 5;
		break;
	default:
		p.bframes = 3;
		p.refs = 4;
		p.quality = QUALITY_HIGH;
		break;
	}
	p.slices = 4;
	p.blurayCompat = true;
	return p;
}

/** Maximum VBV video bitrate allowed for the given mode (kbit/s). */
int MaxVideoBitrateKbps(Mode mode) {
	return mode == MODE_AVCHD ? 18000 : 40000;
}

/** Usable raw capacity in bytes for a media-size preset. */
double MediaSizeBytes(MediaSize size) {
	switch (size) {
	case MEDIA_DVD5: return 4700372992.0;		// ~4.7 GB (AVCHD/DVD-Video)
	case MEDIA_DVD9: return 8543666176.0;		// ~8.5 GB (AVCHD/DVD-Video)
	case MEDIA_BD25: return 25025314816.0;		// BD-R 25 GB
	case MEDIA_BD50: return 50050629632.0;		// BD-R 50 GB (BDXL)
	case MEDIA_BD100: return 100103242752.0;	// BD-R 100 GB (BDXL)
	}
	return 25025314816.0;
}

/** Usable raw capacity in GB (for display). */
double MediaSizeGB(MediaSize size) {
	return MediaSizeBytes(size) / 1.0e9;
}

/** True if the media-size preset can be used with the given authoring mode. */
bool MediaSizeValidForMode(MediaSize size, Mode mode) {
	if (mode == MODE_AVCHD)
		return size == MEDIA_DVD5 || size == MEDIA_DVD9;
	if (mode == MODE_BLURAY)
		return size == MEDIA_BD25 || size == MEDIA_BD50 || size == MEDIA_BD100;
	return false;
}

/** Default media size for the given authoring mode. */
MediaSize DefaultMediaSize(Mode mode) {
	return mode == MODE_AVCHD ? MEDIA_DVD5 : MEDIA_BD25;
}

/** Rough encoded-size estimate for the given program duration (bytes). */
double EstimateBytes(double totalSeconds, int videoBitrateKbps, int audioBitrateKbps) {
	return (videoBitrateKbps + audioBitrateKbps) * 1000.0 / 8.0 * totalSeconds * 1.05;
}

}
