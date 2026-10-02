// app_settings.cpp
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#include "app_settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace xge
{
	namespace
	{
		const QString kWarnKey = QStringLiteral("Window/warn_before_two_windows");
	}

	AppSettings::AppSettings(const QString& file) :
		file(file)
	{
		// Reading a file that is not there makes none.
		if (QFileInfo::exists(file))
		{
			warn = QSettings(file, QSettings::IniFormat).value(kWarnKey, true).toBool();
		}
	}

	QString AppSettings::defaultFile()
	{
		return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("xgegui.ini"));
	}

	void AppSettings::setWarnBeforeTwoWindows(bool value)
	{
		if (value == warn)
		{
			return;
		}

		warn = value;

		setValue(kWarnKey, value);
	}

	QVariant AppSettings::value(const QString& key) const
	{
		return QFileInfo::exists(file) ? QSettings(file, QSettings::IniFormat).value(key) : QVariant();
	}

	void AppSettings::setValue(const QString& key, const QVariant& value)
	{
		if (this->value(key) == value)
		{
			return;
		}

		QSettings settings(file, QSettings::IniFormat);
		settings.setValue(key, value);
		settings.sync();
	}
}
