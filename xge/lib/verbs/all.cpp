// all.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

#include <array>
#include <cmath>
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
	extern const Verb moveVerb, hopVerb, jumpVerb, leapVerb, climbVerb, goVerb, accelerateVerb, turnVerb, thrustVerb, swingVerb, stopVerb, reverseVerb, trackVerb, followVerb, shiftVerb;
	// Steering.
	extern const Verb chaseVerb, aimVerb, huntVerb, wanderVerb;
	// In and out of play.
	extern const Verb dieVerb, resetVerb, spawnVerb, despawnVerb, fireVerb, releaseVerb, revealVerb, dropVerb;
	// Numbers.
	extern const Verb incVerb, decVerb, setVerb, copyVerb, countVerb, rankVerb;
	// Look and sound.
	extern const Verb becomeVerb, playVerb, silenceVerb;
	// Flow.
	extern const Verb pushVerb, popVerb, enterVerb, triggerVerb, turnsVerb, passVerb, outVerb, demoVerb, undoVerb;
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
			&moveVerb, &hopVerb, &jumpVerb, &leapVerb, &climbVerb, &goVerb, &accelerateVerb, &turnVerb, &thrustVerb, &swingVerb, &stopVerb, &reverseVerb, &trackVerb, &followVerb, &shiftVerb,
			&chaseVerb, &aimVerb, &huntVerb, &wanderVerb,
			&dieVerb, &resetVerb, &spawnVerb, &despawnVerb, &fireVerb, &releaseVerb, &revealVerb, &dropVerb,
			&incVerb, &decVerb, &setVerb, &copyVerb, &countVerb, &rankVerb,
			&becomeVerb, &playVerb, &silenceVerb,
			&pushVerb, &popVerb, &enterVerb, &triggerVerb, &turnsVerb, &passVerb, &outVerb, &demoVerb, &undoVerb,
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
