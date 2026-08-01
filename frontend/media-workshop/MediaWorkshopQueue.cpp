#include "MediaWorkshopQueue.hpp"

#include "moc_MediaWorkshopQueue.cpp"

MediaWorkshopQueue::MediaWorkshopQueue(QObject *parent) : QObject(parent), runner(this)
{
	connect(&runner, &FFmpegRunner::JobUpdated, this, &MediaWorkshopQueue::JobUpdated);
	connect(&runner, &FFmpegRunner::JobFinished, this, &MediaWorkshopQueue::RunnerFinished);
}

bool MediaWorkshopQueue::IsBusy() const
{
	return runner.IsRunning() || !pendingJobs.isEmpty();
}

void MediaWorkshopQueue::Enqueue(const QList<MediaWorkshopJob> &jobs)
{
	const bool wasBusy = IsBusy();
	cancellingAll = false;
	for (const MediaWorkshopJob &job : jobs)
		pendingJobs.enqueue(job);
	if (!wasBusy && IsBusy())
		emit QueueStateChanged(true);
	StartNext();
}

void MediaWorkshopQueue::CancelCurrent()
{
	runner.Cancel();
}

void MediaWorkshopQueue::CancelAll()
{
	cancellingAll = true;
	while (!pendingJobs.isEmpty()) {
		MediaWorkshopJob job = pendingJobs.dequeue();
		job.state = MediaWorkshopJobState::Cancelled;
		job.errorCode = QStringLiteral("CANCELLED");
		emit JobUpdated(job);
		emit JobFinished(job);
	}
	if (runner.IsRunning())
		runner.Cancel();
	else
		emit QueueStateChanged(false);
}

void MediaWorkshopQueue::StartNext()
{
	if (runner.IsRunning())
		return;
	if (pendingJobs.isEmpty()) {
		emit QueueStateChanged(false);
		return;
	}

	tools = MediaWorkshopTools::Locate();
	MediaWorkshopJob job = pendingJobs.dequeue();
	if (!tools.IsValid()) {
		job.state = MediaWorkshopJobState::Failed;
		job.errorCode = QStringLiteral("FFMPEG_NOT_FOUND");
		job.errorSummary = QStringLiteral("ffmpeg_and_ffprobe_are_required");
		emit JobUpdated(job);
		emit JobFinished(job);
		QMetaObject::invokeMethod(this, &MediaWorkshopQueue::StartNext, Qt::QueuedConnection);
		return;
	}
	runner.Start(job, tools);
}

void MediaWorkshopQueue::RunnerFinished(const MediaWorkshopJob &job)
{
	emit JobFinished(job);
	if (cancellingAll && pendingJobs.isEmpty())
		cancellingAll = false;
	QMetaObject::invokeMethod(this, &MediaWorkshopQueue::StartNext, Qt::QueuedConnection);
}
