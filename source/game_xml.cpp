// game_xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_xml.h"

namespace xge
{
	game_xml::game_xml()
	{
		xc::XMLPlatformUtils::Initialize("en_US");
		domParser = new xc::XercesDOMParser();
	}

	game_xml::~game_xml()
	{
		delete domParser;
		xc::XMLPlatformUtils::Terminate();
	}

	void game_xml::init(std::string& filename, WindowDesc& windowDesc,
		std::map<std::string, float>& variables, std::vector<State>& states,
		std::vector<Object>& preObjects)
	{
		if (domParser->loadGrammar("games/assets/xmlgameengine.xsd", xc::Grammar::SchemaGrammarType) == NULL)
		{
			std::cout << "couldn't load schema\n\n";
			exit(EXIT_FAILURE);
		}

		domParser->setErrorHandler(&parserErrorHandler);
		domParser->setValidationScheme(xc::XercesDOMParser::Val_Auto);
		domParser->setDoNamespaces(true);
		domParser->setDoSchema(true);
		domParser->setValidationConstraintFatal(true);

		domParser->parse(filename.c_str());

		if (domParser->getErrorCount() == 0)
		{
			std::cout << "XML file validated against the schema successfully\n\n";
		}
		else
		{
			std::cout << "XML file doesn't conform to the schema\n\n";
			exit(EXIT_FAILURE);
		}

		// find key points in document
		const auto* xc_doc = domParser->getDocument();
		const auto* xc_root = xc_doc->getDocumentElement();
		const auto* xc_window = xc_root->getFirstElementChild();
		const auto* xc_variables = xc_window->getNextElementSibling();
		auto xc_variable = xc_variables->getFirstElementChild();
		const auto* xc_objects = xc_variables->getNextElementSibling();
		auto xc_object = xc_objects->getFirstElementChild();
		const auto* xc_states = xc_objects->getNextElementSibling();
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

			const auto* xc_sprite = xc_object->getFirstElementChild();
			std::string xc_sprite_src = getAttributeByName(xc_sprite, "src");

			const auto* xc_pos = xc_sprite->getNextElementSibling();
			std::string xc_pos_x = getAttributeByName(xc_pos, "x");
			std::string xc_pos_y = getAttributeByName(xc_pos, "y");

			const auto* xc_vel = xc_pos->getNextElementSibling();
			std::string xc_vel_x = getAttributeByName(xc_vel, "x");
			std::string xc_vel_y = getAttributeByName(xc_vel, "y");

			Vector2str position{ xc_pos_x, xc_pos_y };
			Vector2str velocity{ xc_vel_x, xc_vel_y };

			const auto* xc_collision = xc_vel->getNextElementSibling();
			std::string xc_collision_enabled = getAttributeByName(xc_collision, "enabled");

			CollisionData xc_collision_data;
			xc_collision_data.top = getAttributeByName(xc_collision, "top");
			xc_collision_data.bottom = getAttributeByName(xc_collision, "bottom");
			xc_collision_data.left = getAttributeByName(xc_collision, "left");
			xc_collision_data.right = getAttributeByName(xc_collision, "right");
			xc_collision_data.basic = getAttributeByName(xc_collision, "basic");
			const bool xc_collision_state = (xc_collision_enabled == "true") ? true : false;

			std::map<std::string, std::string> xc_action_map;

			const auto* xc_actions = xc_collision->getNextElementSibling();
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

			//Object object{};
			//object.name = xc_obj_name;
			//object.src = xc_sprite_src;
			//object.action = xc_action_map;
			//object.collision = xc_collision_state;
			//object.collisionData = xc_collision_data;
			//object.sposition = position;
			//object.svelocity = velocity;

			//objects.push_back(std::move(object));

			Object preObject{};
			preObject.name = xc_obj_name;
			preObject.src = xc_sprite_src;
			preObject.action = xc_action_map;
			preObject.collision = xc_collision_state;
			preObject.collisionData = xc_collision_data;
			preObject.sposition = position;
			preObject.svelocity = velocity;

			preObjects.push_back(std::move(preObject));

			xc_object = xc_object->getNextElementSibling();
		}

		// load states
		while (xc_state != nullptr)
		{
			std::string xc_state_name = getAttributeByName(xc_state, "name");

			const auto* xc_shows = xc_state->getFirstElementChild();
			auto xc_show = xc_shows->getFirstElementChild();

			std::vector<std::string> xc_show_vec;

			while (xc_show != nullptr)
			{
				std::string xc_show_object = getAttributeByName(xc_show, "object");
				xc_show_vec.push_back(xc_show_object);
				xc_show = xc_show->getNextElementSibling();
			}

			const auto* xc_inputs = xc_shows->getNextElementSibling();
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

	void game_xml::ParserErrorHandler::reportParseException(const xc::SAXParseException& ex)
	{
		char* msg = xc::XMLString::transcode(ex.getMessage());
		std::cout << "at line " << ex.getLineNumber() << " column " << ex.getColumnNumber() << " " << msg << '\n';
		xc::XMLString::release(&msg);
	}

	void game_xml::ParserErrorHandler::warning(const xc::SAXParseException& ex)
	{
		reportParseException(ex);
	}

	void game_xml::ParserErrorHandler::error(const xc::SAXParseException& ex)
	{
		reportParseException(ex);
	}

	void game_xml::ParserErrorHandler::fatalError(const xc::SAXParseException& ex)
	{
		reportParseException(ex);
	}

	void game_xml::ParserErrorHandler::resetErrors() noexcept
	{

	}

	std::string game_xml::getAttributeByName(const xc::DOMElement* element, const std::string& attribute)
	{
		XMLCh* attr = xc::XMLString::transcode(attribute.c_str());
		const XMLCh* value = element->getAttribute(attr);
		char* val = xc::XMLString::transcode(value);
		std::string v = val;
		xc::XMLString::release(&val);
		xc::XMLString::release(&attr);
		return v;
	}
}
