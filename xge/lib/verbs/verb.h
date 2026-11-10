// verb.h
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#pragma once

#include "game/command.h"
#include "game/object.h"
#include "play/geometry.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

// A verb is one command tag of the language: <bounce />, <hop>, <spawn>.
// Each has a file of its own in this folder that says everything the engine
// knows about it: its name, where it may be written, how its numbers are
// worked out and what it does. verbs/all.cpp lists them.

namespace xge
{
	class CommandExecutor;
	class XmlNode;
	struct Checking;

	// Where a list of commands runs.
	enum class Place { Edge, Touch, Input, Condition, StateTimer, ObjectTimer, Action, PathStep, Computer, Check };

	// Some of the places, added up.
	using Places = unsigned;
	constexpr Places in(Place place) noexcept { return 1u << static_cast<unsigned>(place); }

	// The places the schema's groups of commands go (xgedef.xsd,
	// changesCommands and the rest), narrowed to where each does something.
	namespace places
	{
		// What changes the game: anywhere but a <computer>.
		constexpr Places changes = in(Place::Edge) | in(Place::Touch) | in(Place::Input) | in(Place::Condition) | in(Place::StateTimer)
			| in(Place::ObjectTimer) | in(Place::Action) | in(Place::PathStep) | in(Place::Check);
		// The game's own: a state's lists.
		constexpr Places flow = in(Place::Input) | in(Place::Condition) | in(Place::StateTimer);
		// About the object running it.
		constexpr Places self = in(Place::Edge) | in(Place::Touch) | in(Place::ObjectTimer) | in(Place::Action) | in(Place::PathStep);
		constexpr Places steering = in(Place::Touch) | in(Place::ObjectTimer) | in(Place::Action) | in(Place::PathStep);
		// About what it touched, and the window's edge.
		constexpr Places touch = in(Place::Touch);
		constexpr Places edge = in(Place::Edge);
		// The controls an action holds.
		constexpr Places controls = in(Place::Action);
		// What a <computer> does.
		constexpr Places press = in(Place::Computer);
		// About what a map's check found.
		constexpr Places cells = in(Place::Check);
	}

	// Which of the Command's kinds a Cmd struct is: kindOf<CmdBounce>().
	template <typename Wanted, typename... Kinds>
	constexpr std::size_t kindAmong(const std::variant<Kinds...>*) noexcept
	{
		const bool same[] = { std::is_same_v<Wanted, Kinds>... };
		std::size_t index = 0;
		while (index < sizeof...(Kinds) && !same[index]) { ++index; }
		return index;
	}
	template <typename Wanted>
	constexpr std::size_t kindOf() noexcept { return kindAmong<Wanted>(static_cast<const Command*>(nullptr)); }

	// Who runs a command, and where: what every verb looks at to know what
	// to do. The places a verb may be written are its forms' (below); this
	// runs whatever got there.
	struct Doer
	{
		// The object the command is about (a rule's, a timer's, an
		// action's); none for a state's list, where a bare <reset /> is
		// the whole game.
		Object* self{ nullptr };
		// What it touched, for a rule about another object.
		Object* other{ nullptr };
		const Touch* touch{ nullptr };
		// The window edge, for a screen-edge rule.
		std::optional<Edge> screenEdge;
		// False for a key let go: only what is held stops (Verb::onRelease).
		bool pressed{ true };
		// Run by a rule, a timer or a condition, not by a key.
		bool byRule{ true };
		// In an object's action of this name: a <move> is held, and the
		// controls (<hop>, <jump>, <climb>, <thrust>...) work.
		const std::string* action{ nullptr };
		// In a cell map's check: what it found.
		const Object::CellMatch* match{ nullptr };
	};

	// One way a verb is written: how a message names it, where it may go
	// and the command it makes. Most verbs have one; <reset /> has three.
	struct Form
	{
		const char* tag{ nullptr };
		Places places{ 0 };
		std::size_t kind{ 0 };
	};

	struct Verb
	{
		// The tag: "bounce".
		const char* name;
		// Its forms; unused ones have no tag.
		Form forms[3];
		// Which form a command is, for a verb whose forms make the same
		// kind of command; none: the first of that kind.
		std::size_t (*formOf)(const Command& command);
		// Acts when a key is let go as well: a held verb stops.
		bool onRelease;
		// What its tag says besides the names every command may have (see
		// readCommand): the attributes it must have, its content and its
		// children, into `command`. None: it says nothing more. Stops the
		// load (fail, load/reading.h) at anything missing or unknown.
		void (*read)(const XmlNode& node, const std::string& where, RawCommand& command);
		// What was written, its numbers worked out. Throws
		// std::runtime_error naming what is wrong with it.
		Command (*make)(const RawCommand& raw, const ValueEvaluator& evaluate);
		// The names it uses exist (load/check.h); none: it names nothing.
		// Throws std::runtime_error saying which does not.
		void (*check)(const Command& command, const Checking& checking);
		// Does it, as `doer` says. False when it is refused, which ends the
		// list it is in.
		bool (*run)(const Command& command, const Doer& doer, CommandExecutor& executor);
	};

	// Every verb, in the order of docs/reference/commands.md.
	const std::vector<const Verb*>& allVerbs();

	// The verb of that tag name; null when there is none.
	const Verb* findVerb(std::string_view name) noexcept;

	// The verb a command is of, and which of its forms; null for what is not
	// a verb's (a condition in an action, a command made as it runs: see
	// shapeOf).
	const Verb* verbOf(const Command& command) noexcept;
	const Form* formOf(const Command& command) noexcept;

	// A command tag as written, read by its verb; an unknown tag stops the
	// load.
	RawCommand readCommand(const XmlNode& node, const std::string& where);

	// --- What several verbs' read() share (more in load/reading.h).

	// Whether a tag holds anything: text or a child.
	bool hasContent(const XmlNode& node);
	// What a <hop> says besides its direction: one distance as its content,
	// or an <x> across and a <y> up or down; then how long it takes, its
	// <seconds>, and how high it goes, its <arc>. True for an <x> and a <y>.
	bool readHopParts(const XmlNode& node, const std::string& where, RawCommand& command);
	// <onto class="hslot" />, one or more and nothing else: the cells a hop
	// or a track on cells may go to.
	void readOnto(const XmlNode& node, const std::string& here, RawCommand& command);
	// A <count>'s or <trigger>'s cells: one <here /> or <around>, the latter
	// holding <straight /> and an <alike variable> if wanted.
	void readCellPlace(const XmlNode& node, const std::string& where, RawCommand& command);
	// Members of a held class, holder to holder: how many, or <all />; a
	// <distribute>: how many each, into a holder or a group's own ones.
	void readSendHeld(const XmlNode& node, const std::string& where, RawCommand& command);

	// --- What several verbs' make() share.

	// Whether anything was written for a value.
	bool isWritten(const RawValue& value) noexcept;
	// The value, or `otherwise` when nothing was written.
	float valueOr(const RawValue& value, float otherwise, const ValueEvaluator& evaluate);

	// up, down, left or right; anything else throws, naming the verb.
	Direction directionFromName(const std::string& name, const std::string& verb);
	// A turn's way: a rotation, clockwise (adds to the heading, kept as
	// Right) or counterclockwise (takes from it, kept as Left).
	Direction rotationFromName(const std::string& name, const std::string& verb);
	// <here /> or <around /> as written in a <count> or a <trigger>.
	CellPlace cellPlaceOf(const RawCommand& raw);
	// How long a <hop> takes: no <seconds>, none at all.
	float hopSeconds(const RawCommand& raw, const ValueEvaluator& evaluate);
	// How high a timed <hop> or <jump> goes: no <arc>, not at all.
	float arcOf(const RawCommand& raw, const char* what, float seconds, const ValueEvaluator& evaluate);
	// The speed of a <go>, <hunt> or <wander>, which may not be below 0.
	float speedOf(const RawCommand& raw, const char* what, const ValueEvaluator& evaluate);
	// What one holder holds, sent to another: a <distribute>, or a <send>
	// that names no object.
	Command makeSendHeld(const RawCommand& raw, const ValueEvaluator& evaluate);
}
