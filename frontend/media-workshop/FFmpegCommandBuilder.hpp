#pragma once

#include "MediaWorkshopJob.hpp"

#include <QString>
#include <QStringList>

struct FFmpegCommandBuildResult {
	bool success = false;
	QString error;
	QStringList arguments;
};

class FFmpegCommandBuilder {
public:
	static FFmpegCommandBuildResult Build(const MediaWorkshopJob &job);
	static QString PositionExpression(MediaWorkshopOverlayPosition position);

private:
	static QString Number(double value);
};
