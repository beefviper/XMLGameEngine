// xsd_lite.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026

#include "xsd_lite.h"

#include "xml_document.h"

#include <cctype>
#include <charconv>
#include <cstdint>

namespace xge
{
	namespace
	{
		// Digits only (with an optional leading sign), in range - the same
		// lexical check for every integer-derived XSD built-in this
		// validator knows about (xs:integer/xs:unsignedByte/xs:unsignedShort
		// below), just with a different [minValue, maxValue].
		bool isIntegerInRange(const std::string& value, long long minValue, long long maxValue)
		{
			if (value.empty()) { return false; }

			std::size_t index = (value[0] == '+' || value[0] == '-') ? 1 : 0;
			if (index >= value.size()) { return false; }

			for (std::size_t i = index; i < value.size(); ++i)
			{
				if (!std::isdigit(static_cast<unsigned char>(value[i]))) { return false; }
			}

			long long parsed = 0;
			const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
			return result.ec == std::errc{} && parsed >= minValue && parsed <= maxValue;
		}
	}

	std::string XsdLiteValidator::localName(const std::string& qualifiedName)
	{
		const std::size_t colon = qualifiedName.find(':');
		return colon == std::string::npos ? qualifiedName : qualifiedName.substr(colon + 1);
	}

	bool XsdLiteValidator::typeMatches(const std::string& type, const std::string& value)
	{
		const std::string type_ = localName(type);

		// xs:boolean's lexical space is true/false/1/0 - every attribute in
		// assets/xmlgameengine.xsd is written as true/false, but this
		// checks the type's actual rules rather than just this schema's
		// current usage of it.
		if (type_ == "boolean") { return value == "true" || value == "false" || value == "1" || value == "0"; }
		if (type_ == "integer") { return isIntegerInRange(value, INT64_MIN, INT64_MAX); }
		if (type_ == "unsignedByte") { return isIntegerInRange(value, 0, 255); }
		if (type_ == "unsignedShort") { return isIntegerInRange(value, 0, 65535); }

		// xs:string, and anything else this validator doesn't specifically
		// know about - see the class comment in xsd_lite.h for why that's
		// permissive rather than a failure.
		return true;
	}

	bool XsdLiteValidator::parseComplexType(const XmlNode& complexType, Element& outElement, std::string& error)
	{
		std::unique_ptr<XmlNode> child = complexType.getFirstChild();

		while (child)
		{
			const std::string name = localName(child->getName());

			if (name == "sequence")
			{
				// xs:sequence's own minOccurs (assets/xmlgameengine.xsd
				// uses this exactly once, on <collisions>'s inner sequence,
				// to say the whole group of <collision> children is
				// optional) makes every element directly inside it at
				// least that optional too - "0" here just floors each
				// child's own minOccurs to 0, which is exactly equivalent
				// to the real rule for a sequence (like this schema's)
				// with only one element inside it; that's the only shape
				// "the basic subset of functions we're using" (see
				// xsd_lite.h) needs to get right.
				const std::string sequenceMinOccurs = child->getAttribute("minOccurs");
				const bool sequenceOptional = (sequenceMinOccurs == "0");

				std::unique_ptr<XmlNode> sequenceChild = child->getFirstChild();

				while (sequenceChild)
				{
					if (localName(sequenceChild->getName()) == "element")
					{
						Element nested;
						if (!parseElement(*sequenceChild, nested, error)) { return false; }
						if (sequenceOptional) { nested.minOccurs = 0; }
						outElement.children.push_back(std::move(nested));
					}

					sequenceChild = sequenceChild->getNextSibling();
				}
			}
			else if (name == "attribute")
			{
				Attribute attribute;
				attribute.name = child->getAttribute("name");
				attribute.type = child->getAttribute("type");
				attribute.required = (child->getAttribute("use") == "required");

				if (attribute.name.empty())
				{
					error = "xs:attribute with no 'name' attribute";
					return false;
				}

				outElement.attributes.push_back(std::move(attribute));
			}

			child = child->getNextSibling();
		}

		return true;
	}

	bool XsdLiteValidator::parseElement(const XmlNode& xsdElement, Element& outElement, std::string& error)
	{
		outElement.name = xsdElement.getAttribute("name");

		if (outElement.name.empty())
		{
			error = "xs:element with no 'name' attribute";
			return false;
		}

		const std::string minOccurs = xsdElement.getAttribute("minOccurs");
		const std::string maxOccurs = xsdElement.getAttribute("maxOccurs");

		outElement.minOccurs = minOccurs.empty() ? 1 : std::stoi(minOccurs);
		outElement.maxOccurs = maxOccurs.empty() ? 1 : (maxOccurs == "unbounded" ? -1 : std::stoi(maxOccurs));

		std::unique_ptr<XmlNode> complexType = xsdElement.getFirstChild();

		while (complexType && localName(complexType->getName()) != "complexType")
		{
			complexType = complexType->getNextSibling();
		}

		// An xs:element with no xs:complexType is a simple-typed leaf (no
		// attributes, no children expected) - none of this schema's own
		// elements are that simple, but leaving it valid here rather than
		// failing keeps this consistent with the permissive spirit
		// described in xsd_lite.h.
		return !complexType || parseComplexType(*complexType, outElement, error);
	}

	bool XsdLiteValidator::loadSchema(const std::string& schemaFilename, XmlBackend backend)
	{
		std::unique_ptr<XmlDocument> schemaDocument = XmlDocumentFactory::create(backend);

		if (!schemaDocument->load(schemaFilename))
		{
			errorMessage = "failed to parse schema file '" + schemaFilename + "': " + schemaDocument->getErrorMessage();
			return false;
		}

		std::unique_ptr<XmlNode> schemaRoot = schemaDocument->getRootElement();

		if (!schemaRoot || localName(schemaRoot->getName()) != "schema")
		{
			errorMessage = "'" + schemaFilename + "' is not an xs:schema document";
			return false;
		}

		std::unique_ptr<XmlNode> topElement = schemaRoot->getFirstChild();

		while (topElement && localName(topElement->getName()) != "element")
		{
			topElement = topElement->getNextSibling();
		}

		if (!topElement)
		{
			errorMessage = "'" + schemaFilename + "' has no top-level xs:element";
			return false;
		}

		return parseElement(*topElement, rootElement, errorMessage);
	}

	bool XsdLiteValidator::validateAttributes(const Element& element, const XmlNode& actual, const std::string& path) const
	{
		for (const Attribute& attribute : element.attributes)
		{
			const std::string value = actual.getAttribute(attribute.name);

			// "" also means "missing" here (see XmlNode::getAttribute,
			// xml_document.h) - every attribute this schema declares is a
			// name, a number, or true/false, none of which has a legitimate
			// empty-string value, so this is an adequate required check.
			if (value.empty())
			{
				if (attribute.required)
				{
					errorMessage = path + ": missing required attribute '" + attribute.name + "'";
					return false;
				}

				continue;
			}

			if (!typeMatches(attribute.type, value))
			{
				errorMessage = path + ": attribute '" + attribute.name + "' = \"" + value + "\" is not a valid " + attribute.type;
				return false;
			}
		}

		return true;
	}

	bool XsdLiteValidator::validateSequence(const std::vector<Element>& expected, std::unique_ptr<XmlNode> cursor, const std::string& path) const
	{
		for (const Element& element : expected)
		{
			const std::string elementPath = path + "/" + element.name;
			int count = 0;

			while (cursor && localName(cursor->getName()) == element.name)
			{
				if (!validateAttributes(element, *cursor, elementPath)) { return false; }

				if (!element.children.empty() && !validateSequence(element.children, cursor->getFirstChild(), elementPath))
				{
					return false;
				}

				++count;
				cursor = cursor->getNextSibling();

				if (element.maxOccurs != -1 && count >= element.maxOccurs) { break; }
			}

			if (count < element.minOccurs)
			{
				errorMessage = path + ": expected at least " + std::to_string(element.minOccurs) +
					" <" + element.name + "> element(s), found " + std::to_string(count);
				return false;
			}
		}

		if (cursor)
		{
			errorMessage = path + ": unexpected element <" + cursor->getName() + ">";
			return false;
		}

		return true;
	}

	bool XsdLiteValidator::validate(const XmlNode& root) const
	{
		errorMessage.clear();

		if (localName(root.getName()) != rootElement.name)
		{
			errorMessage = "expected root element <" + rootElement.name + ">, found <" + root.getName() + ">";
			return false;
		}

		return validateAttributes(rootElement, root, rootElement.name)
			&& validateSequence(rootElement.children, root.getFirstChild(), rootElement.name);
	}

	std::string XsdLiteValidator::getErrorMessage() const
	{
		return errorMessage;
	}
}
