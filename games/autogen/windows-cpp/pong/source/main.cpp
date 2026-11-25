// main.cpp
// Pong
// author: beefviper
// date: Nov 24, 2026
//
// The window: keys in, a step of the time since the last frame, the picture and the sounds out.

#include "pong.h"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace
{
	using namespace pong;

	const sf::Color lightgrey{ 192, 192, 192 };

	struct Style
	{
		unsigned size;
		sf::Color color;
	};

	const Style banner{ 160, sf::Color::White };
	const Style prompt{ 48, lightgrey };
	const Style hud{ 128, sf::Color::White };

	class Picture
	{
	public:
		Picture(sf::RenderWindow& into, const sf::Font& with) : window(into), font(with) {}

		void box(sf::Vector2f position, sf::Vector2f extent, sf::Color color = sf::Color::White)
		{
			sf::RectangleShape shape(extent);
			shape.setPosition(position);
			shape.setFillColor(color);
			window.draw(shape);
		}

		// How big these words are drawn: the box their letters cover.
		sf::Vector2f measure(const std::string& words, const Style& style) const
		{
			return sf::Text(font, words, style.size).getLocalBounds().size;
		}

		// Words with their letters' top left at `position`.
		void text(const std::string& words, const Style& style, sf::Vector2f position)
		{
			sf::Text shown(font, words, style.size);
			shown.setFillColor(style.color);
			shown.setPosition(position - shown.getLocalBounds().position);
			window.draw(shown);
		}

	private:
		sf::RenderWindow& window;
		const sf::Font& font;
	};

	void drawTitle(Picture& picture, const Pong& game)
	{
		const sf::Vector2f titleSize = picture.measure("PONG", banner);
		picture.text("PONG", banner, { windowWidth / 2 - titleSize.x / 2, margin * 4 });
		const sf::Vector2f startSize = picture.measure("1 player against the computer, 2 for two players", prompt);
		picture.text("1 player against the computer, 2 for two players", prompt, { windowWidth / 2 - startSize.x / 2, windowHeight / 2 - startSize.y / 2 + 120 });
		const std::string computerText = game.computer.look == Computer::Look::Smart ? "a d: the computer plays smart" : "a d: the computer plays easy";
		const sf::Vector2f computerSize = picture.measure(computerText, prompt);
		picture.text(computerText, prompt, { windowWidth / 2 - computerSize.x / 2, windowHeight / 2 - computerSize.y / 2 + 190 });
	}

	void drawPlaying(Picture& picture, const Pong& game)
	{
		for (int row = 0; row < 18; ++row)
		{
			picture.box({ windowWidth / 2 - 3, 10 + 40 * static_cast<float>(row) }, { 6, 20 }, lightgrey);
		}
		picture.box(game.paddle1.position, game.paddle1.size);
		picture.box(game.paddle2.position, game.paddle2.size);
		const std::string score1Text = std::to_string(game.paddle1.score);
		const sf::Vector2f score1Size = picture.measure(score1Text, hud);
		picture.text(score1Text, hud, { windowWidth / 2 - 80 - score1Size.x, margin });
		picture.text(std::to_string(game.paddle2.score), hud, { windowWidth / 2 + 80, margin });
		picture.box(game.ball.position, game.ball.size);
	}

	void drawPaused(Picture& picture)
	{
		const sf::Vector2f pausedSize = picture.measure("PAUSED", banner);
		picture.text("PAUSED", banner, { windowWidth / 2 - pausedSize.x / 2, windowHeight / 2 - pausedSize.y / 2 });
	}

	void drawOver(Picture& picture)
	{
		const sf::Vector2f gameoverSize = picture.measure("GAME OVER", banner);
		picture.text("GAME OVER", banner, { windowWidth / 2 - gameoverSize.x / 2, windowHeight / 2 - gameoverSize.y / 2 });
		const sf::Vector2f continueSize = picture.measure("spacebar to play again", prompt);
		picture.text("spacebar to play again", prompt, { windowWidth / 2 - continueSize.x / 2, windowHeight / 2 - continueSize.y / 2 + 120 });
	}

	// The screen on top, over those it shows still beneath it.
	void draw(Picture& picture, const Pong& game)
	{
		switch (game.screen())
		{
		case Screen::Title: drawTitle(picture, game); break;
		case Screen::Playing: drawPlaying(picture, game); break;
		case Screen::Paused: drawPlaying(picture, game); drawPaused(picture); break;
		case Screen::Over: drawPlaying(picture, game); drawOver(picture); break;
		}
	}

	// A square wave, faded in and out over two thousandths of a second so it
	// doesn't click.
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

	// The pads' buttons as they are now: an Xbox pad's buttons in SFML's
	// order, and its directions by the d-pad or the left stick past halfway.
	std::array<bool, 12> padButtons(unsigned pad)
	{
		std::array<bool, 12> buttons{};
		if (!sf::Joystick::isConnected(pad)) { return buttons; }
		for (unsigned button = 0; button < 8; ++button) { buttons[button] = sf::Joystick::isButtonPressed(pad, button); }
		const auto axis = [pad](sf::Joystick::Axis which) { return sf::Joystick::hasAxis(pad, which) ? sf::Joystick::getAxisPosition(pad, which) : 0.0f; };
		buttons[static_cast<std::size_t>(PadButton::Up)] = axis(sf::Joystick::Axis::PovY) < -50 || axis(sf::Joystick::Axis::Y) < -50;
		buttons[static_cast<std::size_t>(PadButton::Down)] = axis(sf::Joystick::Axis::PovY) > 50 || axis(sf::Joystick::Axis::Y) > 50;
		buttons[static_cast<std::size_t>(PadButton::Left)] = axis(sf::Joystick::Axis::PovX) < -50 || axis(sf::Joystick::Axis::X) < -50;
		buttons[static_cast<std::size_t>(PadButton::Right)] = axis(sf::Joystick::Axis::PovX) > 50 || axis(sf::Joystick::Axis::X) > 50;
		return buttons;
	}
}

int main(int, char* argv[])
{
	sf::RenderWindow window(sf::VideoMode({ static_cast<unsigned>(windowWidth), static_cast<unsigned>(windowHeight) }), "Pong");
	window.setFramerateLimit(60);
	window.setKeyRepeatEnabled(false);

	// The font sits beside the program.
	sf::Font font;
	const std::filesystem::path beside = std::filesystem::path(argv[0]).parent_path() / "assets/tuffy.ttf";
	if (!font.openFromFile(beside) && !font.openFromFile("assets/tuffy.ttf"))
	{
		std::cerr << "error: assets/tuffy.ttf is missing\n";
		return 1;
	}

	const std::map<Sound, sf::SoundBuffer> tones{
		{ Sound::Paddle, tone(459, 0.1) },
		{ Sound::Wall, tone(226, 0.02) },
		{ Sound::Score, tone(490, 0.26) },
	};
	std::map<Sound, sf::Sound> sounds;
	for (const auto& [sound, buffer] : tones) { sounds.emplace(sound, buffer); }

	Pong game;
	game.keepIn("pong-kept.txt");
	Picture picture(window, font);
	std::array<std::array<bool, 12>, 2> pads{};
	sf::Clock clock;
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>()) { window.close(); }
			if (const auto* key = event->getIf<sf::Event::KeyPressed>()) { game.key(key->code, true); }
			if (const auto* key = event->getIf<sf::Event::KeyReleased>()) { game.key(key->code, false); }
		}

		// a pad's button is pressed or let go when it is not as it was
		for (unsigned pad = 0; pad < 2; ++pad)
		{
			const std::array<bool, 12> now = padButtons(pad);
			for (std::size_t button = 0; button < now.size(); ++button)
			{
				if (now[button] != pads[pad][button]) { game.pad(pad + 1, static_cast<PadButton>(button), now[button]); }
			}
			pads[pad] = now;
		}

		// the time since the last frame, at most a quarter of a second
		game.frame(std::min(clock.restart().asSeconds(), 0.25f));
		for (const Sound sound : game.takeSounds()) { sounds.at(sound).play(); }

		window.clear(sf::Color::Black);
		draw(picture, game);
		window.display();
	}
	game.keep();
}
