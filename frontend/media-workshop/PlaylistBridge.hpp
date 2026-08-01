#pragma once

#include <QString>
#include <QStringList>

enum class PlaylistAppendResult { Added, AlreadyPresent, SourceNotFound, WrongSourceType, UpdateFailed };

class PlaylistBridge {
public:
	static QStringList SourceNames();
	static PlaylistAppendResult Append(const QString &sourceName, const QString &path);
};
