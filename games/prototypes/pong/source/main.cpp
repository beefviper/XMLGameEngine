// main.cpp
// Pong
// author: beefviper
// date: Nov 13, 2026
//
// The window: keys in, a frame at 60 a second, the picture and the sounds
// out.

#include "pong.h"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
	using namespace pong;

	const sf::Color lightGrey{ 192, 192, 192 };

	// A square wave, as the arcade's tones were, faded in and out over two
	// thousandths of a second so it doesn't click.
	sf::SoundBuffer tone(double hertz, double seconds)
	{
		constexpr unsigned rate = 44100;
		constexpr double volume = 0.3;
		const std::size_t count = static_cast<std::size_t>(seconds * rate);
		const std::size_t fade = std::min<std::size_t>(count / 4, rate / 500);

		std::vector<std::int16_t> samples(count);
		for (std::size_t i = 0; i < count; ++i)
		{
			const double phase = std::fmod(static_cast<double>(i) * hertz / rate, 1.0);
			double level = phase < 0.5 ? volume : -volume;
			if (i < fade) { level *= static_cast<double>(i) / static_cast<double>(fade); }
			if (i >= count - fade) { level *= static_cast<double>(count - 1 - i) / static_cast<double>(fade); }
			samples[i] = static_cast<std::int16_t>(std::lround(level * 32767));
		}

		sf::SoundBuffer buffer;
		if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, rate, { sf::SoundChannel::Mono }))
		{
			std::cerr << "warning: could not make a sound\n";
		}
		return buffer;
	}

	class Picture
	{
	public:
		Picture(sf::RenderWindow& into, const sf::Font& with) : window(into), font(with) {}

		void box(sf::Vector2f position, sf::Vector2f size, sf::Color color = sf::Color::White)
		{
			sf::RectangleShape shape(size);
			shape.setPosition(position);
			shape.setFillColor(color);
			window.draw(shape);
		}

		// Text whose letters' top left is at `position`.
		void text(const std::string& words, unsigned size, sf::Color color, sf::Vector2f position)
		{
			sf::Text shown(font, words, size);
			shown.setFillColor(color);
			shown.setPosition(position - shown.getLocalBounds().position);
			window.draw(shown);
		}

		// Text across the middle of the window, `below` the middle (or
		// `down` from the top).
		void centred(const std::string& words, unsigned size, sf::Color color, float below)
		{
			const sf::Vector2f measured = sf::Text(font, words, size).getLocalBounds().size;
			text(words, size, color, { windowWidth / 2 - measured.x / 2, windowHeight / 2 - measured.y / 2 + below });
		}
		void centredAt(const std::string& words, unsigned size, sf::Color color, float down)
		{
			const float across = sf::Text(font, words, size).getLocalBounds().size.x;
			text(words, size, color, { windowWidth / 2 - across / 2, down });
		}

		float widthOf(const std::string& words, unsigned size) const
		{
			return sf::Text(font, words, size).getLocalBounds().size.x;
		}

	private:
		sf::RenderWindow& window;
		const sf::Font& font;
	};

	// The net, the paddles, the ball and the scores either side of the net.
	void drawGame(Picture& picture, const Pong& game)
	{
		for (int dash = 0; dash < 18; ++dash)
		{
			picture.box({ windowWidth / 2 - 3, 10 + 40.0f * static_cast<float>(dash) }, { 6, 20 }, lightGrey);
		}

		picture.box(game.left.position, { paddleWidth, paddleHeight });
		picture.box(game.right.position, { paddleWidth, paddleHeight });
		picture.box(game.ball.position, { ballSize, ballSize });

		const std::string left = std::to_string(game.left.score);
		picture.text(left, 128, sf::Color::White, { windowWidth / 2 - 80 - picture.widthOf(left, 128), margin });
		picture.text(std::to_string(game.right.score), 128, sf::Color::White, { windowWidth / 2 + 80, margin });
	}

	void draw(Picture& picture, const Pong& game)
	{
		switch (game.screen())
		{
		case Screen::Title:
			picture.centredAt("PONG", 160, sf::Color::White, margin * 4);
			picture.centred("spacebar to play the computer, 2 for two players", 48, lightGrey, 120);
			picture.centred(game.computerIsSmart() ? "a d: the computer plays smart" : "a d: the computer plays easy", 48, lightGrey, 190);
			break;
		case Screen::Playing:
			drawGame(picture, game);
			break;
		case Screen::Paused:
			drawGame(picture, game);
			picture.centred("PAUSED", 160, sf::Color::White, 0);
			break;
		case Screen::Over:
			drawGame(picture, game);
			picture.centred("GAME OVER", 160, sf::Color::White, 0);
			picture.centred("spacebar to play again", 48, lightGrey, 120);
			break;
		}
	}
}

int main(int, char* argv[])
{
	sf::RenderWindow window(sf::VideoMode({ static_cast<unsigned>(windowWidth), static_cast<unsigned>(windowHeight) }), "Pong");
	window.setFramerateLimit(60);
	window.setKeyRepeatEnabled(false);

	// The font sits beside the program.
	sf::Font font;
	const std::filesystem::path beside = std::filesystem::path(argv[0]).parent_path() / "tuffy.ttf";
	if (!font.openFromFile(beside) && !font.openFromFile("tuffy.ttf"))
	{
		std::cerr << "error: tuffy.ttf is missing\n";
		return 1;
	}

	// a paddle hit, the wall an octave lower, and a long one for a point
	std::map<std::string, sf::SoundBuffer> tones{
		{ "paddle", tone(459, 0.1) },
		{ "wall", tone(226, 0.02) },
		{ "score", tone(490, 0.26) },
	};
	std::map<std::string, sf::Sound> sounds;
	for (const auto& [name, buffer] : tones) { sounds.emplace(name, buffer); }

	Pong game;
	game.keepChoicesIn("pong-kept.txt");
	Picture picture(window, font);

	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>()) { window.close(); }
			if (const auto* key = event->getIf<sf::Event::KeyPressed>()) { game.key(key->code, true); }
			if (const auto* key = event->getIf<sf::Event::KeyReleased>()) { game.key(key->code, false); }

			// a game pad's Start pauses too
			if (const auto* button = event->getIf<sf::Event::JoystickButtonPressed>(); button && button->button == 7)
			{
				game.key(sf::Keyboard::Key::Pause, true);
			}
		}

		game.frame();
		for (const std::string& name : game.takeSounds()) { sounds.at(name).play(); }

		window.clear(sf::Color::Black);
		draw(picture, game);
		window.display();
	}
}
