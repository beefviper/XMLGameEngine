// main.cpp
// Breakout
// author: beefviper
// date: Nov 24, 2026
//
// The window: keys and the mouse in, a step of the time since the last frame, the picture and the sounds out.

#include "breakout.h"

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
	using namespace breakout;

	const sf::Color orange{ 255, 165, 0 };
	const sf::Color lightgrey{ 192, 192, 192 };

	struct Style
	{
		unsigned size;
		sf::Color color;
	};

	const Style banner{ 100, orange };
	const Style prompt{ 36, sf::Color::White };
	const Style help{ 26, lightgrey };
	const Style hud{ 64, sf::Color::White };

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

	// a brick's colour by its look
	const std::array<sf::Color, 4> brickColors{ sf::Color::Red, orange, sf::Color::Green, sf::Color::Yellow };

	void drawTitle(Picture& picture)
	{
		const sf::Vector2f titleSize = picture.measure("BREAKOUT", banner);
		picture.text("BREAKOUT", banner, { windowWidth / 2 - titleSize.x / 2, windowHeight / 2 - titleSize.y / 2 - 110 });
		const sf::Vector2f help1Size = picture.measure("three balls, two walls: yellow 1, green 3, orange 5, red 7", help);
		picture.text("three balls, two walls: yellow 1, green 3, orange 5, red 7", help, { windowWidth / 2 - help1Size.x / 2, windowHeight / 2 - help1Size.y / 2 - 10 });
		const sf::Vector2f help2Size = picture.measure("a and d, the arrows or the mouse move the paddle", help);
		picture.text("a and d, the arrows or the mouse move the paddle", help, { windowWidth / 2 - help2Size.x / 2, windowHeight / 2 - help1Size.y / 2 - 10 + 40 });
		const sf::Vector2f startSize = picture.measure("1 to start", prompt);
		picture.text("1 to start", prompt, { windowWidth / 2 - startSize.x / 2, windowHeight / 2 - startSize.y / 2 + 120 });
	}

	void drawPlaying(Picture& picture, const Breakout& game)
	{
		picture.text(std::to_string(game.status.score), hud, { 40, 16 });
		const std::string ballsText = std::to_string(game.status.balls);
		const sf::Vector2f ballsSize = picture.measure(ballsText, hud);
		picture.text(ballsText, hud, { windowWidth - ballsSize.x - 40, 16 });
		for (const Brick& cell : game.wall) { picture.box(cell.position, cell.size, brickColors[static_cast<std::size_t>(cell.look)]); }
		picture.box(game.player.position, game.player.size, sf::Color::Cyan);
		picture.box(game.ball.position, game.ball.size);
	}

	void drawPaused(Picture& picture)
	{
		const sf::Vector2f pausedSize = picture.measure("PAUSED", { banner.size, sf::Color::White });
		picture.text("PAUSED", { banner.size, sf::Color::White }, { windowWidth / 2 - pausedSize.x / 2, windowHeight / 2 - pausedSize.y / 2 });
	}

	void drawOver(Picture& picture, const Breakout& game)
	{
		const Style messageStyle = game.message.look == Message::Look::Lost ? Style{ banner.size, sf::Color::Red } : Style{ banner.size, sf::Color::Green };
		const std::string messageText = game.message.look == Message::Look::Lost ? "GAME OVER" : "YOU WIN";
		const sf::Vector2f messageSize = picture.measure(messageText, messageStyle);
		picture.text(messageText, messageStyle, { windowWidth / 2 - messageSize.x / 2, windowHeight / 2 - messageSize.y / 2 - 60 });
		const sf::Vector2f againSize = picture.measure("spacebar to play again", prompt);
		picture.text("spacebar to play again", prompt, { windowWidth / 2 - againSize.x / 2, windowHeight / 2 - againSize.y / 2 + 70 });
	}

	// The screen on top, over those it shows still beneath it.
	void draw(Picture& picture, const Breakout& game)
	{
		switch (game.screen())
		{
		case Screen::Title: drawTitle(picture); break;
		case Screen::Playing: drawPlaying(picture, game); break;
		case Screen::Paused: drawPlaying(picture, game); drawPaused(picture); break;
		case Screen::Over: drawPlaying(picture, game); drawOver(picture, game); break;
		}
	}

	enum class Wave { Square, Triangle };

	// How high the wave is, -1 to 1, at `phase` (0 to 1) through one cycle.
	double height(Wave wave, double phase)
	{
		if (wave == Wave::Triangle) { return phase < 0.25 ? 4 * phase : phase < 0.75 ? 2 - 4 * phase : 4 * phase - 4; }
		return phase < 0.5 ? 1.0 : -1.0;
	}

	// A wave sliding evenly in pitch from `hertz` to `toHertz`, faded in and
	// out over two thousandths of a second so it doesn't click.
	sf::SoundBuffer tone(Wave wave, double hertz, double toHertz, double seconds, double volume)
	{
		constexpr unsigned rate = 44100;
		const std::size_t count = static_cast<std::size_t>(seconds * rate);
		const std::size_t fade = std::min<std::size_t>(count / 4, rate / 500);

		std::vector<std::int16_t> samples(count);
		double phase = 0.0;
		for (std::size_t i = 0; i < count; ++i)
		{
			double level = height(wave, phase) * volume;
			if (i < fade) { level *= static_cast<double>(i) / static_cast<double>(fade); }
			if (i >= count - fade) { level *= static_cast<double>(count - 1 - i) / static_cast<double>(fade); }
			samples[i] = static_cast<std::int16_t>(std::lround(level * 32767));

			const double through = static_cast<double>(i) / static_cast<double>(count);
			phase += hertz * std::pow(toHertz / hertz, through) / rate;
			phase -= std::floor(phase);
		}

		sf::SoundBuffer buffer;
		if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, rate, { sf::SoundChannel::Mono }))
		{
			std::cerr << "warning: could not make a sound\n";
		}
		return buffer;
	}

	// A plain tone: a square wave at the usual loudness.
	sf::SoundBuffer tone(double hertz, double seconds)
	{
		return tone(Wave::Square, hertz, hertz, seconds, 0.3);
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
	sf::RenderWindow window(sf::VideoMode({ static_cast<unsigned>(windowWidth), static_cast<unsigned>(windowHeight) }), "Breakout");
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
		{ Sound::Wall, tone(220, 0.03) },
		{ Sound::Paddle, tone(440, 0.04) },
		{ Sound::Red, tone(1046.5, 0.05) },
		{ Sound::Orange, tone(783.99, 0.05) },
		{ Sound::Green, tone(523.25, 0.05) },
		{ Sound::Yellow, tone(392, 0.05) },
		{ Sound::Lost, tone(Wave::Triangle, 329.63, 82.41, 0.4, 0.4) },
	};
	std::map<Sound, sf::Sound> sounds;
	for (const auto& [sound, buffer] : tones) { sounds.emplace(sound, buffer); }

	Breakout game;
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
			if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) { game.mouse(window.mapPixelToCoords(moved->position).x); }
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
}
