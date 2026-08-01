/////////////////////////////////////////////////////////////////////////////
// Name:        HdProfile.h
// Purpose:     Blu-ray/AVCHD profile resolver (pipeline planner)
// Author:      drDOOM69GAMING
// Licence:     GPL
//
// Maps a small set of user choices (mode, quality, bitrate) onto the
// coherent, spec-bounded set of encoder/authoring parameters the pipeline
// consumes. This is the "profile resolver" layer between the settings model
// and the encoder.
/////////////////////////////////////////////////////////////////////////////

#ifndef DS_HD_PROFILE_H
#define DS_HD_PROFILE_H

#include <wx/wx.h>

namespace HdProfile {

/** HD authoring mode (matches the Disc settings model). */
enum Mode {
	MODE_NONE = 0,		// DVD authoring
	MODE_BLURAY = 1,	// BD-R/BD-RE
	MODE_AVCHD = 2		// AVCHD on DVD-R/RW
};

/** Media size preset used for capacity checks and real-media burning. */
enum MediaSize {
	MEDIA_DVD5 = 0,		// AVCHD: single-layer DVD (4.7 GB)
	MEDIA_DVD9 = 1,		// AVCHD: dual-layer DVD (8.5 GB)
	MEDIA_BD25 = 2,		// Blu-ray: BD-R 25 GB
	MEDIA_BD50 = 3,		// Blu-ray: BD-R 50 GB (BDXL)
	MEDIA_BD100 = 4		// Blu-ray: BD-R 100 GB (BDXL)
};

/** Quality preset: adjusts encoder efficiency parameters. */
enum Quality {
	QUALITY_STANDARD = 0,
	QUALITY_HIGH = 1,
	QUALITY_HIGH_PLUS = 2
};

/** Resolved encoder parameter set for the HD pipeline. */
struct Params {
	Mode mode;
	Quality quality;
	int level;			// x264 level * 10 (40 = 4.0, 41 = 4.1)
	double gopSeconds;	// max GOP length in seconds (BD: 2, AVCHD: 1)
	int maxrateKbps;	// VBV max bitrate (spec bound)
	int bufsizeKbps;	// VBV buffer size
	int bframes;
	int refs;
	int slices;
	bool blurayCompat;

	wxString GetLevelStr() const;
};

/** Resolves the user's choices into the encoder parameter set.
  * videoBitrateKbps is the -b:v target and also guards the VBV buffer size. */
Params Resolve(Mode mode, Quality quality, int videoBitrateKbps);

/** Maximum VBV video bitrate allowed for the given mode (kbit/s). */
int MaxVideoBitrateKbps(Mode mode);

/** Usable raw capacity in bytes for a media-size preset. */
double MediaSizeBytes(MediaSize size);

/** Usable raw capacity in GB (for display). */
double MediaSizeGB(MediaSize size);

/** True if the media-size preset can be used with the given authoring mode. */
bool MediaSizeValidForMode(MediaSize size, Mode mode);

/** Default media size for the given authoring mode. */
MediaSize DefaultMediaSize(Mode mode);

/** Rough encoded-size estimate for the given program duration (bytes). */
double EstimateBytes(double totalSeconds, int videoBitrateKbps, int audioBitrateKbps);

}

#endif // DS_HD_PROFILE_H
