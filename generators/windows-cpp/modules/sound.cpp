// sound.cpp
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

#include "sound.h"
#include "trace.h"

namespace sound
{
	float pitch(const std::string& name)
	{
		return xge::pitchFromName(name).value_or(0.0f);
	}

	Note rest(float seconds)
	{
		return Note{ Wave::Square, 0.0f, 0.0f, seconds };
	}

	bool Sound::make(const std::string& soundName, float volume, const std::vector<Note>& notes)
	{
		name = soundName;

		xge::SoundDesc desc;
		desc.name = soundName;
		desc.volume = volume;
		for (const Note& note : notes)
		{
			xge::SoundNote made;
			made.rest = note.hertz <= 0.0f;
			made.wave = note.wave;
			made.startHz = note.hertz;
			made.endHz = note.slideTo > 0.0f ? note.slideTo : note.hertz;
			made.seconds = note.seconds;
			desc.notes.push_back(made);
		}

		const std::vector<std::int16_t> samples = xge::synthesize(desc);
		if (samples.empty() || !buffer.loadFromSamples(samples.data(), samples.size(), 1, xge::kSoundSampleRate, { sf::SoundChannel::Mono }))
		{
			return false;
		}

		voice.emplace(buffer);
		return true;
	}

	void Sound::play()
	{
#ifdef XGE_TRACE
		// A game played for the frame trace is silent (and quick), and says which
		// sound it would have played: see trace.h.
		if (trace::state().tracing)
		{
			trace::heard(name);
			return;
		}
#endif
		if (voice) { voice->play(); }
	}
}
