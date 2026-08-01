#include "FFmpegCommandBuilder.hpp"

#include <QLocale>
#include <QRegularExpression>
#include <QtMath>

QString FFmpegCommandBuilder::Number(double value)
{
	return QLocale::c().toString(value, 'f', 3).remove(QRegularExpression(QStringLiteral("\\.?0+$")));
}

QString FFmpegCommandBuilder::PositionExpression(MediaWorkshopOverlayPosition position)
{
	switch (position) {
	case MediaWorkshopOverlayPosition::TopLeft:
		return QStringLiteral("20:20");
	case MediaWorkshopOverlayPosition::TopRight:
		return QStringLiteral("main_w-overlay_w-20:20");
	case MediaWorkshopOverlayPosition::BottomLeft:
		return QStringLiteral("20:main_h-overlay_h-20");
	case MediaWorkshopOverlayPosition::BottomRight:
		return QStringLiteral("main_w-overlay_w-20:main_h-overlay_h-20");
	case MediaWorkshopOverlayPosition::Center:
		return QStringLiteral("(main_w-overlay_w)/2:(main_h-overlay_h)/2");
	}

	return QStringLiteral("main_w-overlay_w-20:20");
}

FFmpegCommandBuildResult FFmpegCommandBuilder::Build(const MediaWorkshopJob &job)
{
	FFmpegCommandBuildResult result;
	if (job.inputPath.isEmpty()) {
		result.error = QStringLiteral("input_path_empty");
		return result;
	}
	if (job.temporaryPath.isEmpty()) {
		result.error = QStringLiteral("temporary_path_empty");
		return result;
	}
	if (job.output.width < 16 || job.output.height < 16 || job.output.width % 2 != 0 ||
	    job.output.height % 2 != 0 || job.output.fps < 1 || job.output.fps > 120) {
		result.error = QStringLiteral("invalid_output_size");
		return result;
	}
	if (job.trimStartSeconds < 0.0 || job.trimEndSeconds < 0.0) {
		result.error = QStringLiteral("invalid_trim_range");
		return result;
	}
	if (job.trimEndSeconds > 0.0 && job.trimEndSeconds <= job.trimStartSeconds) {
		result.error = QStringLiteral("invalid_trim_range");
		return result;
	}
	if (job.output.crf < 0 || job.output.crf > 51 || job.output.audioBitrateKbps < 32 ||
	    job.output.audioBitrateKbps > 512) {
		result.error = QStringLiteral("invalid_output_encoding");
		return result;
	}
	if (job.audio.fadeInSeconds < 0.0 || job.audio.fadeOutSeconds < 0.0 || job.audio.volumeDb < -60.0 ||
	    job.audio.volumeDb > 24.0) {
		result.error = QStringLiteral("invalid_audio_settings");
		return result;
	}

	QStringList arguments{QStringLiteral("-hide_banner"), QStringLiteral("-nostdin"), QStringLiteral("-y")};
	if (job.trimStartSeconds > 0.0) {
		arguments << QStringLiteral("-ss") << Number(job.trimStartSeconds);
	}
	arguments << QStringLiteral("-i") << job.inputPath;

	QList<MediaWorkshopOverlay> overlays;
	for (const MediaWorkshopOverlay &overlay : job.overlays) {
		if (!overlay.enabled)
			continue;
		if (overlay.inputPath.isEmpty() || overlay.widthRatio <= 0.0 || overlay.widthRatio > 1.0 ||
		    overlay.opacity < 0.0 || overlay.opacity > 1.0 ||
		    (overlay.endSeconds > 0.0 && overlay.endSeconds <= overlay.startSeconds)) {
			result.error = QStringLiteral("invalid_overlay");
			return result;
		}
		overlays.push_back(overlay);
		if (overlays.size() > 2) {
			result.error = QStringLiteral("too_many_overlays");
			return result;
		}
		arguments << QStringLiteral("-stream_loop") << QStringLiteral("-1") << QStringLiteral("-i")
			  << overlay.inputPath;
	}

	const double sourceDurationSeconds = static_cast<double>(job.sourceDurationMs) / 1000.0;
	const double outputDuration = job.trimEndSeconds > job.trimStartSeconds
					      ? job.trimEndSeconds - job.trimStartSeconds
					      : qMax(0.0, sourceDurationSeconds - job.trimStartSeconds);
	if (outputDuration > 0.0)
		arguments << QStringLiteral("-t") << Number(outputDuration);

	QStringList filters;
	filters << QStringLiteral("[0:v]setpts=PTS-STARTPTS,scale=%1:%2:force_original_aspect_ratio=decrease,"
				  "pad=%1:%2:(ow-iw)/2:(oh-ih)/2:black[base0]")
			.arg(job.output.width)
			.arg(job.output.height);

	QString currentVideo = QStringLiteral("base0");
	for (int index = 0; index < overlays.size(); ++index) {
		const MediaWorkshopOverlay &overlay = overlays.at(index);
		const int inputIndex = index + 1;
		const QString overlayLabel = QStringLiteral("overlay%1").arg(index);
		const QString outputLabel = QStringLiteral("base%1").arg(index + 1);
		filters << QStringLiteral("[%1:v]setpts=PTS-STARTPTS,scale=%2:-2,format=rgba,"
					  "colorchannelmixer=aa=%3[%4]")
				   .arg(inputIndex)
				   .arg(qMax(2, qRound(job.output.width * overlay.widthRatio)) & ~1)
				   .arg(Number(overlay.opacity), overlayLabel);

		QString enableExpression;
		if (overlay.endSeconds > overlay.startSeconds) {
			enableExpression = QStringLiteral("between(t,%1,%2)")
						   .arg(Number(overlay.startSeconds), Number(overlay.endSeconds));
		} else if (overlay.startSeconds > 0.0) {
			enableExpression = QStringLiteral("gte(t,%1)").arg(Number(overlay.startSeconds));
		} else {
			enableExpression = QStringLiteral("gte(t,0)");
		}

		filters << QStringLiteral("[%1][%2]overlay=%3:eof_action=pass:repeatlast=0:enable='%4'[%5]")
				   .arg(currentVideo, overlayLabel, PositionExpression(overlay.position), enableExpression,
					outputLabel);
		currentVideo = outputLabel;
	}

	arguments << QStringLiteral("-filter_complex") << filters.join(QLatin1Char(';')) << QStringLiteral("-map")
		  << QStringLiteral("[%1]").arg(currentVideo);

	if (job.hasAudio) {
		arguments << QStringLiteral("-map") << QStringLiteral("0:a:0?");
		QStringList audioFilters;
		if (!qFuzzyIsNull(job.audio.volumeDb)) {
			audioFilters << QStringLiteral("volume=%1dB").arg(Number(job.audio.volumeDb));
		}
		if (job.audio.fadeInSeconds > 0.0) {
			audioFilters << QStringLiteral("afade=t=in:st=0:d=%1").arg(Number(job.audio.fadeInSeconds));
		}
		if (job.audio.fadeOutSeconds > 0.0 && outputDuration > job.audio.fadeOutSeconds) {
			audioFilters << QStringLiteral("afade=t=out:st=%1:d=%2")
						.arg(Number(outputDuration - job.audio.fadeOutSeconds),
						     Number(job.audio.fadeOutSeconds));
		}
		if (job.audio.loudnessNormalize) {
			audioFilters << QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11");
		}
		if (!audioFilters.isEmpty()) {
			arguments << QStringLiteral("-af") << audioFilters.join(QLatin1Char(','));
		}
	}

	arguments << QStringLiteral("-c:v") << QStringLiteral("libx264") << QStringLiteral("-preset")
		  << QStringLiteral("medium") << QStringLiteral("-crf") << QString::number(job.output.crf)
		  << QStringLiteral("-r") << QString::number(job.output.fps) << QStringLiteral("-pix_fmt")
		  << QStringLiteral("yuv420p");
	if (job.hasAudio) {
		arguments << QStringLiteral("-c:a") << QStringLiteral("aac") << QStringLiteral("-b:a")
			  << QStringLiteral("%1k").arg(job.output.audioBitrateKbps);
	}
	arguments << QStringLiteral("-movflags") << QStringLiteral("+faststart") << QStringLiteral("-progress")
		  << QStringLiteral("pipe:1") << QStringLiteral("-nostats") << job.temporaryPath;

	result.success = true;
	result.arguments = arguments;
	return result;
}
