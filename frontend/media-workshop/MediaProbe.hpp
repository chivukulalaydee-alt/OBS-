#pragma once

#include <QByteArray>
#include <QString>

struct MediaProbeResult {
	bool success = false;
	bool hasVideo = false;
	bool hasAudio = false;
	qint64 durationMs = 0;
	QString error;
};

class MediaProbe {
public:
	static MediaProbeResult Parse(const QByteArray &json);
};
