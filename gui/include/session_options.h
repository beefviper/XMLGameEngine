// session_options.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "audio.h"
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

	// What the Options dialog chooses. The Qt renderer, Xerces and SFML 3's
	// sound are what is used unless something else is asked for, or the first
	// of the others that the program was built with when they were left out.
	struct SessionOptions
	{
		VideoBackend video{ VideoBackend::Qt };
		XmlBackend xml{ XmlDocumentFactory::defaultBackend() };
		AudioBackend audio{ AudioFactory::defaultBackend() };

		bool operator==(const SessionOptions& other) const noexcept
		{
			return video == other.video && xml == other.xml && audio == other.audio;
		}
	};

	// Every choice the program was built with, in the order the dropdowns list
	// them (the default first).
	const std::vector<VideoBackend>& allVideoBackends();
	const std::vector<XmlBackend>& allXmlBackends();
	const std::vector<AudioBackend>& allAudioBackends();

	// The name shown for a choice.
	QString videoBackendTitle(VideoBackend backend);
	QString xmlBackendTitle(XmlBackend backend);
	QString audioBackendTitle(AudioBackend backend);

	// A choice's name in the settings file: plain, and never changed, unlike
	// its title or its place in the enum. The reverse is empty for a name that
	// is no choice.
	QString videoBackendKey(VideoBackend backend);
	QString xmlBackendKey(XmlBackend backend);
	QString audioBackendKey(AudioBackend backend);
	std::optional<VideoBackend> videoBackendFromKey(const QString& key);
	std::optional<XmlBackend> xmlBackendFromKey(const QString& key);
	std::optional<AudioBackend> audioBackendFromKey(const QString& key);

	// The library backend a VideoBackend stands for (not for VideoBackend::Qt).
	WindowBackend libraryBackend(VideoBackend backend);
}
