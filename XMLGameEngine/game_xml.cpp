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
		{
			xc::XercesDOMParser domParser;
			if (domParser.loadGrammar("assets/xmlgameengine.xsd", xc::Grammar::SchemaGrammarType) == NULL)
			{
				std::cout << "couldn't load schema\n\n";
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
				std::cout << "XML file validated against the schema successfully\n\n";
			else
				std::cout << "XML file doesn't conform to the schema\n\n";

			auto s2x = [](std::string input)
			{
				std::unique_ptr<const XMLCh*> output = std::make_unique<const XMLCh*>(xc::XMLString::transcode(input.c_str()));
				return *std::move(output);
			};
			auto x2s = [](const XMLCh* input)
			{
				return xc::XMLString::transcode(input);
			};
			auto getAttributeByName = [s2x, x2s](xc::DOMElement* element, std::string name)
			{
				return x2s(element->getAttribute(s2x(name)));
			};

			auto xc_doc = domParser.getDocument();
			auto xc_root = xc_doc->getDocumentElement();
			auto xc_window = xc_root->getFirstElementChild();
			auto xc_variables = xc_window->getNextElementSibling();
			auto xc_variable = xc_variables->getFirstElementChild();
			auto xc_objects = xc_variables->getNextElementSibling();
			auto xc_object = xc_objects->getFirstElementChild();
			auto xc_states = xc_objects->getNextElementSibling();
			auto xc_state = xc_states->getFirstElementChild();

			// load window description
			windowDesc.name = getAttributeByName(xc_window, "name");
			windowDesc.width = std::stof(getAttributeByName(xc_window, "width"));
			windowDesc.height = std::stof(getAttributeByName(xc_window, "height"));
			windowDesc.background = getAttributeByName(xc_window, "background");
			windowDesc.fullscreen = getAttributeByName(xc_window, "fullscreen");
			windowDesc.framerate = std::stoi(getAttributeByName(xc_window, "framerate"));

			// load variables
			while (xc_variable != nullptr)
			{
				std::string xc_var_name = getAttributeByName(xc_variable, "name");
				float xc_var_value = std::stof(getAttributeByName(xc_variable, "value"));
				variables[xc_var_name] = xc_var_value;
				xc_variable = xc_variable->getNextElementSibling();
			}

			// load objects
			while (xc_object != nullptr)
			{
				std::string xc_obj_name = getAttributeByName(xc_object, "name");

				auto xc_sprite = xc_object->getFirstElementChild();
				std::string xc_sprite_src = getAttributeByName(xc_sprite, "src");

				auto xc_pos = xc_sprite->getNextElementSibling();
				std::string xc_pos_x = getAttributeByName(xc_pos, "x");
				std::string xc_pos_y = getAttributeByName(xc_pos, "y");

				auto xc_vel = xc_pos->getNextElementSibling();
				std::string xc_vel_x = getAttributeByName(xc_vel, "x");
				std::string xc_vel_y = getAttributeByName(xc_vel, "y");

				Vector2str position{ xc_pos_x, xc_pos_y };
				Vector2str velocity{ xc_vel_x, xc_vel_y };

				auto xc_collision = xc_vel->getNextElementSibling();
				std::string xc_collision_enabled = getAttributeByName(xc_collision, "enabled");

				CollisionData xc_collision_data;
				xc_collision_data.top = getAttributeByName(xc_collision, "top");
				xc_collision_data.bottom = getAttributeByName(xc_collision, "bottom");
				xc_collision_data.left = getAttributeByName(xc_collision, "left");
				xc_collision_data.right = getAttributeByName(xc_collision, "right");
				xc_collision_data.basic = getAttributeByName(xc_collision, "basic");
				bool xc_collision_state = (xc_collision_enabled == "true") ? true : false;

				SObject sObject;
				sObject.name = xc_obj_name;
				sObject.sposition = position;
				sObject.svelocity = velocity;
				sObjects.push_back(sObject);

				std::map<std::string, std::string> xc_action_map;

				auto xc_actions = xc_collision->getNextElementSibling();
				if (xc_actions)
				{
					auto xc_action = xc_actions->getFirstElementChild();

					while (xc_action != nullptr)
					{
						std::string xc_act_name = getAttributeByName(xc_action, "name");
						std::string xc_act_value = getAttributeByName(xc_action, "value");
						xc_action_map[xc_act_name] = xc_act_value;

						xc_action = xc_action->getNextElementSibling();
					}
				}

				Object object;
				object.init(xc_obj_name, xc_sprite_src, xc_action_map);
				object.collisionData = xc_collision_data;
				object.collision = xc_collision_state;
				objects.push_back(object);
				xc_object = xc_object->getNextElementSibling();
			}

			// load states
			while (xc_state != nullptr)
			{
				std::string xc_state_name = getAttributeByName(xc_state, "name");

				auto xc_shows = xc_state->getFirstElementChild();
				auto xc_show = xc_shows->getFirstElementChild();

				std::vector<std::string> xc_show_vec;

				while (xc_show != nullptr)
				{
					std::string xc_show_object = getAttributeByName(xc_show, "object");
					xc_show_vec.push_back(xc_show_object);
					xc_show = xc_show->getNextElementSibling();
				}

				auto xc_inputs = xc_shows->getNextElementSibling();
				auto xc_input = xc_inputs->getFirstElementChild();

				std::map<std::string, std::string> xc_inputs_map;

				while (xc_input != nullptr)
				{
					std::string xc_input_action = getAttributeByName(xc_input, "action");
					std::string xc_input_button = getAttributeByName(xc_input, "button");

					xc_inputs_map[xc_input_action] = xc_input_button;

					xc_input = xc_input->getNextElementSibling();
				}

				State state;
				state.init(xc_state_name, xc_show_vec, xc_inputs_map);
				states.push_back(state);

				xc_state = xc_state->getNextElementSibling();
			}
		}
		xc::XMLPlatformUtils::Terminate();
	}

	void ParserErrorHandler::reportParseException(const xc::SAXParseException& ex)
	{
		char* msg = xc::XMLString::transcode(ex.getMessage());
		std::cout << "at line " << ex.getLineNumber() << " column " << ex.getColumnNumber() << msg << '\n';
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
}
