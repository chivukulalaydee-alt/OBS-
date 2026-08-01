#include "MediaWorkshopTools.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

bool MediaWorkshopTools::IsValid() const
{
	return QFileInfo(ffmpeg).isExecutable() && QFileInfo(ffprobe).isExecutable();
}

MediaWorkshopTools MediaWorkshopTools::Locate()
{
	MediaWorkshopTools tools;
	const QDir executableDirectory(QCoreApplication::applicationDirPath());
	const QString bundledRoot = executableDirectory.absoluteFilePath(
		QStringLiteral("../../data/obs-studio/media-workshop/bin"));
	const QString bundledFFmpeg = QDir(bundledRoot).filePath(QStringLiteral("ffmpeg.exe"));
	const QString bundledFFprobe = QDir(bundledRoot).filePath(QStringLiteral("ffprobe.exe"));
	if (QFileInfo(bundledFFmpeg).isExecutable() && QFileInfo(bundledFFprobe).isExecutable()) {
		tools.ffmpeg = QDir::cleanPath(bundledFFmpeg);
		tools.ffprobe = QDir::cleanPath(bundledFFprobe);
		return tools;
	}

	tools.ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
	tools.ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
	return tools;
}
