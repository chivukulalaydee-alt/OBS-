#include "../FFmpegCommandBuilder.hpp"
#include "../FFmpegProgressParser.hpp"
#include "../MediaProbe.hpp"

#include <QCoreApplication>
#include <QDebug>

namespace {
int failures = 0;

void Check(bool condition, const char *message)
{
	if (!condition) {
		qCritical().noquote() << "FAIL:" << message;
		++failures;
	}
}
}

int main(int argc, char **argv)
{
	QCoreApplication application(argc, argv);

	MediaWorkshopJob job;
	job.inputPath = QString::fromUtf8("D:/测试 素材/(主视频).mp4");
	job.temporaryPath = QString::fromUtf8("D:/输出 目录/结果.part.mp4");
	job.trimStartSeconds = 1.25;
	job.trimEndSeconds = 9.75;
	job.sourceDurationMs = 12000;
	job.audio.volumeDb = -3.0;
	job.audio.fadeInSeconds = 0.5;
	job.audio.fadeOutSeconds = 1.0;
	job.audio.loudnessNormalize = true;

	MediaWorkshopOverlay firstOverlay;
	firstOverlay.enabled = true;
	firstOverlay.inputPath = QString::fromUtf8("D:/叠加/第一层.mp4");
	firstOverlay.opacity = 0.5;
	firstOverlay.widthRatio = 0.3;
	firstOverlay.startSeconds = 1.0;
	firstOverlay.endSeconds = 5.0;
	job.overlays.push_back(firstOverlay);

	MediaWorkshopOverlay secondOverlay;
	secondOverlay.enabled = true;
	secondOverlay.inputPath = QString::fromUtf8("D:/叠加/第二层.mp4");
	secondOverlay.position = MediaWorkshopOverlayPosition::BottomLeft;
	secondOverlay.opacity = 0.8;
	job.overlays.push_back(secondOverlay);

	const FFmpegCommandBuildResult build = FFmpegCommandBuilder::Build(job);
	Check(build.success, "valid job should build");
	Check(build.arguments.contains(job.inputPath), "Unicode input path should remain one argument");
	Check(build.arguments.contains(firstOverlay.inputPath), "first overlay path should remain one argument");
	Check(build.arguments.contains(secondOverlay.inputPath), "second overlay path should remain one argument");
	Check(!build.arguments.contains(QStringLiteral("cmd.exe")), "command must not use cmd.exe");
	Check(build.arguments.contains(QStringLiteral("-filter_complex")), "video filter graph should be present");
	Check(build.arguments.join(QLatin1Char(' ')).contains(QStringLiteral("colorchannelmixer=aa=0.5")),
	      "overlay opacity should be encoded");
	Check(build.arguments.join(QLatin1Char(' ')).contains(QStringLiteral("scale=384:-2")),
	      "overlay width should be relative to output width");
	Check(build.arguments.join(QLatin1Char(' ')).contains(QStringLiteral("loudnorm=I=-16")),
	      "loudness normalization should be encoded");
	Check(build.arguments.contains(QStringLiteral("30")), "frame rate should be encoded");

	MediaWorkshopJob invalid = job;
	invalid.trimEndSeconds = invalid.trimStartSeconds;
	Check(!FFmpegCommandBuilder::Build(invalid).success, "invalid trim range should fail");
	invalid = job;
	invalid.trimStartSeconds = -1.0;
	Check(FFmpegCommandBuilder::Build(invalid).error == QStringLiteral("invalid_trim_range"),
	      "negative trim should fail with a stable error code");
	invalid = job;
	invalid.output.crf = 52;
	Check(FFmpegCommandBuilder::Build(invalid).error == QStringLiteral("invalid_output_encoding"),
	      "out-of-range CRF should fail");
	invalid = job;
	invalid.output.fps = 0;
	Check(FFmpegCommandBuilder::Build(invalid).error == QStringLiteral("invalid_output_size"),
	      "out-of-range frame rate should fail");
	invalid = job;
	invalid.trimStartSeconds = 1.0;
	invalid.trimEndSeconds = 0.0;
	const FFmpegCommandBuildResult bounded = FFmpegCommandBuilder::Build(invalid);
	const int durationIndex = bounded.arguments.indexOf(QStringLiteral("-t"));
	Check(durationIndex >= 0 && bounded.arguments.value(durationIndex + 1) == QStringLiteral("11"),
	      "source duration should bound looping overlays when trim end is omitted");
	invalid = job;
	invalid.overlays.push_back(firstOverlay);
	Check(FFmpegCommandBuilder::Build(invalid).error == QStringLiteral("too_many_overlays"),
	      "more than two enabled overlays should fail");

	FFmpegProgressParser parser;
	FFmpegProgressUpdate update = parser.Feed("out_time_us=2500000\nspeed=1.25x\npro");
	Check(!update.valid, "partial progress record should wait for completion");
	update = parser.Feed("gress=continue\n");
	Check(update.valid && !update.finished, "continue progress should parse");
	Check(update.outputTimeUs == 2500000, "progress timestamp should parse");
	Check(qAbs(update.speed - 1.25) < 0.001, "progress speed should parse");
	update = parser.Feed("garbage\nprogress=end\n");
	Check(update.valid && update.finished, "end progress should parse after malformed input");

	const QByteArray probeJson = R"({"streams":[{"codec_type":"video","duration":"8.5"},{"codec_type":"audio"}],"format":{"duration":"9.25"}})";
	const MediaProbeResult probe = MediaProbe::Parse(probeJson);
	Check(probe.success && probe.hasVideo && probe.hasAudio, "valid probe JSON should expose streams");
	Check(probe.durationMs == 9250, "format duration should be parsed in milliseconds");
	Check(MediaProbe::Parse("not-json").error == QStringLiteral("invalid_probe_json"),
	      "malformed probe JSON should fail predictably");
	Check(MediaProbe::Parse(R"({"streams":[{"codec_type":"audio"}],"format":{"duration":"1"}})")
		      .error == QStringLiteral("video_stream_missing"),
	      "audio-only input should be rejected");

	if (failures == 0)
		qInfo() << "All media workshop logic tests passed";
	return failures == 0 ? 0 : 1;
}
