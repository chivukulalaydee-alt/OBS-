#include "MediaProbe.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
double JsonNumber(const QJsonValue &value)
{
	if (value.isDouble())
		return value.toDouble();
	if (value.isString()) {
		bool ok = false;
		const double number = value.toString().toDouble(&ok);
		return ok ? number : 0.0;
	}
	return 0.0;
}
}

MediaProbeResult MediaProbe::Parse(const QByteArray &json)
{
	MediaProbeResult result;
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
		result.error = QStringLiteral("invalid_probe_json");
		return result;
	}

	const QJsonObject root = document.object();
	const QJsonArray streams = root.value(QStringLiteral("streams")).toArray();
	double streamDuration = 0.0;
	for (const QJsonValue &value : streams) {
		const QJsonObject stream = value.toObject();
		const QString type = stream.value(QStringLiteral("codec_type")).toString();
		result.hasVideo = result.hasVideo || type == QStringLiteral("video");
		result.hasAudio = result.hasAudio || type == QStringLiteral("audio");
		streamDuration = qMax(streamDuration, JsonNumber(stream.value(QStringLiteral("duration"))));
	}

	const double formatDuration = JsonNumber(root.value(QStringLiteral("format")).toObject().value(
		QStringLiteral("duration")));
	const double durationSeconds = qMax(formatDuration, streamDuration);
	result.durationMs = qRound64(durationSeconds * 1000.0);
	if (!result.hasVideo) {
		result.error = QStringLiteral("video_stream_missing");
		return result;
	}
	if (result.durationMs <= 0) {
		result.error = QStringLiteral("duration_missing");
		return result;
	}

	result.success = true;
	return result;
}
