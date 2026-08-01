#include "../MediaWorkshopQueue.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

int main(int argc, char **argv)
{
	QCoreApplication application(argc, argv);
	if (application.arguments().size() < 4)
		return 2;

	const QString mode = application.arguments().at(1);
	const bool successMode = mode.startsWith(QStringLiteral("success"));
	MediaWorkshopJob job;
	job.inputPath = application.arguments().at(2);
	job.outputPath = application.arguments().at(3);
	job.temporaryPath = QFileInfo(job.outputPath).dir().filePath(
		QStringLiteral(".%1.runner.part.mp4").arg(QFileInfo(job.outputPath).completeBaseName()));
	job.output.width = 640;
	job.output.height = 360;
	job.output.crf = 25;
	job.trimStartSeconds = mode == QStringLiteral("success-trim") ? 1.0 : 0.0;
	job.trimEndSeconds = mode == QStringLiteral("success-trim") ? 5.0 : 0.0;
	job.audio.volumeDb = -2.0;
	job.audio.fadeInSeconds = 0.25;
	job.audio.fadeOutSeconds = 0.5;
	job.audio.loudnessNormalize = true;

	if (successMode && application.arguments().size() >= 5) {
		MediaWorkshopOverlay overlay;
		overlay.enabled = true;
		overlay.inputPath = application.arguments().at(4);
		overlay.widthRatio = 0.3;
		overlay.opacity = 0.5;
		overlay.position = MediaWorkshopOverlayPosition::TopRight;
		overlay.startSeconds = 0.5;
		overlay.endSeconds = 3.0;
		job.overlays.push_back(overlay);
	}
	if (successMode && application.arguments().size() >= 6) {
		MediaWorkshopOverlay overlay;
		overlay.enabled = true;
		overlay.inputPath = application.arguments().at(5);
		overlay.widthRatio = 0.25;
		overlay.opacity = 0.8;
		overlay.position = MediaWorkshopOverlayPosition::BottomLeft;
		job.overlays.push_back(overlay);
	}

	QFile::remove(job.outputPath);
	QFile::remove(job.temporaryPath);
	MediaWorkshopQueue queue;
	MediaWorkshopJob finishedJob;
	bool receivedResult = false;
	QObject::connect(&queue, &MediaWorkshopQueue::JobFinished, &application,
			 [&finishedJob, &receivedResult](const MediaWorkshopJob &result) {
				 finishedJob = result;
				 receivedResult = true;
			 });
	QObject::connect(&queue, &MediaWorkshopQueue::QueueStateChanged, &application, [&application](bool busy) {
		if (!busy)
			application.quit();
	});
	QTimer::singleShot(45000, &application, &QCoreApplication::quit);
	queue.Enqueue({job});
	if (mode == QStringLiteral("cancel"))
		QTimer::singleShot(100, &queue, &MediaWorkshopQueue::CancelCurrent);
	application.exec();

	if (!receivedResult)
		return 3;
	if (successMode)
		return finishedJob.state == MediaWorkshopJobState::Succeeded && QFileInfo::exists(job.outputPath) &&
			       !QFileInfo::exists(job.temporaryPath)
			       ? 0
			       : 4;
	if (mode == QStringLiteral("failure"))
		return finishedJob.state == MediaWorkshopJobState::Failed && !QFileInfo::exists(job.outputPath) &&
			       !QFileInfo::exists(job.temporaryPath)
			       ? 0
			       : 5;
	if (mode == QStringLiteral("cancel"))
		return finishedJob.state == MediaWorkshopJobState::Cancelled && !QFileInfo::exists(job.outputPath) &&
			       !QFileInfo::exists(job.temporaryPath)
			       ? 0
			       : 6;
	return 7;
}
