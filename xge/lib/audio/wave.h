// wave.h
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026

#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace xge
{
	// The longest a recorded sound may be, so that a wrong file (a whole song
	// or an hour of silence) is a load error instead of memory filled.
	constexpr double kMaxSoundFileSeconds = 60.0;

	// The samples of a WAV file, made like every other sound's (audio/sound.h):
	// signed 16 bit, one channel, kSoundSampleRate a second. It reads
	// uncompressed PCM, 8 or 16 bit, one or two channels, at any rate; two
	// channels are averaged into one, and another rate is resampled by a
	// straight line between neighbouring samples. Throws std::runtime_error,
	// naming the file, when it is missing, not a WAV, compressed, of another
	// size of sample or more channels, or longer than kMaxSoundFileSeconds.
	std::vector<std::int16_t> readWave(const std::filesystem::path& file);
}
