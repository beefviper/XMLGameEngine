// all.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

#include "load/reading.h"
#include "play/executor.h"

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

	VerbNames namesOf(const WrittenCommand& command) noexcept
	{
		const Verb* verb = findVerb(command.verb);
		return verb ? verb->names : VerbNames{};
	}

	bool alwaysLive(const WrittenCommand& command) noexcept
	{
		const Verb* verb = findVerb(command.verb);
		return verb && verb->alwaysLive;
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

	void printHopParts(std::ostream& out, const WrittenHop& hop)
	{
		if (hop.diagonal) { out << hop.diagonal->x << " " << hop.diagonal->y; }
		else if (hop.distance) { out << *hop.distance; }
		if (hop.seconds) { out << ", " << *hop.seconds << "s"; }
		if (hop.arc) { out << ", arc " << *hop.arc; }
		for (const std::string& onto : hop.onto) { out << ", onto " << onto; }
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
	static WrittenCommand readCommandHere(const XmlNode& node, const std::string& where);

	Game& Doer::game() const noexcept { return executor->game; }

	WrittenCommand readCommand(const XmlNode& node, const std::string& where)
	{
		return onLineOf(node, [&] { return readCommandHere(node, where); });
	}

	static WrittenCommand readCommandHere(const XmlNode& node, const std::string& where)
	{
		WrittenCommand command;
		command.verb = node.getName();
		command.line = node.line();

		const Verb* verb = findVerb(command.verb);
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

	bool readHopParts(const XmlNode& node, const std::string& where, WrittenHop& hop)
	{
		const std::string here = where + " > <hop>";
		if (auto seconds = findChild(&node, "seconds")) { hop.seconds = readValue(*seconds, here); }
		if (auto arc = findChild(&node, "arc")) { hop.arc = readValue(*arc, here); }

		// Nothing written: one cell, on a cell map.
		if (!node.getFirstChild() && readText(node).empty()) { return false; }
		if (!findChild(&node, "x") && !findChild(&node, "y"))
		{
			hop.distance = readLeadValue(node, { "seconds", "arc" }, "its distance, or an <x> and a <y>, then <seconds> and <arc> if wanted", where);
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
		hop.diagonal = WrittenVector2{ *step.x, *step.y };
		return true;
	}


	void readOnto(const XmlNode& node, const std::string& here, std::vector<std::string>& onto)
	{
		if (!readText(node).empty()) { fail(here, "has text; with <onto> it says nothing else"); }
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "onto") { fail(here, "unknown <" + child->getName() + ">; with <onto> it says nothing else"); }
			onto.push_back(requireAttribute(*child, "class", here + " > <onto>"));
		}
	}


	CellPlace readCellPlace(const XmlNode& node, const std::string& where)
	{
		CellPlace cells;
		std::unique_ptr<XmlNode> place = node.getFirstChild();
		if (!place || place->getNextSibling() || (place->getName() != "here" && place->getName() != "around"))
		{
			fail(where, "holds <" + (place ? place->getName() : std::string()) + ">; expected one <here /> or <around />");
		}
		cells.where = place->getName() == "here" ? CellPlace::Where::Here : CellPlace::Where::Around;
		for (std::unique_ptr<XmlNode> child = place->getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (cells.where == CellPlace::Where::Here) { fail(where + " > <here>", "holds <" + tag + ">; it is empty"); }
			if (tag == "straight" && !cells.straight) { cells.straight = true; continue; }
			if (tag == "alike" && cells.alike.empty()) { cells.alike = requireAttribute(*child, "variable", where + " > <around> > <alike>"); continue; }
			fail(where + " > <around>", "holds <" + tag + ">; expected <straight /> and <alike variable=\"...\" />, each once if wanted");
		}
		return cells;
	}


	void readSendHeld(const XmlNode& node, const std::string& where, WrittenCommand& command, bool takesAll)
	{
		const std::string& verb = command.verb;
		command.from = requireAttribute(node, "from", where);
		command.into = requireAttribute(node, "into", where);
		const std::string here = where + " > <" + verb + ">";
		WrittenSend& send = command.own.emplace<WrittenSend>();
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "all" || !takesAll) { fail(here, "unknown <" + child->getName() + ">; expected how many" + std::string(takesAll ? ", or <all />" : " each")); }
			send.all = true;
		}
		if (!send.all) { send.count = readValue(node, where); }
		else if (!readText(node).empty()) { fail(here, "has <all /> and a number; give one"); }
	}

	bool isWritten(const WrittenValue& value) noexcept
	{
		return !(value.kind == WrittenValue::Kind::Expression && value.text.empty());
	}

	float valueOr(const WrittenValue& value, float otherwise, const ValueEvaluator& evaluate)
	{
		return isWritten(value) ? evaluate(value) : otherwise;
	}

	float valueOr(const std::optional<WrittenValue>& value, float otherwise, const ValueEvaluator& evaluate)
	{
		return value ? evaluate(*value) : otherwise;
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

	float hopSeconds(const WrittenHop& hop, const ValueEvaluator& evaluate)
	{
		if (!hop.seconds) { return 0.0f; }
		const float seconds = evaluate(*hop.seconds);
		if (!(seconds > 0.0f)) { throw std::runtime_error("<hop> has <seconds> of " + formatDisplayNumber(seconds) + "; expected more than 0"); }
		return seconds;
	}

	float arcOf(const WrittenHop& hop, const char* what, float seconds, const ValueEvaluator& evaluate)
	{
		if (!hop.arc) { return 0.0f; }
		if (!(seconds > 0.0f)) { throw std::runtime_error(std::string("<") + what + "> has an <arc> but no <seconds>: only a hop that takes time leaves the ground"); }
		const float arc = evaluate(*hop.arc);
		if (arc < 0.0f) { throw std::runtime_error(std::string("<") + what + "> has an <arc> of " + formatDisplayNumber(arc) + "; expected 0 or more"); }
		return arc;
	}

	float speedOf(const WrittenValue& own, const char* what, const ValueEvaluator& evaluate)
	{
		const float speed = evaluate(own);
		if (speed < 0.0f) { throw std::runtime_error(std::string("<") + what + ">: the speed is below 0"); }
		return speed;
	}

	Command makeSendHeld(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		CmdSend send{ written.object, written.from, written.into, std::nullopt, std::nullopt, false, 0, false, {} };
		send.held = true;
		const WrittenSend& own = ownOf<WrittenSend>(written);
		send.all = own.all;
		send.sprite = written.sprite;
		if (!own.all)
		{
			const float count = own.count ? evaluate(*own.count) : 0.0f;
			if (count < 1.0f || count != std::floor(count)) { throw std::runtime_error("<" + written.verb + " from=\"" + written.from + "\" into=\"" + written.into + "\">: sends " + formatDisplayNumber(count) + "; expected a whole number from 1, or <all />"); }
			send.count = static_cast<int>(count);
		}
		return send;
	}
}
