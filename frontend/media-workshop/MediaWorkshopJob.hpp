#pragma once

#include <QList>
#include <QString>
#include <QUuid>

enum class MediaWorkshopJobState {
	Pending,
	Probing,
	Running,
	Validating,
	Succeeded,
	Failed,
	Cancelling,
	Cancelled,
};

enum class MediaWorkshopOverlayPosition {
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	Center,
};

struct MediaWorkshopOverlay {
	bool enabled = false;
	QString inputPath;
	double widthRatio = 0.25;
	double opacity = 1.0;
	MediaWorkshopOverlayPosition position = MediaWorkshopOverlayPosition::TopRight;
	double startSeconds = 0.0;
	double endSeconds = 0.0;
};

struct MediaWorkshopAudioSettings {
	double volumeDb = 0.0;
	double fadeInSeconds = 0.0;
	double fadeOutSeconds = 0.0;
	bool loudnessNormalize = false;
};

struct MediaWorkshopOutputSettings {
	int width = 1280;
	int height = 720;
	int fps = 30;
	int crf = 23;
	int audioBitrateKbps = 192;
};

struct MediaWorkshopJob {
	QUuid id = QUuid::createUuid();
	QString inputPath;
	QString outputPath;
	QString temporaryPath;
	double trimStartSeconds = 0.0;
	double trimEndSeconds = 0.0;
	QList<MediaWorkshopOverlay> overlays;
	MediaWorkshopAudioSettings audio;
	MediaWorkshopOutputSettings output;
	MediaWorkshopJobState state = MediaWorkshopJobState::Pending;
	qint64 sourceDurationMs = 0;
	bool hasAudio = true;
	double progress = 0.0;
	QString errorCode;
	QString errorSummary;
};

inline bool MediaWorkshopJobIsTerminal(MediaWorkshopJobState state)
{
	return state == MediaWorkshopJobState::Succeeded || state == MediaWorkshopJobState::Failed ||
	       state == MediaWorkshopJobState::Cancelled;
}
