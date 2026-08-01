#pragma once

#include "FFmpegProgressParser.hpp"
#include "MediaWorkshopJob.hpp"
#include "MediaWorkshopTools.hpp"

#include <QObject>
#include <QProcess>
#include <QTimer>

class FFmpegRunner : public QObject {
	Q_OBJECT

public:
	explicit FFmpegRunner(QObject *parent = nullptr);
	~FFmpegRunner() override;

	bool IsRunning() const;
	void Start(const MediaWorkshopJob &job, const MediaWorkshopTools &tools);
	void Cancel();

signals:
	void JobUpdated(const MediaWorkshopJob &job);
	void JobFinished(const MediaWorkshopJob &job);

private:
	enum class Stage { Idle, ProbingInput, Encoding, ProbingOutput };

	void StartProbe(const QString &path, Stage nextStage);
	void StartEncoding();
	void ProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void ProcessError(QProcess::ProcessError error);
	void ReadStandardOutput();
	void ReadStandardError();
	void Fail(const QString &code, const QString &summary);
	void FinishCancelled();
	void FinishSuccess();
	void CleanupTemporaryFile();
	void ResetProcessBuffers();

	QProcess process;
	QTimer killTimer;
	FFmpegProgressParser progressParser;
	MediaWorkshopJob job;
	MediaWorkshopTools tools;
	Stage stage = Stage::Idle;
	QByteArray standardOutput;
	QByteArray standardError;
	bool cancellationRequested = false;
	bool finishing = false;
};
