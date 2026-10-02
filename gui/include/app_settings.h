// app_settings.h
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#pragma once

#include <QString>
#include <QVariant>

namespace xge
{
	// What XGEGUI remembers between runs, kept in an ini file next to the
	// program. The file is only written when a setting is changed; a setting
	// that cannot be saved (the folder is read-only) still holds for the run.
	// Besides the settings below it holds where the windows were left (see
	// value and setValue).
	class AppSettings
	{
	public:
		// Reads the settings from `file`, by default xgegui.ini in the folder
		// the program is in.
		explicit AppSettings(const QString& file = defaultFile());

		static QString defaultFile();

		// Whether to ask before the game moves to a window of its own (a video
		// library other than the Qt renderer needs one). Asking is the default.
		// In the file: warn_before_two_windows under [Window].
		bool warnBeforeTwoWindows() const noexcept { return warn; }
		void setWarnBeforeTwoWindows(bool value);

		// Whether a game opened (from the File menu, or the one left open last
		// time) starts playing at once. Not starting is the default: the game
		// waits, paused, so the player can get ready and press Play.
		// In the file: start_game_on_load under [Game].
		bool startGameOnLoad() const noexcept { return startOnLoad; }
		void setStartGameOnLoad(bool value);

		// Any other thing kept, by its name in the file ("Windows/game", say);
		// invalid when the file does not have it.
		QVariant value(const QString& key) const;
		void setValue(const QString& key, const QVariant& value);

	private:
		QString file;
		bool warn{ true };
		bool startOnLoad{ false };
	};
}
