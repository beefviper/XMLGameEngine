// xsd_lite.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026

#include "xsd_lite.h"

#include "xml_document.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

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

		// Strips any "prefix:" off a qualified name (e.g. "xs:element" ->
		// "element") - every backend's getName()/getAttribute() hands back
		// the raw qualified name as written (see xml_document.h), and this
		// schema only ever uses one prefix ("xs"), but comparing against
		// the local name rather than assuming that exact prefix string
		// costs nothing and is one less thing to break if it ever changes.
		std::string localName(const std::string& qualifiedName)
		{
			const std::size_t colon = qualifiedName.find(':');
			return colon == std::string::npos ? qualifiedName : qualifiedName.substr(colon + 1);
		}

		std::string trim(const std::string& text)
		{
			const auto first = text.find_first_not_of(" \t\r\n");
			if (first == std::string::npos) { return {}; }
			const auto last = text.find_last_not_of(" \t\r\n");
			return text.substr(first, last - first + 1);
		}

		// minOccurs/maxOccurs as written: absent is 1, "unbounded" is -1.
		int occurs(const XmlNode& node, const char* attribute)
		{
			const std::string value = node.getAttribute(attribute);
			if (value.empty()) { return 1; }
			return value == "unbounded" ? -1 : std::stoi(value);
		}
	}

	// ------------------------------------------------------------ the model
	struct XsdLiteValidator::Model
	{
		struct Attribute
		{
			std::string name;
			std::string type;
			bool required = false;
		};

		struct SimpleType
		{
			std::vector<std::string> enumeration; // empty: any value the built-in base allows
			std::string base;
		};

		struct ComplexType;

		// One item of a content model: an element, or a sequence/choice of
		// other particles, and how many times it may repeat.
		struct Particle
		{
			enum class Kind { Element, Sequence, Choice };

			Kind kind = Kind::Element;
			int minOccurs = 1;
			int maxOccurs = 1;

			// Element:
			std::string name;
			std::shared_ptr<ComplexType> complexType; // null: a simple element (text only) ...
			std::string simpleType;                   // ... of this type ("" : any text)

			// Sequence / Choice:
			std::vector<Particle> children;
		};

		struct ComplexType
		{
			bool mixed = false;
			std::vector<Attribute> attributes;
			std::unique_ptr<Particle> content; // null: no child elements allowed
		};

		Particle root;
		std::map<std::string, SimpleType> simpleTypes;

		// The top-level named pieces of the schema, kept as XML until something
		// refers to them (so the order they are written in does not matter),
		// and what has been built from them.
		std::map<std::string, std::unique_ptr<XmlNode>> complexTypeNodes;
		std::map<std::string, std::unique_ptr<XmlNode>> groupNodes;
		std::map<std::string, std::shared_ptr<ComplexType>> complexTypes;
		std::map<std::string, Particle> groups;
		std::set<std::string> inProgress;

		// -------------------------------------------------------- building
		bool build(const XmlNode& schemaRoot, std::string& error);
		bool buildSimpleType(const XmlNode& node, std::string& error);
		bool buildElement(const XmlNode& node, Particle& out, std::string& error);
		bool buildComplexType(const XmlNode& node, ComplexType& out, std::string& error);
		bool buildContent(const XmlNode& node, Particle& out, std::string& error);
		bool resolveComplexType(const std::string& name, std::shared_ptr<ComplexType>& out, std::string& error);
		bool resolveGroup(const std::string& name, Particle& out, std::string& error);

		// ------------------------------------------------------ validating
		using Children = std::vector<std::unique_ptr<XmlNode>>;

		// value against a declared type (e.g. "xs:boolean", or the name of one
		// of the schema's own xs:simpleTypes) - see the class comment in
		// xsd_lite.h for how unrecognized types are handled.
		bool typeMatches(const std::string& type, const std::string& value) const;

		bool validateElement(const Particle& declaration, const XmlNode& actual, const std::string& path, std::string& error) const;
		bool validateAttributes(const ComplexType& type, const XmlNode& actual, const std::string& path, std::string& error) const;

		// What matching a content model against an element's children found:
		// the children matched so far up to `position`, and, when it did not
		// match, either a real error inside a child that was there
		// (`hardError`), or just what it was still waiting for (`expected`).
		// Optional parts are tried and put back, so `position` says little about
		// where a mismatch really was; `failPosition` and `failExpected` keep the
		// furthest child at which something the content model wanted was not
		// there, which is what a message should point at.
		struct MatchState
		{
			std::size_t position = 0;
			bool hardError = false;
			std::string error;
			std::string expected;
			std::size_t failPosition = 0;
			std::string failExpected;

			void noteFailure(std::size_t at, const std::string& wanted)
			{
				if (at >= failPosition) { failPosition = at; failExpected = wanted; }
			}
		};

		bool matchParticle(const Particle& particle, const Children& children, MatchState& state, const std::string& path) const;

		// The names of the elements a particle could start with, for "expected
		// one of ..." in a message.
		static void startingNames(const Particle& particle, std::vector<std::string>& names);
		bool matchOnce(const Particle& particle, const Children& children, MatchState& state, const std::string& path, int occurrence) const;
	};

	bool XsdLiteValidator::Model::buildSimpleType(const XmlNode& node, std::string& error)
	{
		const std::string name = node.getAttribute("name");
		if (name.empty())
		{
			error = "xs:simpleType with no 'name' attribute";
			return false;
		}

		SimpleType simpleType;

		for (std::unique_ptr<XmlNode> restriction = node.getFirstChild(); restriction; restriction = restriction->getNextSibling())
		{
			if (localName(restriction->getName()) != "restriction") { continue; }

			simpleType.base = localName(restriction->getAttribute("base"));

			for (std::unique_ptr<XmlNode> facet = restriction->getFirstChild(); facet; facet = facet->getNextSibling())
			{
				if (localName(facet->getName()) == "enumeration")
				{
					simpleType.enumeration.push_back(facet->getAttribute("value"));
				}
			}
		}

		simpleTypes[name] = std::move(simpleType);
		return true;
	}

	// An xs:sequence or xs:choice, or an xs:group ref, as a Particle.
	bool XsdLiteValidator::Model::buildContent(const XmlNode& node, Particle& out, std::string& error)
	{
		const std::string kind = localName(node.getName());

		if (kind == "group")
		{
			const std::string ref = localName(node.getAttribute("ref"));
			Particle group;
			if (!resolveGroup(ref, group, error)) { return false; }

			// A reference is a sequence of the one thing the group is, repeated
			// as the reference says.
			out.kind = Particle::Kind::Sequence;
			out.minOccurs = occurs(node, "minOccurs");
			out.maxOccurs = occurs(node, "maxOccurs");
			out.children.push_back(std::move(group));
			return true;
		}

		if (kind != "sequence" && kind != "choice")
		{
			error = "unsupported schema construct xs:" + kind;
			return false;
		}

		out.kind = kind == "sequence" ? Particle::Kind::Sequence : Particle::Kind::Choice;
		out.minOccurs = occurs(node, "minOccurs");
		out.maxOccurs = occurs(node, "maxOccurs");

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string childKind = localName(child->getName());
			Particle particle;

			if (childKind == "element")
			{
				if (!buildElement(*child, particle, error)) { return false; }
			}
			else if (childKind == "sequence" || childKind == "choice" || childKind == "group")
			{
				if (!buildContent(*child, particle, error)) { return false; }
			}
			else
			{
				continue; // xs:annotation and the like
			}

			out.children.push_back(std::move(particle));
		}

		return true;
	}

	bool XsdLiteValidator::Model::buildComplexType(const XmlNode& node, ComplexType& out, std::string& error)
	{
		out.mixed = node.getAttribute("mixed") == "true";

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string kind = localName(child->getName());

			if (kind == "sequence" || kind == "choice" || kind == "group")
			{
				auto content = std::make_unique<Particle>();
				if (!buildContent(*child, *content, error)) { return false; }
				out.content = std::move(content);
			}
			else if (kind == "attribute")
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

				out.attributes.push_back(std::move(attribute));
			}
		}

		return true;
	}

	bool XsdLiteValidator::Model::buildElement(const XmlNode& node, Particle& out, std::string& error)
	{
		out.kind = Particle::Kind::Element;
		out.name = node.getAttribute("name");

		if (out.name.empty())
		{
			error = "xs:element with no 'name' attribute";
			return false;
		}

		out.minOccurs = occurs(node, "minOccurs");
		out.maxOccurs = occurs(node, "maxOccurs");

		const std::string type = localName(node.getAttribute("type"));

		if (!type.empty())
		{
			if (complexTypeNodes.count(type))
			{
				return resolveComplexType(type, out.complexType, error);
			}

			out.simpleType = type; // xs:string, xs:boolean, ... or one of simpleTypes
			return true;
		}

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child; child = child->getNextSibling())
		{
			if (localName(child->getName()) == "complexType")
			{
				out.complexType = std::make_shared<ComplexType>();
				return buildComplexType(*child, *out.complexType, error);
			}
		}

		// No type at all: whatever text is there is fine.
		return true;
	}

	bool XsdLiteValidator::Model::resolveComplexType(const std::string& name, std::shared_ptr<ComplexType>& out, std::string& error)
	{
		if (auto built = complexTypes.find(name); built != complexTypes.end())
		{
			out = built->second;
			return true;
		}

		const auto node = complexTypeNodes.find(name);
		if (node == complexTypeNodes.end())
		{
			error = "the schema refers to a type '" + name + "' it does not define";
			return false;
		}

		if (!inProgress.insert("type:" + name).second)
		{
			error = "the schema type '" + name + "' contains itself, which this validator does not support";
			return false;
		}

		auto type = std::make_shared<ComplexType>();
		const bool ok = buildComplexType(*node->second, *type, error);
		inProgress.erase("type:" + name);
		if (!ok) { return false; }

		complexTypes[name] = type;
		out = type;
		return true;
	}

	bool XsdLiteValidator::Model::resolveGroup(const std::string& name, Particle& out, std::string& error)
	{
		if (auto built = groups.find(name); built != groups.end())
		{
			out = built->second;
			return true;
		}

		const auto node = groupNodes.find(name);
		if (node == groupNodes.end())
		{
			error = "the schema refers to a group '" + name + "' it does not define";
			return false;
		}

		if (!inProgress.insert("group:" + name).second)
		{
			error = "the schema group '" + name + "' contains itself, which this validator does not support";
			return false;
		}

		bool ok = false;
		for (std::unique_ptr<XmlNode> child = node->second->getFirstChild(); child; child = child->getNextSibling())
		{
			const std::string kind = localName(child->getName());
			if (kind == "sequence" || kind == "choice")
			{
				Particle group;
				ok = buildContent(*child, group, error);
				if (ok) { groups[name] = group; out = std::move(group); }
				break;
			}
		}

		inProgress.erase("group:" + name);
		if (!ok && error.empty()) { error = "the schema group '" + name + "' has no xs:sequence or xs:choice"; }
		return ok;
	}

	bool XsdLiteValidator::Model::build(const XmlNode& schemaRoot, std::string& error)
	{
		std::unique_ptr<XmlNode> topElement;

		// Collect the named pieces first; they can then refer to each other in
		// any order.
		for (std::unique_ptr<XmlNode> top = schemaRoot.getFirstChild(); top; )
		{
			const std::string kind = localName(top->getName());
			std::unique_ptr<XmlNode> next = top->getNextSibling();

			if (kind == "simpleType")
			{
				if (!buildSimpleType(*top, error)) { return false; }
			}
			else if (kind == "complexType")
			{
				complexTypeNodes[top->getAttribute("name")] = std::move(top);
			}
			else if (kind == "group")
			{
				groupNodes[top->getAttribute("name")] = std::move(top);
			}
			else if (kind == "element" && !topElement)
			{
				topElement = std::move(top);
			}

			top = std::move(next);
		}

		if (!topElement)
		{
			error = "the schema has no top-level xs:element";
			return false;
		}

		return buildElement(*topElement, root, error);
	}

	// ----------------------------------------------------------- validating
	void XsdLiteValidator::Model::startingNames(const Particle& particle, std::vector<std::string>& names)
	{
		if (particle.kind == Particle::Kind::Element)
		{
			names.push_back("<" + particle.name + ">");
			return;
		}

		for (const Particle& part : particle.children)
		{
			startingNames(part, names);

			// In a sequence, only the first part that has to be there can start it.
			if (particle.kind == Particle::Kind::Sequence && part.minOccurs > 0) { break; }
		}
	}

	bool XsdLiteValidator::Model::typeMatches(const std::string& type, const std::string& value) const
	{
		const std::string type_ = localName(type);

		if (const auto simple = simpleTypes.find(type_); simple != simpleTypes.end())
		{
			const auto& enumeration = simple->second.enumeration;
			if (!enumeration.empty() && std::find(enumeration.begin(), enumeration.end(), value) == enumeration.end())
			{
				return false;
			}

			return simple->second.base.empty() || typeMatches(simple->second.base, value);
		}

		// xs:boolean's lexical space is true/false/1/0 - every value in
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

	bool XsdLiteValidator::Model::validateAttributes(const ComplexType& type, const XmlNode& actual, const std::string& path, std::string& error) const
	{
		for (const Attribute& attribute : type.attributes)
		{
			const std::string value = actual.getAttribute(attribute.name);

			// "" also means "missing" here (see XmlNode::getAttribute,
			// xml_document.h) - every attribute this schema declares is a
			// name, a keyword, a number, or true/false, none of which has a
			// legitimate empty-string value, so this is an adequate required
			// check.
			if (value.empty())
			{
				if (attribute.required)
				{
					error = path + ": missing required attribute '" + attribute.name + "'";
					return false;
				}

				continue;
			}

			if (!typeMatches(attribute.type, value))
			{
				error = path + ": attribute '" + attribute.name + "' = \"" + value + "\" is not a valid " + attribute.type;
				return false;
			}
		}

		return true;
	}

	bool XsdLiteValidator::Model::validateElement(const Particle& declaration, const XmlNode& actual, const std::string& path, std::string& error) const
	{
		Children children;
		for (std::unique_ptr<XmlNode> child = actual.getFirstChild(); child; )
		{
			std::unique_ptr<XmlNode> next = child->getNextSibling();
			children.push_back(std::move(child));
			child = std::move(next);
		}

		const std::string text = trim(actual.getText());

		if (!declaration.complexType)
		{
			// A simple element: text only, of the declared type.
			if (!children.empty())
			{
				error = path + ": unexpected element <" + children.front()->getName() + "> inside a text-only element";
				return false;
			}

			if (!declaration.simpleType.empty() && !typeMatches(declaration.simpleType, text))
			{
				error = path + ": \"" + text + "\" is not a valid " + declaration.simpleType;
				return false;
			}

			return true;
		}

		const ComplexType& type = *declaration.complexType;

		if (!validateAttributes(type, actual, path, error)) { return false; }

		if (!type.mixed && !text.empty())
		{
			error = path + ": unexpected text \"" + text + "\"";
			return false;
		}

		if (!type.content)
		{
			if (!children.empty())
			{
				error = path + ": unexpected element <" + children.front()->getName() + ">";
				return false;
			}

			return true;
		}

		MatchState state;
		const bool matched = matchParticle(*type.content, children, state, path);

		if (state.hardError)
		{
			error = state.error;
			return false;
		}

		if (matched && state.position == children.size()) { return true; }

		// Where it went wrong: for a match that fell short, the furthest child
		// something was still wanted at (see MatchState); for one that matched
		// but left children over, the first one left over.
		const std::size_t at = matched ? state.position : state.failPosition;
		const std::string& expected = matched ? state.expected : state.failExpected;

		if (at < children.size())
		{
			error = path + ": unexpected element <" + children[at]->getName() + ">";
			if (!expected.empty()) { error += ", expected " + expected; }
		}
		else
		{
			error = path + ": " + (expected.empty() ? std::string("incomplete") : "expected " + expected + " but the element ends");
		}

		return false;
	}

	// One pass of `particle`: an element (must be the next child), a sequence
	// (each part in turn), or a choice (the first alternative that matches).
	bool XsdLiteValidator::Model::matchOnce(const Particle& particle, const Children& children, MatchState& state, const std::string& path, int occurrence) const
	{
		if (particle.kind == Particle::Kind::Element)
		{
			if (state.position >= children.size() || localName(children[state.position]->getName()) != particle.name)
			{
				state.expected = "<" + particle.name + ">";
				state.noteFailure(state.position, state.expected);
				return false;
			}

			std::string elementPath = path + "/" + particle.name;
			if (particle.maxOccurs != 1) { elementPath += "[" + std::to_string(occurrence + 1) + "]"; }

			std::string error;
			if (!validateElement(particle, *children[state.position], elementPath, error))
			{
				state.hardError = true;
				state.error = error;
				return false;
			}

			++state.position;
			return true;
		}

		if (particle.kind == Particle::Kind::Sequence)
		{
			for (const Particle& part : particle.children)
			{
				if (!matchParticle(part, children, state, path)) { return false; }
			}

			return true;
		}

		// Choice: the first alternative that matches, preferring one that
		// takes something over one that matches by taking nothing.
		const std::size_t start = state.position;
		bool matchedEmpty = false;

		for (const Particle& alternative : particle.children)
		{
			state.position = start;
			if (matchParticle(alternative, children, state, path))
			{
				if (state.position > start) { return true; }
				matchedEmpty = true;
			}
			else if (state.hardError)
			{
				return false;
			}
		}

		state.position = start;

		if (!matchedEmpty)
		{
			std::vector<std::string> names;
			startingNames(particle, names);

			state.expected = "one of";
			for (std::size_t i = 0; i < names.size() && i < 6; ++i) { state.expected += (i ? ", " : " ") + names[i]; }
			if (names.size() > 6) { state.expected += ", ..."; }
			state.noteFailure(start, state.expected);
		}

		return matchedEmpty;
	}

	// `particle`, repeated as often as its minOccurs/maxOccurs allow.
	bool XsdLiteValidator::Model::matchParticle(const Particle& particle, const Children& children, MatchState& state, const std::string& path) const
	{
		int count = 0;

		while (particle.maxOccurs == -1 || count < particle.maxOccurs)
		{
			const std::size_t before = state.position;

			if (!matchOnce(particle, children, state, path, count))
			{
				if (state.hardError) { return false; }

				// Whatever was tried and did not fit is put back; if it was
				// only an optional repeat, that is fine.
				state.position = before;
				break;
			}

			++count;
			if (state.position == before) { break; } // matched, but took nothing
		}

		if (count < particle.minOccurs)
		{
			return false;
		}

		state.expected.clear();
		return true;
	}

	// ------------------------------------------------------------ the class
	XsdLiteValidator::XsdLiteValidator() = default;
	XsdLiteValidator::~XsdLiteValidator() = default;

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

		model = std::make_unique<Model>();

		if (!model->build(*schemaRoot, errorMessage))
		{
			errorMessage = "'" + schemaFilename + "': " + errorMessage;
			model.reset();
			return false;
		}

		return true;
	}

	bool XsdLiteValidator::validate(const XmlNode& root) const
	{
		errorMessage.clear();

		if (!model)
		{
			errorMessage = "no schema loaded";
			return false;
		}

		if (localName(root.getName()) != model->root.name)
		{
			errorMessage = "expected root element <" + model->root.name + ">, found <" + root.getName() + ">";
			return false;
		}

		return model->validateElement(model->root, root, model->root.name, errorMessage);
	}

	std::string XsdLiteValidator::getErrorMessage() const
	{
		return errorMessage;
	}
}
