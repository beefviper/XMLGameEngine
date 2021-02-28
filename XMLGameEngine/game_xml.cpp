// game_xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game.h"
#include "game_xml.h"

namespace xge
{
	void Game::initXML(void)
	{
		xc::XMLPlatformUtils::Initialize();

		xc::XercesDOMParser domParser;
		if (domParser.loadGrammar("assets/xmlgameengine.xsd", xc::Grammar::SchemaGrammarType) == NULL)
		{
			fprintf(stderr, "couldn't load schema\n");
			return;
		}

		ParserErrorHandler parserErrorHandler;

		domParser.setErrorHandler(&parserErrorHandler);
		domParser.setValidationScheme(xc::XercesDOMParser::Val_Auto);
		domParser.setDoNamespaces(true);
		domParser.setDoSchema(true);
		domParser.setValidationConstraintFatal(true);

		domParser.parse(filename.c_str());

		if (domParser.getErrorCount() == 0)
			printf("XML file validated against the schema successfully\n");
		else
			printf("XML file doesn't conform to the schema\n");

		auto doc = domParser.getDocument();
		auto root = doc->getDocumentElement();

		auto window = root->getFirstElementChild();

		std::unique_ptr<const XMLCh*> name = std::make_unique<const XMLCh*>(xc::XMLString::transcode("name"));

		auto window_name = window->getAttribute(*name);
		
		std::cout << "window: name = " << xc::XMLString::transcode(window_name) << std::endl;

		tx::XMLDocument xml;
		tx::XMLError xmlErrorCode = xml.LoadFile(filename.c_str());
		checkXMLResult(xml, xmlErrorCode);

		tx::XMLElement* xGame = getXMLRoot(xml.FirstChildElement("game"));

		tx::XMLElement* xWindow = getXMLElement(xGame, "window");

		tx::XMLElement* xVariables = getXMLElement(xGame, "variables");
		tx::XMLElement* xObjects = getXMLElement(xGame, "objects");
		tx::XMLElement* xStates = getXMLElement(xGame, "states");

		tx::XMLElement* xVariable = getXMLElement(xVariables, "variable");
		tx::XMLElement* xObject = getXMLElement(xObjects, "object");
		tx::XMLElement* xState = getXMLElement(xStates, "state");

		// load window description
		windowDesc.name = getXMLAttribute(xWindow, "name");
		windowDesc.width = getXMLAttributeFloat(xWindow, "width");
		windowDesc.height = getXMLAttributeFloat(xWindow, "height");
		windowDesc.background = getXMLAttribute(xWindow, "background");
		windowDesc.fullscreen = getXMLAttribute(xWindow, "fullscreen");
		windowDesc.framerate = getXMLAttributeInt(xWindow, "framerate");

		// load variables
		while (xVariable != nullptr)
		{
			std::string vName = getXMLAttribute(xVariable, "name");
			float vValue = getXMLAttributeFloat(xVariable, "value");
			variables[vName] = vValue;
			xVariable = xVariable->NextSiblingElement("variable");
		}

		// load objects
		while (xObject != nullptr)
		{
			std::string xName = getXMLAttribute(xObject, "name");

			tx::XMLElement* xSprite = getXMLElement(xObject, "sprite");
			std::string xSrc = getXMLAttribute(xSprite, "src");

			tx::XMLElement* xPosition = getXMLElement(xObject, "position");
			std::string xPosX = getXMLAttribute(xPosition, "x");
			std::string xPosY = getXMLAttribute(xPosition, "y");

			tx::XMLElement* xVelocity = getXMLElement(xObject, "velocity");
			std::string xVelX = getXMLAttribute(xVelocity, "x");
			std::string xVelY = getXMLAttribute(xVelocity, "y");


			Vector2str position{ xPosX, xPosY };
			Vector2str velocity{ xVelX, xVelY };

			tx::XMLElement* xCollision = getXMLElement(xObject, "collision");
			std::string oCollision = getXMLAttribute(xCollision, "enabled");

			CollisionData xCollisionData;
			xCollisionData.top = getXMLAttributeOptional(xCollision, "top");
			xCollisionData.bottom = getXMLAttributeOptional(xCollision, "bottom");
			xCollisionData.left = getXMLAttributeOptional(xCollision, "left");
			xCollisionData.right = getXMLAttributeOptional(xCollision, "right");
			xCollisionData.basic = getXMLAttributeOptional(xCollision, "basic");
			bool bCollision = (oCollision == "true") ? true : false;

			SObject sObject;
			sObject.name = xName;
			sObject.sposition = position;
			sObject.svelocity = velocity;
			sObjects.push_back(sObject);

			tx::XMLElement* xActions = getXMLElementOptional(xObject, "actions");
			tx::XMLElement* xAction = getXMLElementOptional(xActions, "action");

			std::map<std::string, std::string> oActions;

			while (xAction != nullptr)
			{
				std::string aName = getXMLAttribute(xAction, "name");
				std::string aValue = getXMLAttribute(xAction, "value");
				oActions[aName] = aValue;

				xAction = xAction->NextSiblingElement("action");
			}

			Object object;
			object.init(xName, xSrc, oActions);
			object.collisionData = xCollisionData;
			object.collision = bCollision;
			objects.push_back(object);
			xObject = xObject->NextSiblingElement("object");
		}

		// load states
		while (xState != nullptr)
		{
			std::string sName = getXMLAttribute(xState, "name");

			tx::XMLElement* stObject = getXMLElement(xState, "show");

			std::vector<std::string> sShows;

			while (stObject != nullptr)
			{
				std::string shName = getXMLAttribute(stObject, "object");
				sShows.push_back(shName);
				stObject = stObject->NextSiblingElement("show");
			}

			tx::XMLElement* xInputs = getXMLElement(xState, "inputs");
			tx::XMLElement* xInput = getXMLElement(xInputs, "input");

			std::map<std::string, std::string> sInputs;

			while (xInput != nullptr)
			{
				std::string iAction = getXMLAttribute(xInput, "action");
				std::string iButton = getXMLAttribute(xInput, "button");

				sInputs[iAction] = iButton;

				xInput = xInput->NextSiblingElement("input");
			}

			State state;
			state.init(sName, sShows, sInputs);
			states.push_back(state);

			xState = xState->NextSiblingElement("state");
		}
	}

	void ParserErrorHandler::reportParseException(const xc::SAXParseException& ex)
	{
		char* msg = xc::XMLString::transcode(ex.getMessage());
		fprintf(stderr, "at line %llu column %llu, %s\n",
			ex.getLineNumber(), ex.getColumnNumber(), msg);
		xc::XMLString::release(&msg);
	}

	void ParserErrorHandler::warning(const xc::SAXParseException& ex)
	{
		reportParseException(ex);
	}

	void ParserErrorHandler::error(const xc::SAXParseException& ex)
	{
		reportParseException(ex);
	}

	void ParserErrorHandler::fatalError(const xc::SAXParseException& ex)
	{
		reportParseException(ex);
	}

	void ParserErrorHandler::resetErrors()
	{

	}

	void checkXMLResult(tx::XMLDocument& xml, tx::XMLError result)
	{
		if (result != tx::XML_SUCCESS)
		{
			std::cout << xml.ErrorStr() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	tx::XMLElement* getXMLRoot(tx::XMLElement* element)
	{
		if (element == nullptr)
		{
			std::cout << "Error: could not parse <game> tag" << std::endl;
			exit(EXIT_FAILURE);
		}

		return element;
	}

	tx::XMLElement* getXMLElement(tx::XMLElement* element, std::string tag)
	{
		tx::XMLElement* newElement = element->FirstChildElement(tag.c_str());
		if (newElement == nullptr)
		{
			std::cout << "Error: could not parse <" << tag << "> tag"
				<< " on line " << element->GetLineNum() << std::endl;
			exit(EXIT_FAILURE);
		}

		return newElement;
	}

	tx::XMLElement* getXMLElementOptional(tx::XMLElement* element, std::string tag)
	{
		tx::XMLElement* newElement = nullptr;
		if (element != nullptr)
		{
			newElement = element->FirstChildElement(tag.c_str());
		}

		return newElement;
	}

	std::string getXMLText(tx::XMLElement* element)
	{
		std::string text;
		if (!element->GetText())
		{
			std::cout << "Error: could not find <" << element->Name() << "> text"
				<< " on line " << element->GetLineNum() << std::endl;
			exit(EXIT_FAILURE);
		}

		text = element->GetText();

		return text;
	}

	std::string getXMLAttribute(tx::XMLElement* element, std::string attribute)
	{
		const char* attributeText = nullptr;

		attributeText = element->Attribute(attribute.c_str());
		if (attributeText == nullptr)
		{
			std::cout << "Error: can not find <" << element->Name() << "> attribute \"" << attribute << "\""
				<< " on line " << element->GetLineNum() << std::endl;
			exit(EXIT_FAILURE);
		}

		std::string attributeString = attributeText;

		return attributeString;
	}

	std::string getXMLAttributeOptional(tx::XMLElement* element, std::string attribute)
	{
		const char* attributeText = nullptr;
		std::string attributeString;

		attributeText = element->Attribute(attribute.c_str());
		if (attributeText != nullptr)
		{
			attributeString = attributeText;
		}

		return attributeString;
	}

	int getXMLAttributeInt(tx::XMLElement* element, std::string attribute)
	{
		int attributeInt{};

		tx::XMLError xError = element->QueryIntAttribute(attribute.c_str(), &attributeInt);
		if (xError != tx::XML_SUCCESS)
		{
			std::cout << "Error: parsing <" << element->Name() << "> int attribute \"" << attribute << "\""
				<< " on line " << element->GetLineNum() << std::endl;
			exit(EXIT_FAILURE);
		}

		return	attributeInt;
	}

	float getXMLAttributeFloat(tx::XMLElement* element, std::string attribute)
	{
		float attributeFloat{};

		tx::XMLError xError = element->QueryFloatAttribute(attribute.c_str(), &attributeFloat);
		if (xError != tx::XML_SUCCESS)
		{
			std::cout << "Error: parsing <" << element->Name() << "> float attribute \"" << attribute << "\""
				<< " on line " << element->GetLineNum() << std::endl;
			exit(EXIT_FAILURE);
		}

		return	attributeFloat;
	}
}
