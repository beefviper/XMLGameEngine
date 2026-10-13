// xml_document.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "xml_document.h"

#include <stdexcept>

// Each backend is compiled in only when it is built (XGE_WITH_<NAME>, set by
// scripts/cmake/targets.cmake), so its header, which includes its library's, is
// only included then.
#ifdef XGE_WITH_XERCES
#include "xml_xerces.h"
#endif
#ifdef XGE_WITH_TINYXML2
#include "xml_tinyxml2.h"
#endif
#ifdef XGE_WITH_PUGIXML
#include "xml_pugixml.h"
#endif
#ifdef XGE_WITH_RAPIDXML
#include "xml_rapidxml.h"
#endif

namespace xge
{
	namespace
	{
		struct Entry
		{
			XmlBackend backend;
			const char* name;
			bool built;
		};

		// In XmlBackend's order, which is the order of preference for the default.
		const Entry entries[] = {
#ifdef XGE_WITH_XERCES
			{ XmlBackend::Xerces, "xerces", true },
#else
			{ XmlBackend::Xerces, "xerces", false },
#endif
#ifdef XGE_WITH_TINYXML2
			{ XmlBackend::TinyXml2, "tinyxml2", true },
#else
			{ XmlBackend::TinyXml2, "tinyxml2", false },
#endif
#ifdef XGE_WITH_PUGIXML
			{ XmlBackend::PugiXml, "pugixml", true },
#else
			{ XmlBackend::PugiXml, "pugixml", false },
#endif
#ifdef XGE_WITH_RAPIDXML
			{ XmlBackend::RapidXml, "rapidxml", true },
#else
			{ XmlBackend::RapidXml, "rapidxml", false },
#endif
		};
	}

	std::string XmlDocumentFactory::name(XmlBackend backend)
	{
		for (const Entry& entry : entries)
		{
			if (entry.backend == backend) { return entry.name; }
		}
		return "unknown";
	}

	bool XmlDocumentFactory::available(XmlBackend backend)
	{
		for (const Entry& entry : entries)
		{
			if (entry.backend == backend) { return entry.built; }
		}
		return false;
	}

	std::vector<XmlBackend> XmlDocumentFactory::availableBackends()
	{
		std::vector<XmlBackend> built;
		for (const Entry& entry : entries)
		{
			if (entry.built) { built.push_back(entry.backend); }
		}
		return built;
	}

	XmlBackend XmlDocumentFactory::defaultBackend()
	{
		const std::vector<XmlBackend> built = availableBackends();
		return built.empty() ? XmlBackend::Xerces : built.front();
	}

	std::unique_ptr<XmlDocument> XmlDocumentFactory::create(XmlBackend backend)
	{
#ifdef XGE_WITH_XERCES
		if (backend == XmlBackend::Xerces) { return std::make_unique<XercesXmlDocument>(); }
#endif
#ifdef XGE_WITH_TINYXML2
		if (backend == XmlBackend::TinyXml2) { return std::make_unique<TinyXml2Document>(); }
#endif
#ifdef XGE_WITH_PUGIXML
		if (backend == XmlBackend::PugiXml) { return std::make_unique<PugiXmlDocument>(); }
#endif
#ifdef XGE_WITH_RAPIDXML
		if (backend == XmlBackend::RapidXml) { return std::make_unique<RapidXmlDocument>(); }
#endif

		std::string built;
		for (const XmlBackend each : availableBackends())
		{
			built += (built.empty() ? "" : ", ") + name(each);
		}
		throw std::runtime_error("the " + name(backend) + " XML library is not built into this program (built with: "
			+ (built.empty() ? "none" : built) + ")");
	}

	std::ostream& operator<<(std::ostream& o, SchemaValidation validation)
	{
		switch (validation)
		{
		case SchemaValidation::None:   o << "none (no schema referenced)"; break;
		case SchemaValidation::Weak:   o << "weak (checked by this project's own lightweight XSD subset validator)"; break;
		case SchemaValidation::Strong: o << "strong (checked by Xerces's real XSD engine)"; break;
		}

		return o;
	}
}
