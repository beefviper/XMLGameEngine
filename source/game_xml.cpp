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
		std::map<std::string, float>& variables, std::vector<RawState>& rawStates,
		std::vector<RawObject>& rawObjects)
	{
		domParser->setErrorHandler(&parserErrorHandler);
		domParser->setValidationScheme(xc::XercesDOMParser::Val_Auto);
		domParser->setDoNamespaces(true);
		domParser->setDoSchema(true);
		domParser->setValidationConstraintFatal(true);

		domParser->parse(filename.c_str());

		auto schemaLocation = getAttributeByName(domParser->getDocument()->getDocumentElement(), "xsi:noNamespaceSchemaLocation");
		auto errorCount = domParser->getErrorCount();

		if (errorCount == 0 && schemaLocation != "")
		{
			std::cout << "XML file validated against the schema successfully\n\n";
		}
		else if (errorCount == 0)
		{
			std::cout << "XML file was parsed successfully\n\n";
		}
		else if (errorCount > 0 && schemaLocation != "")
		{
			std::cout << "XML file failed to validate against the schema\n\n";
			exit(EXIT_FAILURE);
		}
		else if (errorCount > 0)
		{
			std::cout << "XML file failed to parse\n\n";
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
			std::string xc_obj_class = getAttributeByName(xc_object, "class");

			// find key points in object
			const auto* xc_sprite = xc_object->getFirstElementChild();
			const auto* xc_pos = xc_sprite->getNextElementSibling();
			const auto* xc_vel = xc_pos->getNextElementSibling();
			const auto* xc_collisions = xc_vel->getNextElementSibling();

			const xc::DOMElement* xc_actions = nullptr;
			const xc::DOMElement* xc_objvars = nullptr;

			if (xc_collisions) // hacky code, in case no actions are defined, by object variables are
			{
				const xc::DOMElement* xc_temp_element = xc_collisions->getNextElementSibling();
				if (xc_temp_element != nullptr)
				{
					const XMLCh* xc_temp_tag = xc_temp_element->getTagName();
					char* xc_temp_tag_char = xc::XMLString::transcode(xc_temp_tag);
					std::string xc_temp_tag_str(xc_temp_tag_char);

					if (xc_temp_tag_str == "actions")
					{
						xc_actions = xc_temp_element;
					}
					else if (xc_temp_tag_str == "variables")
					{
						xc_objvars = xc_temp_element;
					}
					xc::XMLString::release(&xc_temp_tag_char);
				}
			}

			if (xc_actions)
			{
				xc_objvars = xc_actions->getNextElementSibling();
			}

			// load sprite
			std::string xc_sprite_src = getAttributeByName(xc_sprite, "src");

			// load position
			std::string xc_pos_x = getAttributeByName(xc_pos, "x");
			std::string xc_pos_y = getAttributeByName(xc_pos, "y");

			// load velocity
			std::string xc_vel_x = getAttributeByName(xc_vel, "x");
			std::string xc_vel_y = getAttributeByName(xc_vel, "y");

			Vector2str position{ xc_pos_x, xc_pos_y };
			Vector2str velocity{ xc_vel_x, xc_vel_y };

			// load collisions
			std::string xc_collision_enabled = getAttributeByName(xc_collisions, "enabled");
			std::string xc_collision_group = getAttributeByName(xc_collisions, "group");

			RawCollisionData xc_collision_data;

			xc_collision_data.enabled = (xc_collision_enabled == "true") ? true : false;
			xc_collision_data.group = (xc_collision_group == "true") ? true : false;

			if (xc_collisions)
			{
				auto xc_collision = xc_collisions->getFirstElementChild();

				while (xc_collision != nullptr)
				{
					if (auto col_edge = getAttributeByName(xc_collision, "edge"); col_edge != "")
					{

						auto col_action = getAttributeByName(xc_collision, "action");

						if (col_edge == "all")
						{
							xc_collision_data.top = col_action;
							xc_collision_data.bottom = col_action;
							xc_collision_data.left = col_action;
							xc_collision_data.right = col_action;
						}
						else if (col_edge == "horizontal")
						{
							xc_collision_data.top = col_action;
							xc_collision_data.bottom = col_action;
						}
						else if (col_edge == "vertical")
						{
							xc_collision_data.left = col_action;
							xc_collision_data.right = col_action;
						}
						else if (col_edge == "top")
						{
							xc_collision_data.top = col_action;
						}
						else if (col_edge == "bottom")
						{
							xc_collision_data.bottom = col_action;
						}
						else if (col_edge == "left")
						{
							xc_collision_data.left = col_action;
						}
						else if (col_edge == "right")
						{
							xc_collision_data.right = col_action;
						}
					}

					// TODO: add code to handle class and object attributes in collision tag

					if (auto col_basic = getAttributeByName(xc_collision, "basic"); col_basic != "")
					{
						xc_collision_data.basic = getAttributeByName(xc_collision, "action");
					}

					xc_collision = xc_collision->getNextElementSibling();
				}
			}

			// load actions
			std::map<std::string, std::string> xc_action_map;

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

			// load object variables
			std::map<std::string, std::string> xc_objvar_map;

			if (xc_objvars)
			{
				auto xc_objvar = xc_objvars->getFirstElementChild();

				while (xc_objvar != nullptr)
				{
					std::string xc_objvar_name = getAttributeByName(xc_objvar, "name");
					std::string xc_objvar_value = getAttributeByName(xc_objvar, "value");
					xc_objvar_map[xc_objvar_name] = xc_objvar_value;

					xc_objvar = xc_objvar->getNextElementSibling();
				}
			}

			RawObject rawObject{};
			rawObject.name = xc_obj_name;
			rawObject.objClass = xc_obj_class;
			rawObject.src = xc_sprite_src;
			rawObject.action = xc_action_map;
			rawObject.variable = xc_objvar_map;
			rawObject.rawCollisionData = xc_collision_data;
			rawObject.rawPosition = position;
			rawObject.rawVelocity = velocity;

			rawObjects.push_back(std::move(rawObject));

			xc_object = xc_object->getNextElementSibling();
		}

		// load states
		while (xc_state != nullptr)
		{
			std::string xc_state_name = getAttributeByName(xc_state, "name");

			// load shows
			const auto* xc_shows = xc_state->getFirstElementChild();
			auto xc_show = xc_shows->getFirstElementChild();

			std::vector<std::string> xc_show_vec;

			while (xc_show != nullptr)
			{
				std::string xc_show_object = getAttributeByName(xc_show, "object");
				xc_show_vec.push_back(xc_show_object);
				xc_show = xc_show->getNextElementSibling();
			}

			// load inputs
			const auto* xc_inputs = xc_shows->getNextElementSibling();
			auto xc_input = xc_inputs->getFirstElementChild();

			std::map<std::string, std::string> xc_inputs_map;

			while (xc_input != nullptr)
			{
				std::string xc_input_button = getAttributeByName(xc_input, "button");
				std::string xc_input_action = getAttributeByName(xc_input, "action");

				xc_inputs_map[xc_input_button] = xc_input_action;

				xc_input = xc_input->getNextElementSibling();
			}

			// TODO: add code to load conditions

			RawState rawState{};
			rawState.name = xc_state_name;
			rawState.show = xc_show_vec;
			rawState.input = xc_inputs_map;
			rawStates.push_back(rawState);

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
