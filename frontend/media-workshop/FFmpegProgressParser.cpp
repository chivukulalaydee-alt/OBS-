#include "FFmpegProgressParser.hpp"

FFmpegProgressUpdate FFmpegProgressParser::Feed(const QByteArray &data)
{
	buffer.append(data);
	FFmpegProgressUpdate latest;

	qsizetype newline = -1;
	while ((newline = buffer.indexOf('\n')) >= 0) {
		QByteArray line = buffer.left(newline).trimmed();
		buffer.remove(0, newline + 1);
		FFmpegProgressUpdate update = ParseLine(line);
		if (update.valid)
			latest = update;
	}

	return latest;
}

void FFmpegProgressParser::Reset()
{
	buffer.clear();
	outputTimeUs = -1;
	speed = 0.0;
}

FFmpegProgressUpdate FFmpegProgressParser::ParseLine(const QByteArray &line)
{
	FFmpegProgressUpdate update;
	const qsizetype separator = line.indexOf('=');
	if (separator <= 0)
		return update;

	const QByteArray key = line.left(separator);
	const QByteArray value = line.mid(separator + 1);
	bool ok = false;
	if (key == "out_time_us" || key == "out_time_ms") {
		const qint64 parsed = value.toLongLong(&ok);
		if (ok)
			outputTimeUs = parsed;
	} else if (key == "speed") {
		QByteArray normalized = value;
		if (normalized.endsWith('x'))
			normalized.chop(1);
		const double parsed = normalized.toDouble(&ok);
		if (ok)
			speed = parsed;
	} else if (key == "progress") {
		update.valid = true;
		update.finished = value == "end";
		update.outputTimeUs = outputTimeUs;
		update.speed = speed;
	}

	return update;
}
