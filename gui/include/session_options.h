// session_options.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window.h"
#include "xml_document.h"

#include <QString>

#include <optional>
#include <vector>

namespace xge
{
	// How the game is drawn: with the application's own renderer (QtWindow,
	// qt_window.h), which draws with Qt in the main window and needs no window
	// library, or with one of the library's window backends, each in a window of
	// its own.
	enum class VideoBackend
	{
		Qt,
		SFML3,
		SDL2,
		Raylib,
		OpenGL
	};

	// What the Options dialog chooses. The Qt renderer and Xerces are what is
	// used unless something else is asked for.
	struct SessionOptions
	{
		VideoBackend video{ VideoBackend::Qt };
		XmlBackend xml{ XmlBackend::Xerces };

		bool operator==(const SessionOptions& other) const noexcept
		{
			return video == other.video && xml == other.xml;
		}
	};

	// Every choice, in the order the dropdowns list them (the default first).
	const std::vector<VideoBackend>& allVideoBackends();
	const std::vector<XmlBackend>& allXmlBackends();

	// The name shown for a choice.
	QString videoBackendTitle(VideoBackend backend);
	QString xmlBackendTitle(XmlBackend backend);

	// A choice's name in the settings file: plain, and never changed, unlike
	// its title or its place in the enum. The reverse is empty for a name that
	// is no choice.
	QString videoBackendKey(VideoBackend backend);
	QString xmlBackendKey(XmlBackend backend);
	std::optional<VideoBackend> videoBackendFromKey(const QString& key);
	std::optional<XmlBackend> xmlBackendFromKey(const QString& key);

	// The library backend a VideoBackend stands for (not for VideoBackend::Qt).
	WindowBackend libraryBackend(VideoBackend backend);
}
