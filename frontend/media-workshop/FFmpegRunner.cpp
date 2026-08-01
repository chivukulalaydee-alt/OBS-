#include "FFmpegRunner.hpp"

#include "FFmpegCommandBuilder.hpp"
#include "MediaProbe.hpp"

#include <QFile>
#include <QFileInfo>

#include <util/base.h>

#include "moc_FFmpegRunner.cpp"

namespace {
QString ErrorSummary(const QByteArray &data)
{
	QString text = QString::fromUtf8(data).trimmed();
	if (text.size() > 800)
		text = text.right(800);
	return text;
}
}

FFmpegRunner::FFmpegRunner(QObject *parent) : QObject(parent)
{
	process.setProcessChannelMode(QProcess::SeparateChannels);
	killTimer.setSingleShot(true);
	connect(&process, &QProcess::readyReadStandardOutput, this, &FFmpegRunner::ReadStandardOutput);
	connect(&process, &QProcess::readyReadStandardError, this, &FFmpegRunner::ReadStandardError);
	connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
		&FFmpegRunner::ProcessFinished);
	connect(&process, &QProcess::errorOccurred, this, &FFmpegRunner::ProcessError);
	connect(&killTimer, &QTimer::timeout, this, [this]() {
		if (process.state() != QProcess::NotRunning)
			process.kill();
	});
}

FFmpegRunner::~FFmpegRunner()
{
	if (process.state() != QProcess::NotRunning) {
		process.kill();
		process.waitForFinished(3000);
	}
	CleanupTemporaryFile();
}

bool FFmpegRunner::IsRunning() const
{
	return stage != Stage::Idle;
}

void FFmpegRunner::Start(const MediaWorkshopJob &newJob, const MediaWorkshopTools &newTools)
{
	if (IsRunning())
		return;

	job = newJob;
	tools = newTools;
	cancellationRequested = false;
	finishing = false;
	job.progress = 0.0;
	job.errorCode.clear();
	job.errorSummary.clear();
	job.state = MediaWorkshopJobState::Probing;
	emit JobUpdated(job);
	blog(LOG_INFO, "[media-workshop] [%s] input probe started", job.id.toString(QUuid::WithoutBraces).toUtf8().constData());
	StartProbe(job.inputPath, Stage::ProbingInput);
}

void FFmpegRunner::Cancel()
{
	if (!IsRunning() || cancellationRequested)
		return;

	cancellationRequested = true;
	job.state = MediaWorkshopJobState::Cancelling;
	emit JobUpdated(job);
	if (process.state() != QProcess::NotRunning) {
		process.terminate();
		killTimer.start(3000);
	} else {
		FinishCancelled();
	}
}

void FFmpegRunner::StartProbe(const QString &path, Stage nextStage)
{
	stage = nextStage;
	ResetProcessBuffers();
	process.setProgram(tools.ffprobe);
	process.setArguments({QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-print_format"),
			      QStringLiteral("json"), QStringLiteral("-show_format"), QStringLiteral("-show_streams"), path});
	process.start();
}

void FFmpegRunner::StartEncoding()
{
	const FFmpegCommandBuildResult command = FFmpegCommandBuilder::Build(job);
	if (!command.success) {
		Fail(QStringLiteral("INVALID_SETTINGS"), command.error);
		return;
	}

	stage = Stage::Encoding;
	job.state = MediaWorkshopJobState::Running;
	emit JobUpdated(job);
	ResetProcessBuffers();
	progressParser.Reset();
	process.setProgram(tools.ffmpeg);
	process.setArguments(command.arguments);
	blog(LOG_INFO, "[media-workshop] [%s] encoding started", job.id.toString(QUuid::WithoutBraces).toUtf8().constData());
	process.start();
}

void FFmpegRunner::ProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	killTimer.stop();
	ReadStandardOutput();
	ReadStandardError();
	if (finishing)
		return;
	if (cancellationRequested) {
		FinishCancelled();
		return;
	}
	if (exitStatus != QProcess::NormalExit || exitCode != 0) {
		const QString code = stage == Stage::Encoding ? QStringLiteral("ENCODE_FAILED")
							      : QStringLiteral("PROBE_FAILED");
		Fail(code, ErrorSummary(standardError));
		return;
	}

	if (stage == Stage::ProbingInput) {
		const MediaProbeResult probe = MediaProbe::Parse(standardOutput);
		if (!probe.success) {
			Fail(QStringLiteral("INVALID_INPUT"), probe.error);
			return;
		}
		job.sourceDurationMs = probe.durationMs;
		job.hasAudio = probe.hasAudio;
		StartEncoding();
		return;
	}

	if (stage == Stage::Encoding) {
		if (!QFileInfo::exists(job.temporaryPath)) {
			Fail(QStringLiteral("OUTPUT_MISSING"), QStringLiteral("temporary_output_missing"));
			return;
		}
		job.state = MediaWorkshopJobState::Validating;
		emit JobUpdated(job);
		StartProbe(job.temporaryPath, Stage::ProbingOutput);
		return;
	}

	if (stage == Stage::ProbingOutput) {
		const MediaProbeResult probe = MediaProbe::Parse(standardOutput);
		if (!probe.success) {
			Fail(QStringLiteral("OUTPUT_INVALID"), probe.error);
			return;
		}
		FinishSuccess();
	}
}

void FFmpegRunner::ProcessError(QProcess::ProcessError error)
{
	if (error != QProcess::FailedToStart || finishing || cancellationRequested)
		return;
	Fail(QStringLiteral("TOOL_START_FAILED"), process.errorString());
}

void FFmpegRunner::ReadStandardOutput()
{
	const QByteArray data = process.readAllStandardOutput();
	if (stage == Stage::Encoding) {
		const FFmpegProgressUpdate update = progressParser.Feed(data);
		if (update.valid) {
			const qint64 durationUs = job.trimEndSeconds > job.trimStartSeconds
						      ? qRound64((job.trimEndSeconds - job.trimStartSeconds) * 1000000.0)
						      : job.sourceDurationMs * 1000;
			if (durationUs > 0 && update.outputTimeUs >= 0)
				job.progress = qBound(0.0, static_cast<double>(update.outputTimeUs) / durationUs, 1.0);
			if (update.finished)
				job.progress = 1.0;
			emit JobUpdated(job);
		}
	} else {
		standardOutput.append(data);
	}
}

void FFmpegRunner::ReadStandardError()
{
	standardError.append(process.readAllStandardError());
	if (standardError.size() > 65536)
		standardError = standardError.right(65536);
}

void FFmpegRunner::Fail(const QString &code, const QString &summary)
{
	if (finishing)
		return;
	finishing = true;
	CleanupTemporaryFile();
	job.state = MediaWorkshopJobState::Failed;
	job.errorCode = code;
	job.errorSummary = summary.isEmpty() ? code : summary;
	stage = Stage::Idle;
	blog(LOG_WARNING, "[media-workshop] [%s] failed: %s", job.id.toString(QUuid::WithoutBraces).toUtf8().constData(),
	     code.toUtf8().constData());
	emit JobUpdated(job);
	emit JobFinished(job);
}

void FFmpegRunner::FinishCancelled()
{
	if (finishing)
		return;
	finishing = true;
	CleanupTemporaryFile();
	job.state = MediaWorkshopJobState::Cancelled;
	job.errorCode = QStringLiteral("CANCELLED");
	job.errorSummary.clear();
	stage = Stage::Idle;
	blog(LOG_INFO, "[media-workshop] [%s] cancelled", job.id.toString(QUuid::WithoutBraces).toUtf8().constData());
	emit JobUpdated(job);
	emit JobFinished(job);
}

void FFmpegRunner::FinishSuccess()
{
	if (finishing)
		return;
	finishing = true;
	if (QFileInfo::exists(job.outputPath) || !QFile::rename(job.temporaryPath, job.outputPath)) {
		finishing = false;
		Fail(QStringLiteral("OUTPUT_FINALIZE_FAILED"), QStringLiteral("atomic_rename_failed"));
		return;
	}
	job.progress = 1.0;
	job.state = MediaWorkshopJobState::Succeeded;
	stage = Stage::Idle;
	blog(LOG_INFO, "[media-workshop] [%s] completed", job.id.toString(QUuid::WithoutBraces).toUtf8().constData());
	emit JobUpdated(job);
	emit JobFinished(job);
}

void FFmpegRunner::CleanupTemporaryFile()
{
	if (!job.temporaryPath.isEmpty() && QFileInfo::exists(job.temporaryPath) && !QFile::remove(job.temporaryPath))
		blog(LOG_WARNING, "[media-workshop] [%s] temporary output cleanup failed",
		     job.id.toString(QUuid::WithoutBraces).toUtf8().constData());
}

void FFmpegRunner::ResetProcessBuffers()
{
	standardOutput.clear();
	standardError.clear();
}
