// all.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

#include "load/reading.h"

#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace xge
{
	// The list of the verbs: each is in the file of its name in this folder.
	// A new verb is a file there, a line here and, in game/command.h, the
	// Cmd struct it makes. Written out rather than each file adding itself:
	// a linker leaves out of a library a file nothing names.
	// Touching.
	extern const Verb bounceVerb, deflectVerb, knockVerb, stickVerb, wrapVerb, holeVerb, rideVerb, carryVerb, landVerb, throughVerb, shoveVerb, mergeVerb;
	// Moving.
	extern const Verb moveVerb, hopVerb, jumpVerb, climbVerb, goVerb, turnVerb, thrustVerb, swingVerb, stopVerb, reverseVerb, trackVerb, followVerb, shiftVerb;
	// Steering.
	extern const Verb chaseVerb, aimVerb, huntVerb, wanderVerb;
	// In and out of play.
	extern const Verb dieVerb, resetVerb, spawnVerb, despawnVerb, fireVerb, revealVerb, dropVerb;
	// Numbers.
	extern const Verb incVerb, decVerb, setVerb, copyVerb, countVerb, rankVerb;
	// Look and sound.
	extern const Verb becomeVerb, playVerb, silenceVerb;
	// Flow.
	extern const Verb pushVerb, popVerb, enterVerb, triggerVerb, alternateVerb, passVerb, outVerb, demoVerb, undoVerb;
	// Cells.
	extern const Verb settleVerb, slideVerb, cycleVerb, growVerb, splitVerb, clearVerb, collapseVerb, fallVerb;
	// Boards and holders.
	extern const Verb pickVerb, placeVerb, putVerb, callVerb, sendVerb, distributeVerb, orderVerb;
	// A computer's.
	extern const Verb holdVerb, thinkVerb;

	const std::vector<const Verb*>& allVerbs()
	{
		static const std::vector<const Verb*> verbs{
			&bounceVerb, &deflectVerb, &knockVerb, &stickVerb, &wrapVerb, &holeVerb, &rideVerb, &carryVerb, &landVerb, &throughVerb, &shoveVerb, &mergeVerb,
			&moveVerb, &hopVerb, &jumpVerb, &climbVerb, &goVerb, &turnVerb, &thrustVerb, &swingVerb, &stopVerb, &reverseVerb, &trackVerb, &followVerb, &shiftVerb,
			&chaseVerb, &aimVerb, &huntVerb, &wanderVerb,
			&dieVerb, &resetVerb, &spawnVerb, &despawnVerb, &fireVerb, &revealVerb, &dropVerb,
			&incVerb, &decVerb, &setVerb, &copyVerb, &countVerb, &rankVerb,
			&becomeVerb, &playVerb, &silenceVerb,
			&pushVerb, &popVerb, &enterVerb, &triggerVerb, &alternateVerb, &passVerb, &outVerb, &demoVerb, &undoVerb,
			&settleVerb, &slideVerb, &cycleVerb, &growVerb, &splitVerb, &clearVerb, &collapseVerb, &fallVerb,
			&pickVerb, &placeVerb, &putVerb, &callVerb, &sendVerb, &distributeVerb, &orderVerb,
			&holdVerb, &thinkVerb,
		};
		return verbs;
	}

	const Verb* findVerb(std::string_view name) noexcept
	{
		for (const Verb* verb : allVerbs())
		{
			if (name == verb->name) { return verb; }
		}
		return nullptr;
	}

	namespace
	{
		// The verb and the first form of each kind of command.
		struct Owner
		{
			const Verb* verb{ nullptr };
			const Form* form{ nullptr };
		};

		const std::array<Owner, std::variant_size_v<Command>>& owners()
		{
			static const std::array<Owner, std::variant_size_v<Command>> table = []
			{
				std::array<Owner, std::variant_size_v<Command>> made{};
				for (const Verb* verb : allVerbs())
				{
					for (const Form& form : verb->forms)
					{
						if (form.tag && !made[form.kind].verb) { made[form.kind] = { verb, &form }; }
					}
				}
				return made;
			}();
			return table;
		}
	}

	const Verb* verbOf(const Command& command) noexcept
	{
		return owners()[shapeOf(command).index()].verb;
	}

	const Form* formOf(const Command& command) noexcept
	{
		const Command& shape = shapeOf(command);
		const Owner& owner = owners()[shape.index()];
		if (owner.verb && owner.verb->formOf) { return &owner.verb->forms[owner.verb->formOf(shape)]; }
		return owner.form;
	}

	std::string cellPlaceText(const CellPlace& place)
	{
		if (!place.any()) { return {}; }
		std::string text = place.where == CellPlace::Where::Here ? ", here" : ", around";
		if (place.straight) { text += " straight"; }
		if (!place.alike.empty()) { text += " alike " + place.alike; }
		return text;
	}

	// The tag read, readCommand without the line.
	static RawCommand readCommandHere(const XmlNode& node, const std::string& where);

	RawCommand readCommand(const XmlNode& node, const std::string& where)
	{
		return onLineOf(node, [&] { return readCommandHere(node, where); });
	}

	static RawCommand readCommandHere(const XmlNode& node, const std::string& where)
	{
		RawCommand command;
		command.verb = node.getName();

		const Verb* verb = findVerb(command.verb);
		if (!verb && command.verb == "accelerate") { fail(where, "<accelerate direction=...> is now <thrust direction=...>"); }
		if (!verb) { fail(where, "unknown command <" + command.verb + ">"); }

		// The names any command may have, each an attribute.
		command.object = node.getAttribute("object");
		command.variable = node.getAttribute("variable");
		command.state = node.getAttribute("state");
		command.action = node.getAttribute("action");
		command.direction = node.getAttribute("direction");
		command.burn = node.getAttribute("burn");
		command.sound = node.getAttribute("sound");
		command.sprite = node.getAttribute("sprite");
		command.style = node.getAttribute("style");
		command.path = node.getAttribute("path");
		command.objClass = node.getAttribute("class");
		command.scene = node.getAttribute("scene");
		command.way = node.getAttribute("way");
		command.into = node.getAttribute("into");
		command.from = node.getAttribute("from");

		if (verb->read) { verb->read(node, where, command); }
		return command;
	}

	bool hasContent(const XmlNode& node)
	{
		return node.getFirstChild() || !readText(node).empty();
	}

	bool readHopParts(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		const std::string here = where + " > <hop>";
		if (auto seconds = findChild(&node, "seconds")) { command.seconds = readValue(*seconds, here); }
		if (auto arc = findChild(&node, "arc")) { command.arc = readValue(*arc, here); }

		// Nothing written: one cell, on a cell map.
		if (!node.getFirstChild() && readText(node).empty()) { return false; }
		if (!findChild(&node, "x") && !findChild(&node, "y"))
		{
			command.amount = readLeadValue(node, { "seconds", "arc" }, "its distance, or an <x> and a <y>, then <seconds> and <arc> if wanted", where);
			return false;
		}

		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag != "x" && tag != "y" && tag != "seconds" && tag != "arc")
			{
				fail(here, "unknown <" + tag + ">; expected its distance, or an <x> and a <y>, then <seconds> and <arc> if wanted");
			}
		}
		const PartialVector2 step = readPartialVector2(node, where);
		if (!step.x || !step.y || !readText(node).empty()) { fail(where, "<hop> gives one distance, or an <x> across and a <y> up or down, both"); }
		command.positionX = *step.x;
		command.positionY = *step.y;
		return true;
	}


	void readOnto(const XmlNode& node, const std::string& here, RawCommand& command)
	{
		if (!readText(node).empty()) { fail(here, "has text; with <onto> it says nothing else"); }
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "onto") { fail(here, "unknown <" + child->getName() + ">; with <onto> it says nothing else"); }
			command.onto.push_back(requireAttribute(*child, "class", here + " > <onto>"));
		}
	}


	void readCellPlace(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		std::unique_ptr<XmlNode> place = node.getFirstChild();
		if (!place || place->getNextSibling() || (place->getName() != "here" && place->getName() != "around"))
		{
			fail(where, "holds <" + (place ? place->getName() : std::string()) + ">; expected one <here /> or <around />");
		}
		command.place = place->getName();
		for (std::unique_ptr<XmlNode> child = place->getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (command.place == "here") { fail(where + " > <here>", "holds <" + tag + ">; it is empty"); }
			if (tag == "straight" && !command.straight) { command.straight = true; continue; }
			if (tag == "alike" && command.alike.empty()) { command.alike = requireAttribute(*child, "variable", where + " > <around> > <alike>"); continue; }
			fail(where + " > <around>", "holds <" + tag + ">; expected <straight /> and <alike variable=\"...\" />, each once if wanted");
		}
	}


	void readSendHeld(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		const std::string& verb = command.verb;
		command.from = requireAttribute(node, "from", where);
		command.into = requireAttribute(node, "into", where);
		const std::string here = where + " > <" + verb + ">";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "all" || verb == "distribute") { fail(here, "unknown <" + child->getName() + ">; expected how many" + std::string(verb == "send" ? ", or <all />" : " each")); }
			command.all = true;
		}
		if (!command.all) { command.amount = readValue(node, where); }
		else if (!readText(node).empty()) { fail(here, "has <all /> and a number; give one"); }
	}

	bool isWritten(const RawValue& value) noexcept
	{
		return !(value.kind == RawValue::Kind::Expression && value.text.empty());
	}

	float valueOr(const RawValue& value, float otherwise, const ValueEvaluator& evaluate)
	{
		return isWritten(value) ? evaluate(value) : otherwise;
	}

	Direction directionFromName(const std::string& name, const std::string& verb)
	{
		if (name == "up")    { return Direction::Up; }
		if (name == "down")  { return Direction::Down; }
		if (name == "left")  { return Direction::Left; }
		if (name == "right") { return Direction::Right; }

		throw std::runtime_error("<" + verb + "> has direction=\"" + name + "\"; expected up, down, left or right");
	}

	Direction rotationFromName(const std::string& name, const std::string& verb)
	{
		if (name == "clockwise")        { return Direction::Right; }
		if (name == "counterclockwise") { return Direction::Left; }

		throw std::runtime_error("<" + verb + "> has direction=\"" + name + "\"; expected clockwise or counterclockwise");
	}

	CellPlace cellPlaceOf(const RawCommand& raw)
	{
		CellPlace place;
		if (raw.place == "here") { place.where = CellPlace::Where::Here; }
		else if (raw.place == "around") { place.where = CellPlace::Where::Around; }
		place.straight = raw.straight;
		place.alike = raw.alike;
		return place;
	}

	float hopSeconds(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (!isWritten(raw.seconds)) { return 0.0f; }
		const float seconds = evaluate(raw.seconds);
		if (!(seconds > 0.0f)) { throw std::runtime_error("<hop> has <seconds> of " + formatDisplayNumber(seconds) + "; expected more than 0"); }
		return seconds;
	}

	float arcOf(const RawCommand& raw, const char* what, float seconds, const ValueEvaluator& evaluate)
	{
		if (!isWritten(raw.arc)) { return 0.0f; }
		if (!(seconds > 0.0f)) { throw std::runtime_error(std::string("<") + what + "> has an <arc> but no <seconds>: only a hop that takes time leaves the ground"); }
		const float arc = evaluate(raw.arc);
		if (arc < 0.0f) { throw std::runtime_error(std::string("<") + what + "> has an <arc> of " + formatDisplayNumber(arc) + "; expected 0 or more"); }
		return arc;
	}

	float speedOf(const RawCommand& raw, const char* what, const ValueEvaluator& evaluate)
	{
		const float speed = evaluate(raw.amount);
		if (speed < 0.0f) { throw std::runtime_error(std::string("<") + what + ">: the speed is below 0"); }
		return speed;
	}

	Command makeSendHeld(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		CmdSend send{ raw.object, raw.from, raw.into, std::nullopt, std::nullopt, false, 0, false, {} };
		send.held = true;
		send.all = raw.all;
		send.sprite = raw.sprite;
		if (!raw.all)
		{
			const float count = evaluate(raw.amount);
			if (count < 1.0f || count != std::floor(count)) { throw std::runtime_error("<" + raw.verb + " from=\"" + raw.from + "\" into=\"" + raw.into + "\">: sends " + formatDisplayNumber(count) + "; expected a whole number from 1, or <all />"); }
			send.count = static_cast<int>(count);
		}
		return send;
	}
}
