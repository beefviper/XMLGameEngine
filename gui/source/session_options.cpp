// session_options.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "session_options.h"

#include <QCoreApplication>

namespace xge
{
	const std::vector<VideoBackend>& allVideoBackends()
	{
		static const std::vector<VideoBackend> all{
			VideoBackend::Qt, VideoBackend::SFML3, VideoBackend::SDL2, VideoBackend::Raylib, VideoBackend::OpenGL
		};
		return all;
	}

	const std::vector<XmlBackend>& allXmlBackends()
	{
		static const std::vector<XmlBackend> all{
			XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml
		};
		return all;
	}

	const std::vector<AudioBackend>& allAudioBackends()
	{
		static const std::vector<AudioBackend> all{
			AudioBackend::SFML3, AudioBackend::Raylib, AudioBackend::SDL2, AudioBackend::None
		};
		return all;
	}

	QString videoBackendTitle(VideoBackend backend)
	{
		switch (backend)
		{
		case VideoBackend::SFML3:  return QCoreApplication::translate("xge", "SFML 3");
		case VideoBackend::SDL2:   return QCoreApplication::translate("xge", "SDL2");
		case VideoBackend::Raylib: return QCoreApplication::translate("xge", "raylib");
		case VideoBackend::OpenGL: return QCoreApplication::translate("xge", "OpenGL (GLFW)");
		case VideoBackend::Qt:     return QCoreApplication::translate("xge", "Qt (built in)");
		}

		return QString();
	}

	QString xmlBackendTitle(XmlBackend backend)
	{
		switch (backend)
		{
		case XmlBackend::Xerces:   return QCoreApplication::translate("xge", "Xerces");
		case XmlBackend::TinyXml2: return QCoreApplication::translate("xge", "TinyXML2");
		case XmlBackend::PugiXml:  return QCoreApplication::translate("xge", "PugiXML");
		case XmlBackend::RapidXml: return QCoreApplication::translate("xge", "RapidXML");
		}

		return QString();
	}

	QString audioBackendTitle(AudioBackend backend)
	{
		switch (backend)
		{
		case AudioBackend::SFML3:  return QCoreApplication::translate("xge", "SFML 3");
		case AudioBackend::Raylib: return QCoreApplication::translate("xge", "raylib");
		case AudioBackend::SDL2:   return QCoreApplication::translate("xge", "SDL2");
		case AudioBackend::None:   return QCoreApplication::translate("xge", "None (silent)");
		}

		return QString();
	}

	QString videoBackendKey(VideoBackend backend)
	{
		switch (backend)
		{
		case VideoBackend::Qt:     return QStringLiteral("qt");
		case VideoBackend::SFML3:  return QStringLiteral("sfml3");
		case VideoBackend::SDL2:   return QStringLiteral("sdl2");
		case VideoBackend::Raylib: return QStringLiteral("raylib");
		case VideoBackend::OpenGL: return QStringLiteral("opengl");
		}

		return QString();
	}

	QString xmlBackendKey(XmlBackend backend)
	{
		switch (backend)
		{
		case XmlBackend::Xerces:   return QStringLiteral("xerces");
		case XmlBackend::TinyXml2: return QStringLiteral("tinyxml2");
		case XmlBackend::PugiXml:  return QStringLiteral("pugixml");
		case XmlBackend::RapidXml: return QStringLiteral("rapidxml");
		}

		return QString();
	}

	QString audioBackendKey(AudioBackend backend)
	{
		switch (backend)
		{
		case AudioBackend::SFML3:  return QStringLiteral("sfml3");
		case AudioBackend::Raylib: return QStringLiteral("raylib");
		case AudioBackend::SDL2:   return QStringLiteral("sdl2");
		case AudioBackend::None:   return QStringLiteral("none");
		}

		return QString();
	}

	std::optional<VideoBackend> videoBackendFromKey(const QString& key)
	{
		for (const VideoBackend backend : allVideoBackends())
		{
			if (videoBackendKey(backend) == key)
			{
				return backend;
			}
		}

		return std::nullopt;
	}

	std::optional<XmlBackend> xmlBackendFromKey(const QString& key)
	{
		for (const XmlBackend backend : allXmlBackends())
		{
			if (xmlBackendKey(backend) == key)
			{
				return backend;
			}
		}

		return std::nullopt;
	}

	std::optional<AudioBackend> audioBackendFromKey(const QString& key)
	{
		for (const AudioBackend backend : allAudioBackends())
		{
			if (audioBackendKey(backend) == key)
			{
				return backend;
			}
		}

		return std::nullopt;
	}

	WindowBackend libraryBackend(VideoBackend backend)
	{
		switch (backend)
		{
		case VideoBackend::SFML3:  return WindowBackend::SFML3;
		case VideoBackend::SDL2:   return WindowBackend::SDL2;
		case VideoBackend::Raylib: return WindowBackend::Raylib;
		case VideoBackend::OpenGL: return WindowBackend::OpenGL;
		case VideoBackend::Qt:     break;
		}

		return WindowBackend::SFML3;
	}
}
