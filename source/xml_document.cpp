// xml_document.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml_document.h"

#include "xml_xerces.h"
#include "xml_tinyxml2.h"
#include "xml_pugixml.h"
#include "xml_rapidxml.h"

namespace xge
{
	std::unique_ptr<XmlDocument> XmlDocumentFactory::create(XmlBackend backend)
	{
		switch (backend)
		{
		case XmlBackend::Xerces:   return std::make_unique<XercesXmlDocument>();
		case XmlBackend::TinyXml2: return std::make_unique<TinyXml2Document>();
		case XmlBackend::PugiXml:  return std::make_unique<PugiXmlDocument>();
		case XmlBackend::RapidXml: return std::make_unique<RapidXmlDocument>();
		}

		return std::make_unique<XercesXmlDocument>();
	}
}
