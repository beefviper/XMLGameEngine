// trace.cpp
// XML Game Engine
// author: beefviper
// date: Oct 10, 2026

#include "trace.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace xge::trace
{
	namespace
	{
		std::vector<std::string> split(const std::string& text, char separator)
		{
			std::vector<std::string> parts;
			std::string part;
			std::istringstream stream(text);
			while (std::getline(stream, part, separator))
			{
				parts.push_back(part);
			}
			return parts;
		}

		int wholeNumber(const std::string& text, const std::string& what)
		{
			if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos || text.size() > 9)
			{
				throw std::invalid_argument(what + " '" + text + "' is not a whole number");
			}
			return std::stoi(text);
		}

		void put16(std::ostream& out, unsigned value)
		{
			out.put(static_cast<char>(value & 0xff));
			out.put(static_cast<char>((value >> 8) & 0xff));
		}

		void put32(std::ostream& out, unsigned value)
		{
			put16(out, value & 0xffff);
			put16(out, value >> 16);
		}

		unsigned get32(const std::vector<std::uint8_t>& bytes, std::size_t at)
		{
			return static_cast<unsigned>(bytes[at]) | (static_cast<unsigned>(bytes[at + 1]) << 8)
				| (static_cast<unsigned>(bytes[at + 2]) << 16) | (static_cast<unsigned>(bytes[at + 3]) << 24);
		}

		constexpr std::size_t headerSize = 54;

		std::size_t rowSize(int width)
		{
			return (static_cast<std::size_t>(width) * 3 + 3) / 4 * 4;
		}
	}

	std::vector<int> parseFrames(const std::string& text)
	{
		std::vector<int> frames;
		for (const std::string& part : split(text, ','))
		{
			const int frame = wholeNumber(part, "the frame");
			if (frame < 1)
			{
				throw std::invalid_argument("frames are counted from 1, not '" + part + "'");
			}
			frames.push_back(frame);
		}

		if (frames.empty())
		{
			throw std::invalid_argument("no frames given");
		}

		std::sort(frames.begin(), frames.end());
		frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
		return frames;
	}

	std::vector<KeyEvent> parseKeys(const std::string& text)
	{
		std::vector<KeyEvent> events;
		for (const std::string& part : split(text, ','))
		{
			const std::size_t sign = part.find_first_of("+-");
			if (sign == std::string::npos || sign == 0 || sign + 1 >= part.size())
			{
				throw std::invalid_argument("'" + part + "' is not a frame, + or -, and a key (like 5+space)");
			}

			KeyEvent event;
			event.frame = wholeNumber(part.substr(0, sign), "the frame");
			event.down = part[sign] == '+';
			event.key = keyCodeFromString(part.substr(sign + 1));
			if (event.frame < 1)
			{
				throw std::invalid_argument("frames are counted from 1, not '" + part + "'");
			}
			if (event.key == KeyCode::Unknown)
			{
				throw std::invalid_argument("'" + part.substr(sign + 1) + "' is not a key");
			}
			events.push_back(event);
		}

		std::stable_sort(events.begin(), events.end(), [](const KeyEvent& a, const KeyEvent& b) { return a.frame < b.frame; });
		return events;
	}

	std::string frameFile(int frame)
	{
		std::ostringstream name;
		name << "frame" << std::setfill('0') << std::setw(4) << frame << ".bmp";
		return name.str();
	}

	void writeBitmap(const std::filesystem::path& file, const Bitmap& picture)
	{
		std::ofstream out(file, std::ios::binary);
		if (!out)
		{
			throw std::runtime_error("could not write " + file.string());
		}

		const std::size_t row = rowSize(picture.width);
		const auto pixelBytes = static_cast<unsigned>(row * static_cast<std::size_t>(picture.height));

		out.write("BM", 2);
		put32(out, static_cast<unsigned>(headerSize) + pixelBytes);
		put32(out, 0);
		put32(out, static_cast<unsigned>(headerSize));
		put32(out, 40);
		put32(out, static_cast<unsigned>(picture.width));
		put32(out, static_cast<unsigned>(picture.height));
		put16(out, 1);
		put16(out, 24);
		put32(out, 0);
		put32(out, pixelBytes);
		put32(out, 2835);
		put32(out, 2835);
		put32(out, 0);
		put32(out, 0);

		std::vector<char> line(row, 0);
		for (int y = picture.height - 1; y >= 0; --y)
		{
			for (int x = 0; x < picture.width; ++x)
			{
				const std::size_t from = (static_cast<std::size_t>(y) * static_cast<std::size_t>(picture.width) + static_cast<std::size_t>(x)) * 4;
				const std::size_t to = static_cast<std::size_t>(x) * 3;
				line[to] = static_cast<char>(picture.rgba[from + 2]);
				line[to + 1] = static_cast<char>(picture.rgba[from + 1]);
				line[to + 2] = static_cast<char>(picture.rgba[from]);
			}
			out.write(line.data(), static_cast<std::streamsize>(line.size()));
		}

		if (!out)
		{
			throw std::runtime_error("could not write " + file.string());
		}
	}

	Bitmap readBitmap(const std::filesystem::path& file)
	{
		std::ifstream in(file, std::ios::binary);
		if (!in)
		{
			throw std::runtime_error("could not read " + file.string());
		}

		const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		if (bytes.size() < headerSize || bytes[0] != 'B' || bytes[1] != 'M' || get32(bytes, 10) != headerSize || bytes[28] != 24)
		{
			throw std::runtime_error(file.string() + " is not a 24 bit BMP of the kind the trace writes");
		}

		Bitmap picture;
		picture.width = static_cast<int>(get32(bytes, 18));
		picture.height = static_cast<int>(get32(bytes, 22));

		const std::size_t row = rowSize(picture.width);
		if (picture.width < 1 || picture.height < 1 || bytes.size() < headerSize + row * static_cast<std::size_t>(picture.height))
		{
			throw std::runtime_error(file.string() + " is cut short");
		}

		picture.rgba.resize(static_cast<std::size_t>(picture.width) * static_cast<std::size_t>(picture.height) * 4);
		for (int y = 0; y < picture.height; ++y)
		{
			const std::size_t from = headerSize + row * static_cast<std::size_t>(picture.height - 1 - y);
			for (int x = 0; x < picture.width; ++x)
			{
				const std::size_t source = from + static_cast<std::size_t>(x) * 3;
				const std::size_t to = (static_cast<std::size_t>(y) * static_cast<std::size_t>(picture.width) + static_cast<std::size_t>(x)) * 4;
				picture.rgba[to] = bytes[source + 2];
				picture.rgba[to + 1] = bytes[source + 1];
				picture.rgba[to + 2] = bytes[source];
				picture.rgba[to + 3] = 255;
			}
		}
		return picture;
	}

	Difference compare(const Bitmap& a, const Bitmap& b, int tolerance)
	{
		Difference difference;
		if (a.width != b.width || a.height != b.height)
		{
			difference.pixels = std::max(a.rgba.size(), b.rgba.size()) / 4;
			difference.largest = 255;
			return difference;
		}

		// How far apart the colours of a's pixel and b's are.
		const auto apart = [&](int ax, int ay, int bx, int by)
		{
			const std::size_t from = (static_cast<std::size_t>(ay) * static_cast<std::size_t>(a.width) + static_cast<std::size_t>(ax)) * 4;
			const std::size_t to = (static_cast<std::size_t>(by) * static_cast<std::size_t>(b.width) + static_cast<std::size_t>(bx)) * 4;
			int worst = 0;
			for (std::size_t colour = 0; colour < 3; ++colour)
			{
				worst = std::max(worst, std::abs(static_cast<int>(a.rgba[from + colour]) - static_cast<int>(b.rgba[to + colour])));
			}
			return worst;
		};

		// A pixel that is not the colour of the other picture's but is the colour of
		// one next to it there (or the other way round) is an edge a little out of
		// place, which is no difference: a shape between two pixels is drawn on one
		// or the other. Something moved by more than a pixel still shows.
		const auto matchesNeighbour = [&](int x, int y)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				for (int dx = -1; dx <= 1; ++dx)
				{
					const int nx = x + dx;
					const int ny = y + dy;
					if (nx < 0 || ny < 0 || nx >= a.width || ny >= a.height) { continue; }
					if (apart(x, y, nx, ny) <= tolerance || apart(nx, ny, x, y) <= tolerance) { return true; }
				}
			}
			return false;
		};

		for (int y = 0; y < a.height; ++y)
		{
			for (int x = 0; x < a.width; ++x)
			{
				const int worst = apart(x, y, x, y);
				difference.largest = std::max(difference.largest, worst);
				if (worst > tolerance && !matchesNeighbour(x, y))
				{
					++difference.pixels;
				}
			}
		}
		return difference;
	}

	std::vector<Place> placesOf(Game& game)
	{
		std::vector<Place> places;
		for (const Object& object : game.getCurrentObjects())
		{
			if (game.isShown(object) && object.isVisible)
			{
				places.push_back({ object.name, object.position.x, object.position.y });
			}
		}
		return places;
	}

	std::string stateFile(int frame)
	{
		std::ostringstream name;
		name << "state" << std::setfill('0') << std::setw(4) << frame << ".txt";
		return name.str();
	}

	void writePlaces(const std::filesystem::path& file, const std::vector<Place>& places)
	{
		std::ofstream out(file, std::ios::binary);
		for (const Place& place : places)
		{
			out << place.name << ' ' << std::setprecision(9) << place.x << ' ' << place.y << '\n';
		}
		if (!out)
		{
			throw std::runtime_error("could not write " + file.string());
		}
	}

	std::vector<Place> readPlaces(const std::filesystem::path& file)
	{
		std::ifstream in(file);
		if (!in)
		{
			throw std::runtime_error("could not read " + file.string());
		}

		std::vector<Place> places;
		std::string line;
		while (std::getline(in, line))
		{
			std::istringstream words(line);
			Place place;
			if (words >> place.name >> place.x >> place.y)
			{
				places.push_back(place);
			}
		}
		return places;
	}

	std::string comparePlaces(const std::vector<Place>& engine, const std::vector<Place>& program, float within)
	{
		std::ostringstream said;
		std::size_t different = 0;
		std::string first;

		const std::size_t both = std::min(engine.size(), program.size());
		for (std::size_t at = 0; at < both; ++at)
		{
			const bool apart = std::abs(engine[at].x - program[at].x) > within || std::abs(engine[at].y - program[at].y) > within;
			if (!apart)
			{
				continue;
			}

			if (different == 0)
			{
				std::ostringstream one;
				one << engine[at].name << " is at " << engine[at].x << ", " << engine[at].y << " in the engine and "
					<< program[at].x << ", " << program[at].y << " in the program (thing " << at + 1 << ", which it calls " << program[at].name << ")";
				first = one.str();
			}
			++different;
		}

		if (engine.size() != program.size())
		{
			said << "the engine has " << engine.size() << " shown objects in play and the program " << program.size();
			if (different > 0) { said << "; "; }
		}
		if (different > 0)
		{
			said << different << " of them are further apart than " << within << ": the first, " << first;
		}
		return said.str();
	}

	void SoundLog::play(const std::string& name)
	{
		std::ostringstream line;
		line << "frame " << std::setfill('0') << std::setw(4) << frame << ' ' << name;
		heard.push_back(line.str());
	}

	std::vector<std::string> SoundLog::lines() const
	{
		std::vector<std::string> sorted = heard;
		std::sort(sorted.begin(), sorted.end());
		return sorted;
	}

	namespace
	{
		std::vector<std::string> readLines(const std::filesystem::path& file, bool& found)
		{
			std::vector<std::string> lines;
			std::ifstream in(file);
			found = static_cast<bool>(in);

			std::string line;
			while (std::getline(in, line))
			{
				lines.push_back(line);
			}
			return lines;
		}

		// The same sounds, each at most `slack` frames from the other's, in the
		// same order for each name.
		bool matchWithin(const std::vector<std::string>& a, const std::vector<std::string>& b, int slack)
		{
			if (a.size() != b.size())
			{
				return false;
			}

			// "frame 0012 wall": by name, keeping their order in time
			const auto byName = [](const std::vector<std::string>& lines)
			{
				std::map<std::string, std::vector<int>> frames;
				for (const std::string& line : lines)
				{
					frames[line.substr(11)].push_back(std::stoi(line.substr(6, 4)));
				}
				return frames;
			};

			const auto left = byName(a);
			const auto right = byName(b);
			if (left.size() != right.size())
			{
				return false;
			}

			for (const auto& [name, frames] : left)
			{
				const auto other = right.find(name);
				if (other == right.end() || other->second.size() != frames.size())
				{
					return false;
				}
				for (std::size_t at = 0; at < frames.size(); ++at)
				{
					if (std::abs(frames[at] - other->second[at]) > slack)
					{
						return false;
					}
				}
			}
			return true;
		}

		// Keeps the sounds heard as sounds.txt and compares them with the other
		// program's, saying so on `out`; 1 if they are not the same list.
		int compareSounds(const SoundLog& log, const Request& request, std::ostream& out)
		{
			const std::vector<std::string> mine = log.lines();

			if (!request.write.empty())
			{
				std::ofstream file(request.write / "sounds.txt", std::ios::binary);
				for (const std::string& line : mine) { file << line << '\n'; }
			}

			if (request.against.empty())
			{
				out << "sounds: " << mine.size() << " heard\n";
				return 0;
			}

			bool found = false;
			const std::vector<std::string> theirs = readLines(request.against / "sounds.txt", found);
			if (!found)
			{
				out << "sounds: DIFFERENT, " << (request.against / "sounds.txt").string() << " was not written\n";
				return 1;
			}

			if (matchWithin(mine, theirs, request.soundSlack))
			{
				out << "sounds: same (" << mine.size() << " heard)\n";
				return 0;
			}

			std::size_t at = 0;
			while (at < mine.size() && at < theirs.size() && mine[at] == theirs[at]) { ++at; }

			out << "sounds: DIFFERENT, the engine heard " << mine.size() << " and the program " << theirs.size() << "; first difference: the engine "
				<< (at < mine.size() ? "'" + mine[at] + "'" : std::string("nothing more")) << ", the program "
				<< (at < theirs.size() ? "'" + theirs[at] + "'" : std::string("nothing more")) << '\n';
			return 1;
		}
	}

	int run(Engine& engine, const Request& request, std::ostream& out)
	{
		if (request.frames.empty())
		{
			throw std::invalid_argument("no frames to keep");
		}

		Window* window = engine.currentWindow();
		if (!window)
		{
			throw std::logic_error("the engine has no window to play in");
		}

		if (!request.write.empty())
		{
			std::filesystem::create_directories(request.write);
		}

		std::size_t nextKey = 0;
		std::size_t nextFrame = 0;
		int mismatches = 0;

		for (int frame = 1; nextFrame < request.frames.size(); ++frame)
		{
			if (request.sounds) { request.sounds->frame = frame; }

			while (nextKey < request.keys.size() && request.keys[nextKey].frame == frame)
			{
				const KeyEvent& event = request.keys[nextKey++];
				if (event.down) { engine.handleKeyPressed(event.key); }
				else { engine.handleKeyReleased(event.key); }
			}

			engine.step();
			engine.render();

			if (request.frames[nextFrame] != frame)
			{
				continue;
			}
			++nextFrame;

			const Bitmap picture = window->capture();
			const std::string file = frameFile(frame);

			const std::vector<Place> places = placesOf(engine.currentGame());

			if (!request.write.empty())
			{
				writeBitmap(request.write / file, picture);
				writePlaces(request.write / stateFile(frame), places);
			}

			if (request.against.empty())
			{
				out << "frame " << frame << ": kept\n";
				continue;
			}

			try
			{
				const Difference difference = compare(picture, readBitmap(request.against / file), request.tolerance);

				// Where each thing is, which a picture can hide (under another, off the
				// screen), when the program wrote it.
				std::string apart;
				if (std::filesystem::exists(request.against / stateFile(frame)))
				{
					apart = comparePlaces(places, readPlaces(request.against / stateFile(frame)), 0.05f);
				}

				// Where the engine has everything, to read beside the program's.
				std::filesystem::path theirs = request.against / stateFile(frame);
				theirs.replace_extension(".engine.txt");
				writePlaces(theirs, places);

				if (difference.pixels <= request.allow && apart.empty())
				{
					out << "frame " << frame << ": same";
					if (difference.pixels > 0) { out << " (" << difference.pixels << " pixels differ a little)"; }
					out << '\n';
					continue;
				}

				if (difference.pixels <= request.allow)
				{
					out << "frame " << frame << ": DIFFERENT, though the pictures are the same: " << apart << '\n';
				}
				else
				{
					std::filesystem::path mine = request.against / file;
					mine.replace_extension(".engine.bmp");
					writeBitmap(mine, picture);
					out << "frame " << frame << ": DIFFERENT, " << difference.pixels << " pixels, by up to " << difference.largest
						<< " (the engine's is " << mine.string() << ")";
					if (!apart.empty()) { out << "; " << apart; }
					out << '\n';
				}
			}
			catch (const std::runtime_error& error)
			{
				out << "frame " << frame << ": DIFFERENT, " << error.what() << '\n';
			}
			++mismatches;
		}

		if (request.sounds)
		{
			mismatches += compareSounds(*request.sounds, request, out);
		}

		return mismatches;
	}
}
