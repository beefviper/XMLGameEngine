// web.cpp
// XML Game Engine
// author: beefviper
// date: Nov 23, 2026

#include "audio/web.h"

#include <emscripten.h>

#include <cstdint>

// The page's side, kept in window.xgeSound: its AudioContext, a buffer a
// sound (by number) and the voice playing each, with whether it loops.

EM_JS(void, xge_sound_open, (int rate), {
	var AudioContextOf = window.AudioContext || window.webkitAudioContext;
	var sound = window.xgeSound = { buffers: [], voices: [], looping: [], context: AudioContextOf ? new AudioContextOf() : null };
	if (!sound.context) { return; }
	sound.rate = rate;
	function wake() { if (sound.context.state !== 'running') { sound.context.resume(); } }
	window.addEventListener('keydown', wake);
	window.addEventListener('pointerdown', wake);
});

EM_JS(int, xge_sound_load, (const std::int16_t* samples, int count), {
	var sound = window.xgeSound;
	if (!sound.context || count <= 0) { return -1; }
	var buffer = sound.context.createBuffer(1, count, sound.rate);
	var into = buffer.getChannelData(0);
	var from = samples >> 1;
	for (var i = 0; i < count; ++i) { into[i] = HEAP16[from + i] / 32768; }
	sound.buffers.push(buffer);
	sound.voices.push(null);
	sound.looping.push(false);
	return sound.buffers.length - 1;
});

EM_JS(void, xge_sound_stop, (int number), {
	var sound = window.xgeSound;
	var voice = sound.voices[number];
	if (voice) { try { voice.stop(); } catch (error) {} }
	sound.voices[number] = null;
	sound.looping[number] = false;
});

EM_JS(void, xge_sound_play, (int number, int loop), {
	var sound = window.xgeSound;
	if (!sound.context || !sound.buffers[number]) { return; }
	if (loop && sound.looping[number] && sound.voices[number]) { return; }
	var voice = sound.voices[number];
	if (voice) { try { voice.stop(); } catch (error) {} }
	voice = sound.context.createBufferSource();
	voice.buffer = sound.buffers[number];
	voice.loop = loop !== 0;
	voice.connect(sound.context.destination);
	voice.onended = function() { if (sound.voices[number] === voice) { sound.voices[number] = null; sound.looping[number] = false; } };
	voice.start();
	sound.voices[number] = voice;
	sound.looping[number] = loop !== 0;
});

namespace xge
{
	WebAudio::WebAudio()
	{
		xge_sound_open(static_cast<int>(kSoundSampleRate));
	}

	void WebAudio::load(const std::vector<SoundDesc>& descs, Resources& resources)
	{
		stopAll();
		sounds.clear();
		for (const SoundDesc& sound : descs)
		{
			const auto samples = resources.samples(sound);
			const int number = xge_sound_load(samples->data(), static_cast<int>(samples->size()));
			if (number >= 0) { sounds[sound.name] = number; }
		}
	}

	void WebAudio::play(const std::string& name)
	{
		if (const auto found = sounds.find(name); found != sounds.end()) { xge_sound_play(found->second, 0); }
	}

	void WebAudio::loop(const std::string& name)
	{
		if (const auto found = sounds.find(name); found != sounds.end()) { xge_sound_play(found->second, 1); }
	}

	void WebAudio::stop(const std::string& name)
	{
		if (const auto found = sounds.find(name); found != sounds.end()) { xge_sound_stop(found->second); }
	}

	void WebAudio::stopAll()
	{
		for (const auto& [name, number] : sounds)
		{
			(void)name;
			xge_sound_stop(number);
		}
	}
}
