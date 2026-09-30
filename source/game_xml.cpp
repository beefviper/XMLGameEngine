// game_xml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_xml.h"

#include "xsd_lite.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace xge
{
	void game_xml::init(const std::string& filename, XmlBackend backend, WindowDesc& windowDesc,
		std::map<std::string, float>& variables, std::vector<RawState>& rawStates,
		std::vector<RawObject>& rawObjects, SchemaValidation& validation)
	{
		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);

		if (!document->load(filename))
		{
			std::cout << "XML file failed to load: " << document->getErrorMessage() << "\n\n";
			exit(EXIT_FAILURE);
		}

		// find key points in document
		std::unique_ptr<XmlNode> root = document->getRootElement();

		// A game file names its schema the same way regardless of backend
		// (xsi:noNamespaceSchemaLocation - see games/*.xml), so this reads
		// it directly off root rather than asking XmlDocument for it.
		// Xerces already ran its own real validation as part of load()
		// above whenever this is non-empty (see xml_xerces.cpp) - load()
		// would have failed already otherwise, so reaching here means it
		// passed. Every other backend can only check well-formedness on its
		// own (see xml_document.h), so this falls back to XsdLiteValidator
		// - this project's own interpreter for the small subset of XSD this
		// schema actually uses (see xsd_lite.h) - against that same schema
		// file, resolved relative to filename the same way Xerces resolves
		// it relative to the document.
		const std::string schemaLocation = getAttribute(root.get(), "xsi:noNamespaceSchemaLocation");

		if (schemaLocation.empty())
		{
			validation = SchemaValidation::None;
		}
		else if (backend == XmlBackend::Xerces)
		{
			validation = SchemaValidation::Strong;
		}
		else
		{
			const std::string schemaPath = (std::filesystem::path(filename).parent_path() / schemaLocation).string();

			XsdLiteValidator validator;

			if (!validator.loadSchema(schemaPath, backend) || !validator.validate(*root))
			{
				std::cout << "XML file failed to validate against the schema: " << validator.getErrorMessage() << "\n\n";
				exit(EXIT_FAILURE);
			}

			validation = SchemaValidation::Weak;
		}

		std::unique_ptr<XmlNode> window = findChild(root.get(), "window");
		std::unique_ptr<XmlNode> variablesNode = findChild(root.get(), "variables");
		std::unique_ptr<XmlNode> objectsNode = findChild(root.get(), "objects");
		std::unique_ptr<XmlNode> states = findChild(root.get(), "states");

		std::unique_ptr<XmlNode> variable = variablesNode->getFirstChild();
		std::unique_ptr<XmlNode> object = objectsNode->getFirstChild();
		std::unique_ptr<XmlNode> state = states->getFirstChild();

		// load window description
		windowDesc.name = getAttribute(window.get(), "name");
		windowDesc.width = std::stof(getAttribute(window.get(), "width"));
		windowDesc.height = std::stof(getAttribute(window.get(), "height"));
		windowDesc.background = getAttribute(window.get(), "background");
		windowDesc.fullscreen = getAttribute(window.get(), "fullscreen");
		windowDesc.framerate = std::stoi(getAttribute(window.get(), "framerate"));

		// load variables
		while (variable != nullptr)
		{
			std::string varName = getAttribute(variable.get(), "name");
			float varValue = std::stof(getAttribute(variable.get(), "value"));
			variables[varName] = varValue;
			variable = variable->getNextSibling();
		}

		// load objects
		while (object != nullptr)
		{
			std::string objName = getAttribute(object.get(), "name");
			std::string objClass = getAttribute(object.get(), "class");

			// find key points in object
			std::unique_ptr<XmlNode> sprite = findChild(object.get(), "sprite");
			std::unique_ptr<XmlNode> pos = findChild(object.get(), "position");
			std::unique_ptr<XmlNode> vel = findChild(object.get(), "velocity");
			std::unique_ptr<XmlNode> collisions = findChild(object.get(), "collisions");
			std::unique_ptr<XmlNode> actions = findChild(object.get(), "actions");
			std::unique_ptr<XmlNode> objvars = findChild(object.get(), "variables");

			// load sprite
			std::string spriteSrc = getAttribute(sprite.get(), "src");

			// load position
			std::string posX = getAttribute(pos.get(), "x");
			std::string posY = getAttribute(pos.get(), "y");

			// load velocity
			std::string velX = getAttribute(vel.get(), "x");
			std::string velY = getAttribute(vel.get(), "y");

			Vector2str position{ posX, posY };
			Vector2str velocity{ velX, velY };

			// load collisions
			std::string collisionEnabled = getAttribute(collisions.get(), "enabled");
			std::string collisionGroup = getAttribute(collisions.get(), "group");

			RawCollisionData collisionData;

			collisionData.enabled = (collisionEnabled == "true") ? true : false;
			collisionData.group = (collisionGroup == "true") ? true : false;

			if (collisions)
			{
				std::unique_ptr<XmlNode> collision = collisions->getFirstChild();

				// Appends rather than overwrites, so more than one <collision edge="..."/>
				// element touching the same edge (e.g. an "all" rule plus a specific
				// "left" rule) both run instead of the later one silently winning.
				auto appendAction = [](std::string& existing, const std::string& action)
				{
					if (!existing.empty()) { existing += ';'; }
					existing += action;
				};

				while (collision != nullptr)
				{
					if (auto colEdge = getAttribute(collision.get(), "edge"); colEdge != "")
					{
						auto colAction = getAttribute(collision.get(), "action");

						if (colEdge == "all")
						{
							appendAction(collisionData.top, colAction);
							appendAction(collisionData.bottom, colAction);
							appendAction(collisionData.left, colAction);
							appendAction(collisionData.right, colAction);
						}
						else if (colEdge == "horizontal")
						{
							appendAction(collisionData.left, colAction);
							appendAction(collisionData.right, colAction);
						}
						else if (colEdge == "vertical")
						{
							appendAction(collisionData.top, colAction);
							appendAction(collisionData.bottom, colAction);
						}
						else if (colEdge == "top")
						{
							appendAction(collisionData.top, colAction);
						}
						else if (colEdge == "bottom")
						{
							appendAction(collisionData.bottom, colAction);
						}
						else if (colEdge == "left")
						{
							appendAction(collisionData.left, colAction);
						}
						else if (colEdge == "right")
						{
							appendAction(collisionData.right, colAction);
						}
					}

					// class/object optionally narrow an object-object rule to only
					// respond to a specific class of object, or one specific named
					// object; either or both may be left off to match anything (the
					// old, unfiltered behaviour). Meaningless for edge rules (no "other
					// object" exists at a screen edge), so only read here.
					//
					// An object-object rule is written either the long way,
					// basic="basic" (optionally with a filter), or just by naming
					// what it applies to: a <collision> with class and/or object and
					// no edge is one too, so basic="basic" is only needed for a rule
					// that matches anything.
					const std::string colClass = getAttribute(collision.get(), "class");
					const std::string colObject = getAttribute(collision.get(), "object");
					const bool hasEdge = getAttribute(collision.get(), "edge") != "";

					if (auto colBasic = getAttribute(collision.get(), "basic");
						colBasic != "" || (!hasEdge && (colClass != "" || colObject != "")))
					{
						RawCollisionRule rule;
						rule.filterClass = colClass;
						rule.filterObject = colObject;
						rule.unlessClass = getAttribute(collision.get(), "unless");
						rule.action = getAttribute(collision.get(), "action");
						collisionData.basic.push_back(std::move(rule));
					}

					collision = collision->getNextSibling();
				}
			}

			// load actions
			std::map<std::string, std::string> actionMap;

			if (actions)
			{
				std::unique_ptr<XmlNode> action = actions->getFirstChild();

				while (action != nullptr)
				{
					std::string actName = getAttribute(action.get(), "name");
					std::string actValue = getAttribute(action.get(), "value");
					actionMap[actName] = actValue;

					action = action->getNextSibling();
				}
			}

			// load object variables
			std::map<std::string, std::string> objvarMap;

			if (objvars)
			{
				std::unique_ptr<XmlNode> objvar = objvars->getFirstChild();

				while (objvar != nullptr)
				{
					std::string objvarName = getAttribute(objvar.get(), "name");
					std::string objvarValue = getAttribute(objvar.get(), "value");
					objvarMap[objvarName] = objvarValue;

					objvar = objvar->getNextSibling();
				}
			}

			RawObject rawObject{};
			rawObject.name = objName;
			rawObject.objClass = objClass;
			rawObject.src = spriteSrc;
			rawObject.action = actionMap;
			rawObject.variable = objvarMap;
			rawObject.rawCollisionData = collisionData;
			rawObject.rawPosition = position;
			rawObject.rawVelocity = velocity;

			rawObjects.push_back(std::move(rawObject));

			object = object->getNextSibling();
		}

		// load states
		while (state != nullptr)
		{
			std::string stateName = getAttribute(state.get(), "name");

			// load shows
			std::unique_ptr<XmlNode> shows = findChild(state.get(), "shows");
			std::unique_ptr<XmlNode> show = shows->getFirstChild();

			std::vector<std::string> showVec;

			while (show != nullptr)
			{
				std::string showObject = getAttribute(show.get(), "object");
				showVec.push_back(showObject);
				show = show->getNextSibling();
			}

			// load inputs
			std::unique_ptr<XmlNode> inputs = findChild(state.get(), "inputs");
			std::unique_ptr<XmlNode> input = inputs->getFirstChild();

			std::map<std::string, std::string> inputsMap;

			while (input != nullptr)
			{
				std::string inputButton = getAttribute(input.get(), "button");
				std::string inputAction = getAttribute(input.get(), "action");

				inputsMap[inputButton] = inputAction;

				input = input->getNextSibling();
			}

			// load conditions (optional - the schema allows a state with none)
			std::vector<RawCondition> conditionsVec;

			if (std::unique_ptr<XmlNode> conditions = findChild(state.get(), "conditions"); conditions != nullptr)
			{
				std::unique_ptr<XmlNode> condition = conditions->getFirstChild();

				while (condition != nullptr)
				{
					RawCondition condRaw;
					condRaw.filterClass = getAttribute(condition.get(), "class");
					condRaw.filterObject = getAttribute(condition.get(), "object");
					condRaw.variableName = getAttribute(condition.get(), "variable");

					// Either variable + value, variable + atmost, or remaining
					// (see RawCondition).
					if (const std::string remaining = getAttribute(condition.get(), "remaining"); !remaining.empty())
					{
						condRaw.remaining = std::stof(remaining);
					}
					else if (const std::string atMost = getAttribute(condition.get(), "atmost"); !atMost.empty())
					{
						condRaw.atMost = std::stof(atMost);
					}
					else
					{
						const std::string value = getAttribute(condition.get(), "value");
						if (value.empty())
						{
							throw std::runtime_error("a <condition> needs either variable and value, variable and atmost, or remaining");
						}
						condRaw.value = std::stof(value);
					}
					condRaw.action = getAttribute(condition.get(), "action");
					conditionsVec.push_back(std::move(condRaw));

					condition = condition->getNextSibling();
				}
			}

			RawState rawState{};
			rawState.name = stateName;
			rawState.show = showVec;
			rawState.input = inputsMap;
			rawState.conditions = conditionsVec;
			rawStates.push_back(rawState);

			state = state->getNextSibling();
		}
	}

	std::unique_ptr<XmlNode> game_xml::findChild(const XmlNode* parent, const std::string& name)
	{
		std::unique_ptr<XmlNode> child = parent ? parent->getFirstChild() : nullptr;

		while (child != nullptr && child->getName() != name)
		{
			child = child->getNextSibling();
		}

		return child;
	}

	std::string game_xml::getAttribute(const XmlNode* node, const std::string& name)
	{
		return node ? node->getAttribute(name) : std::string{};
	}
}
