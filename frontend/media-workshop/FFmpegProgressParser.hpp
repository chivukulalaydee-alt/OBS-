#pragma once

#include <QByteArray>

struct FFmpegProgressUpdate {
	bool valid = false;
	bool finished = false;
	qint64 outputTimeUs = -1;
	double speed = 0.0;
};

class FFmpegProgressParser {
public:
	FFmpegProgressUpdate Feed(const QByteArray &data);
	void Reset();

private:
	FFmpegProgressUpdate ParseLine(const QByteArray &line);

	QByteArray buffer;
	qint64 outputTimeUs = -1;
	double speed = 0.0;
};
