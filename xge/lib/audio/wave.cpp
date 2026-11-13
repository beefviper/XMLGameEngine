// wave.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026

#include "audio/wave.h"

#include "audio/sound.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace xge
{
	namespace
	{
		// A WAV is little endian whatever the machine.
		std::uint32_t little(const std::vector<unsigned char>& bytes, std::size_t at, int count)
		{
			std::uint32_t value = 0;
			for (int i = count - 1; i >= 0; --i) { value = (value << 8) | bytes[at + static_cast<std::size_t>(i)]; }
			return value;
		}

		bool tagAt(const std::vector<unsigned char>& bytes, std::size_t at, const char* tag)
		{
			for (std::size_t i = 0; i < 4; ++i)
			{
				if (bytes[at + i] != static_cast<unsigned char>(tag[i])) { return false; }
			}
			return true;
		}

		// WAVE_FORMAT_PCM, and WAVE_FORMAT_EXTENSIBLE (which says PCM again in
		// its sub-format).
		constexpr std::uint32_t kPcm = 1;
		constexpr std::uint32_t kExtensible = 0xFFFE;
	}

	std::vector<std::int16_t> readWave(const std::filesystem::path& file)
	{
		const auto fail = [&](const std::string& why)
		{
			return std::runtime_error("the sound file '" + file.generic_string() + "' " + why);
		};

		std::ifstream in(file, std::ios::binary);
		if (!in) { throw fail("cannot be opened"); }
		const std::vector<unsigned char> bytes{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
		if (bytes.size() < 12 || !tagAt(bytes, 0, "RIFF") || !tagAt(bytes, 8, "WAVE")) { throw fail("is not a WAV file"); }

		// The chunks after the header, each a tag, a size and that many bytes
		// (padded to an even number): "fmt " says what the samples are, "data"
		// holds them, the rest (names, cue points...) are passed over.
		std::uint32_t format = 0;
		std::uint32_t channels = 0;
		std::uint32_t rate = 0;
		std::uint32_t bits = 0;
		bool haveFormat = false;
		std::size_t dataAt = 0;
		std::size_t dataSize = 0;
		bool haveData = false;
		for (std::size_t at = 12; at + 8 <= bytes.size();)
		{
			const std::size_t size = little(bytes, at + 4, 4);
			const std::size_t body = at + 8;
			const std::size_t inFile = std::min(size, bytes.size() - body);
			if (tagAt(bytes, at, "fmt "))
			{
				if (inFile < 16) { throw fail("has a format chunk too short to read"); }
				format = little(bytes, body, 2);
				channels = little(bytes, body + 2, 2);
				rate = little(bytes, body + 4, 4);
				bits = little(bytes, body + 14, 2);
				if (format == kExtensible)
				{
					if (inFile < 26) { throw fail("has a format chunk too short to read"); }
					format = little(bytes, body + 24, 2);
				}
				haveFormat = true;
			}
			else if (tagAt(bytes, at, "data"))
			{
				dataAt = body;
				dataSize = inFile;
				haveData = true;
			}
			at = body + size + (size % 2);
		}

		if (!haveFormat) { throw fail("has no format chunk"); }
		if (format != kPcm) { throw fail("is compressed; only uncompressed (PCM) WAV files are read"); }
		if (bits != 8 && bits != 16) { throw fail("has " + std::to_string(bits) + "-bit samples; only 8 or 16 bit are read"); }
		if (channels != 1 && channels != 2) { throw fail("has " + std::to_string(channels) + " channels; only one or two are read"); }
		if (rate == 0) { throw fail("has a sample rate of 0"); }
		if (!haveData) { throw fail("has no data chunk"); }

		const std::size_t width = bits / 8;
		const std::size_t frames = dataSize / (width * channels);
		if (static_cast<double>(frames) / rate > kMaxSoundFileSeconds)
		{
			throw fail("is longer than " + std::to_string(static_cast<int>(kMaxSoundFileSeconds)) + " seconds");
		}

		// One channel at the file's own rate. 8-bit samples are unsigned,
		// 128 the middle; 16-bit ones are signed.
		std::vector<double> mono(frames);
		for (std::size_t frame = 0; frame < frames; ++frame)
		{
			double sum = 0.0;
			for (std::size_t channel = 0; channel < channels; ++channel)
			{
				const std::size_t at = dataAt + (frame * channels + channel) * width;
				if (width == 1) { sum += (static_cast<double>(bytes[at]) - 128.0) * 256.0; }
				else { sum += static_cast<std::int16_t>(static_cast<std::uint16_t>(little(bytes, at, 2))); }
			}
			mono[frame] = sum / channels;
		}

		// Then at the engine's rate: each sample on a straight line between
		// the two of the file it falls between.
		const std::size_t count = rate == kSoundSampleRate
			? frames
			: static_cast<std::size_t>(std::llround(static_cast<double>(frames) * kSoundSampleRate / rate));
		std::vector<std::int16_t> samples(count);
		for (std::size_t i = 0; i < count; ++i)
		{
			const double where = static_cast<double>(i) * rate / kSoundSampleRate;
			const std::size_t before = std::min(static_cast<std::size_t>(where), frames - 1);
			const std::size_t after = std::min(before + 1, frames - 1);
			const double share = where - static_cast<double>(before);
			const double value = mono[before] + (mono[after] - mono[before]) * share;
			samples[i] = static_cast<std::int16_t>(std::lround(std::clamp(value, -32768.0, 32767.0)));
		}
		return samples;
	}
}
