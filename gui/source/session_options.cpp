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
			VideoBackend::SFML3, VideoBackend::SDL2, VideoBackend::Raylib, VideoBackend::OpenGL, VideoBackend::Qt
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
