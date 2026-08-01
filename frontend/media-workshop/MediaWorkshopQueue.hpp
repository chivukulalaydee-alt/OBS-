#pragma once

#include "FFmpegRunner.hpp"

#include <QObject>
#include <QQueue>

class MediaWorkshopQueue : public QObject {
	Q_OBJECT

public:
	explicit MediaWorkshopQueue(QObject *parent = nullptr);

	bool IsBusy() const;
	void Enqueue(const QList<MediaWorkshopJob> &jobs);
	void CancelCurrent();
	void CancelAll();

signals:
	void JobUpdated(const MediaWorkshopJob &job);
	void JobFinished(const MediaWorkshopJob &job);
	void QueueStateChanged(bool busy);

private:
	void StartNext();
	void RunnerFinished(const MediaWorkshopJob &job);

	QQueue<MediaWorkshopJob> pendingJobs;
	FFmpegRunner runner;
	MediaWorkshopTools tools;
	bool cancellingAll = false;
};
