#pragma once

#include <QString>

struct MediaWorkshopTools {
	QString ffmpeg;
	QString ffprobe;

	bool IsValid() const;
	static MediaWorkshopTools Locate();
};
