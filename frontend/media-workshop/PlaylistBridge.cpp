#include "PlaylistBridge.hpp"

#include <QDir>
#include <QFileInfo>
#include <QUuid>

#include <cstring>

#include <obs.h>

namespace {
constexpr const char *SourceId = "media_playlist_source_codeyan";

QString NormalizedPath(const QString &path)
{
	const QFileInfo info(path);
	const QString canonical = info.canonicalFilePath();
	return QDir::cleanPath(canonical.isEmpty() ? info.absoluteFilePath() : canonical);
}

bool EnumeratePlaylistSources(void *data, obs_source_t *source)
{
	auto *names = static_cast<QStringList *>(data);
	const char *id = obs_source_get_unversioned_id(source);
	if (id && strcmp(id, SourceId) == 0)
		names->push_back(QString::fromUtf8(obs_source_get_name(source)));
	return true;
}
}

QStringList PlaylistBridge::SourceNames()
{
	QStringList names;
	obs_enum_sources(EnumeratePlaylistSources, &names);
	names.sort(Qt::CaseInsensitive);
	return names;
}

PlaylistAppendResult PlaylistBridge::Append(const QString &sourceName, const QString &path)
{
	obs_source_t *source = obs_get_source_by_name(sourceName.toUtf8().constData());
	if (!source)
		return PlaylistAppendResult::SourceNotFound;

	const char *id = obs_source_get_unversioned_id(source);
	if (!id || strcmp(id, SourceId) != 0) {
		obs_source_release(source);
		return PlaylistAppendResult::WrongSourceType;
	}

	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_array_t *playlist = obs_data_get_array(settings, "playlist");
	const QString normalized = NormalizedPath(path);
	const size_t count = obs_data_array_count(playlist);
	for (size_t index = 0; index < count; ++index) {
		obs_data_t *item = obs_data_array_item(playlist, index);
		const QString existing = NormalizedPath(QString::fromUtf8(obs_data_get_string(item, "value")));
		obs_data_release(item);
		if (existing.compare(normalized, Qt::CaseInsensitive) == 0) {
			obs_data_array_release(playlist);
			obs_data_release(settings);
			obs_source_release(source);
			return PlaylistAppendResult::AlreadyPresent;
		}
	}

	obs_data_t *item = obs_data_create();
	obs_data_set_string(item, "value", normalized.toUtf8().constData());
	obs_data_set_string(item, "id", QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8().constData());
	obs_data_set_bool(item, "hidden", false);
	obs_data_set_bool(item, "selected", false);
	obs_data_array_push_back(playlist, item);
	obs_data_release(item);
	obs_data_set_array(settings, "playlist", playlist);
	obs_source_update(source, settings);

	obs_data_array_release(playlist);
	obs_data_release(settings);
	obs_source_release(source);
	return PlaylistAppendResult::Added;
}
