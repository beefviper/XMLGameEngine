// session_options.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window.h"
#include "xml_document.h"

#include <QString>

#include <vector>

namespace xge
{
	// How the game is drawn: with one of the library's window backends, or with
	// the application's own renderer (QtWindow, qt_window.h), which draws with
	// Qt and needs no window library.
	enum class VideoBackend
	{
		SFML3,
		SDL2,
		Raylib,
		OpenGL,
		Qt
	};

	// What the Options dialog chooses. Like the command line, SFML3 and Xerces
	// are what is used unless something else is asked for.
	struct SessionOptions
	{
		VideoBackend video{ VideoBackend::SFML3 };
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

	// The library backend a VideoBackend stands for (not for VideoBackend::Qt).
	WindowBackend libraryBackend(VideoBackend backend);
}
