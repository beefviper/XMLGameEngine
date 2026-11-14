// sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window/sfml.h"

#include "picture/color.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace xge
{
	SFMLWindow::SFMLWindow(const WindowDesc& windowDesc)
	{
		const auto scale = static_cast<unsigned int>(std::max(1, windowDesc.scale));
		const auto width = static_cast<unsigned int>(windowDesc.width);
		const auto height = static_cast<unsigned int>(windowDesc.height);
		const sf::VideoMode videoMode({ width * scale, height * scale });

		const auto windowState = (windowDesc.fullscreen == "true") ? sf::State::Fullscreen : sf::State::Windowed;

		window.create(videoMode, windowDesc.name, sf::Style::Default, windowState);

		if (!window.isOpen())
		{
			throw std::runtime_error("SFML could not make a window");
		}

		// The game's own pixels, each drawn scale window pixels across (the
		// mouse is mapped back through the same view).
		view = sf::View(sf::FloatRect({ 0.0f, 0.0f }, { windowDesc.width, windowDesc.height }));
		fitView();
		setPictureScale(fitNow);
		// The engine sets the game's speed (Engine::loop); the window only waits
		// for the screen's refresh, so a picture is never torn.
		window.setVerticalSyncEnabled(true);
	}

	// The game's picture in the window as it is now: as large as fits with
	// the game's own proportions, centred, the rest left as bars.
	void SFMLWindow::fitView()
	{
		const sf::Vector2u size = window.getSize();
		if (size.x == 0 || size.y == 0) { return; }
		const sf::Vector2f game = view.getSize();
		const float fit = std::min(static_cast<float>(size.x) / game.x, static_cast<float>(size.y) / game.y);
		fitNow = fit;
		const float wide = std::floor(game.x * fit);
		const float high = std::floor(game.y * fit);
		const float left = std::floor((static_cast<float>(size.x) - wide) / 2.0f);
		const float top = std::floor((static_cast<float>(size.y) - high) / 2.0f);
		view.setViewport(sf::FloatRect({ left / static_cast<float>(size.x), top / static_cast<float>(size.y) }, { wide / static_cast<float>(size.x), high / static_cast<float>(size.y) }));
		window.setView(view);
	}

	float SFMLWindow::fit() const
	{
		return fitNow;
	}

	bool SFMLWindow::isOpen() const
	{
		return window.isOpen();
	}

	void SFMLWindow::close()
	{
		window.close();
	}

	std::pair<int, int> SFMLWindow::position() const
	{
		return { window.getPosition().x, window.getPosition().y };
	}

	void SFMLWindow::setPosition(int x, int y)
	{
		window.setPosition(sf::Vector2i(x, y));
	}

	void SFMLWindow::setTitle(const std::string& title)
	{
		window.setTitle(sf::String::fromUtf8(title.begin(), title.end()));
	}

	void SFMLWindow::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			build(object, visuals[object.name]);
		}
	}

	std::vector<std::pair<KeyCode, bool>> SFMLWindow::pollEvents()
	{
		std::vector<std::pair<KeyCode, bool>> events;

		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if (event->is<sf::Event::Resized>())
			{
				fitView();
			}
			else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				events.emplace_back(sfmlKeyToKeyCode(keyPressed->code), true);
			}
			else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
			{
				events.emplace_back(sfmlKeyToKeyCode(keyReleased->code), false);
			}
		}

		return events;
	}

	Controls SFMLWindow::controls()
	{
		Controls controls;
		if (!window.hasFocus())
		{
			return controls;
		}

		const sf::Vector2i pixel = sf::Mouse::getPosition(window);
		const sf::Vector2u size = window.getSize();
		const sf::Vector2f at = window.mapPixelToCoords(pixel);
		controls.pointerInside = pixel.x >= 0 && pixel.y >= 0 && pixel.x < static_cast<int>(size.x) && pixel.y < static_cast<int>(size.y);
		controls.pointerX = at.x;
		controls.pointerY = at.y;
		controls.mouse = { sf::Mouse::isButtonPressed(sf::Mouse::Button::Left), sf::Mouse::isButtonPressed(sf::Mouse::Button::Right),
			sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle) };

		// SFML numbers a pad's buttons as the system does; on an Xbox pad
		// that is A B X Y, the shoulders, Back and Start. The hat is the d-pad
		// (negative is up and left).
		for (unsigned int pad = 0; pad < padCount; ++pad)
		{
			if (!sf::Joystick::isConnected(pad)) { continue; }
			Pad& now = controls.pads[pad];
			now.connected = true;
			const unsigned int buttons = sf::Joystick::getButtonCount(pad);
			for (unsigned int button = 0; button < 8 && button < buttons; ++button)
			{
				now.buttons[button] = sf::Joystick::isButtonPressed(pad, button);
			}
			const float hatX = sf::Joystick::hasAxis(pad, sf::Joystick::Axis::PovX) ? sf::Joystick::getAxisPosition(pad, sf::Joystick::Axis::PovX) : 0.0f;
			const float hatY = sf::Joystick::hasAxis(pad, sf::Joystick::Axis::PovY) ? sf::Joystick::getAxisPosition(pad, sf::Joystick::Axis::PovY) : 0.0f;
			now.buttons[static_cast<std::size_t>(PadButton::Up)] = hatY < -50.0f;
			now.buttons[static_cast<std::size_t>(PadButton::Down)] = hatY > 50.0f;
			now.buttons[static_cast<std::size_t>(PadButton::Left)] = hatX < -50.0f;
			now.buttons[static_cast<std::size_t>(PadButton::Right)] = hatX > 50.0f;
			now.x = sf::Joystick::getAxisPosition(pad, sf::Joystick::Axis::X) / 100.0f;
			now.y = sf::Joystick::getAxisPosition(pad, sf::Joystick::Axis::Y) / 100.0f;
		}

		return controls;
	}

	void SFMLWindow::clear(const std::string& colorName)
	{
		const Color c = colorFromName(colorName);
		window.clear(sf::Color(c.r, c.g, c.b, c.a));
	}

	void SFMLWindow::prepare(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.sprite || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}
	}

	void SFMLWindow::draw(Object& object)
	{
		prepare(object);
		CachedVisual& visual = visuals[object.name];

		// Set every frame: an object moves without its picture changing.
		visual.sprite->setPosition({ object.position.x, object.position.y });
		window.draw(*visual.sprite);
	}

	void SFMLWindow::display()
	{
		window.display();
	}

	KeyCode SFMLWindow::sfmlKeyToKeyCode(sf::Keyboard::Key key) noexcept
	{
		switch (key)
		{
		case sf::Keyboard::Key::A: return KeyCode::A;
		case sf::Keyboard::Key::B: return KeyCode::B;
		case sf::Keyboard::Key::C: return KeyCode::C;
		case sf::Keyboard::Key::D: return KeyCode::D;
		case sf::Keyboard::Key::E: return KeyCode::E;
		case sf::Keyboard::Key::F: return KeyCode::F;
		case sf::Keyboard::Key::G: return KeyCode::G;
		case sf::Keyboard::Key::H: return KeyCode::H;
		case sf::Keyboard::Key::I: return KeyCode::I;
		case sf::Keyboard::Key::J: return KeyCode::J;
		case sf::Keyboard::Key::K: return KeyCode::K;
		case sf::Keyboard::Key::L: return KeyCode::L;
		case sf::Keyboard::Key::M: return KeyCode::M;
		case sf::Keyboard::Key::N: return KeyCode::N;
		case sf::Keyboard::Key::O: return KeyCode::O;
		case sf::Keyboard::Key::P: return KeyCode::P;
		case sf::Keyboard::Key::Q: return KeyCode::Q;
		case sf::Keyboard::Key::R: return KeyCode::R;
		case sf::Keyboard::Key::S: return KeyCode::S;
		case sf::Keyboard::Key::T: return KeyCode::T;
		case sf::Keyboard::Key::U: return KeyCode::U;
		case sf::Keyboard::Key::V: return KeyCode::V;
		case sf::Keyboard::Key::W: return KeyCode::W;
		case sf::Keyboard::Key::X: return KeyCode::X;
		case sf::Keyboard::Key::Y: return KeyCode::Y;
		case sf::Keyboard::Key::Z: return KeyCode::Z;
		case sf::Keyboard::Key::Num0: return KeyCode::Num0;
		case sf::Keyboard::Key::Num1: return KeyCode::Num1;
		case sf::Keyboard::Key::Num2: return KeyCode::Num2;
		case sf::Keyboard::Key::Num3: return KeyCode::Num3;
		case sf::Keyboard::Key::Num4: return KeyCode::Num4;
		case sf::Keyboard::Key::Num5: return KeyCode::Num5;
		case sf::Keyboard::Key::Num6: return KeyCode::Num6;
		case sf::Keyboard::Key::Num7: return KeyCode::Num7;
		case sf::Keyboard::Key::Num8: return KeyCode::Num8;
		case sf::Keyboard::Key::Num9: return KeyCode::Num9;
		case sf::Keyboard::Key::Escape: return KeyCode::Escape;
		case sf::Keyboard::Key::LControl: return KeyCode::LControl;
		case sf::Keyboard::Key::LShift: return KeyCode::LShift;
		case sf::Keyboard::Key::LAlt: return KeyCode::LAlt;
		case sf::Keyboard::Key::LSystem: return KeyCode::LSystem;
		case sf::Keyboard::Key::RControl: return KeyCode::RControl;
		case sf::Keyboard::Key::RShift: return KeyCode::RShift;
		case sf::Keyboard::Key::RAlt: return KeyCode::RAlt;
		case sf::Keyboard::Key::RSystem: return KeyCode::RSystem;
		case sf::Keyboard::Key::Menu: return KeyCode::Menu;
		case sf::Keyboard::Key::LBracket: return KeyCode::LBracket;
		case sf::Keyboard::Key::RBracket: return KeyCode::RBracket;
		case sf::Keyboard::Key::Semicolon: return KeyCode::Semicolon;
		case sf::Keyboard::Key::Comma: return KeyCode::Comma;
		case sf::Keyboard::Key::Period: return KeyCode::Period;
		case sf::Keyboard::Key::Apostrophe: return KeyCode::Apostrophe;
		case sf::Keyboard::Key::Slash: return KeyCode::Slash;
		case sf::Keyboard::Key::Backslash: return KeyCode::Backslash;
		case sf::Keyboard::Key::Grave: return KeyCode::Grave;
		case sf::Keyboard::Key::Equal: return KeyCode::Equal;
		case sf::Keyboard::Key::Hyphen: return KeyCode::Hyphen;
		case sf::Keyboard::Key::Space: return KeyCode::Space;
		case sf::Keyboard::Key::Enter: return KeyCode::Enter;
		case sf::Keyboard::Key::Backspace: return KeyCode::Backspace;
		case sf::Keyboard::Key::Tab: return KeyCode::Tab;
		case sf::Keyboard::Key::PageUp: return KeyCode::PageUp;
		case sf::Keyboard::Key::PageDown: return KeyCode::PageDown;
		case sf::Keyboard::Key::End: return KeyCode::End;
		case sf::Keyboard::Key::Home: return KeyCode::Home;
		case sf::Keyboard::Key::Insert: return KeyCode::Insert;
		case sf::Keyboard::Key::Delete: return KeyCode::Delete;
		case sf::Keyboard::Key::Add: return KeyCode::Add;
		case sf::Keyboard::Key::Subtract: return KeyCode::Subtract;
		case sf::Keyboard::Key::Multiply: return KeyCode::Multiply;
		case sf::Keyboard::Key::Divide: return KeyCode::Divide;
		case sf::Keyboard::Key::Left: return KeyCode::Left;
		case sf::Keyboard::Key::Right: return KeyCode::Right;
		case sf::Keyboard::Key::Up: return KeyCode::Up;
		case sf::Keyboard::Key::Down: return KeyCode::Down;
		case sf::Keyboard::Key::Numpad0: return KeyCode::Numpad0;
		case sf::Keyboard::Key::Numpad1: return KeyCode::Numpad1;
		case sf::Keyboard::Key::Numpad2: return KeyCode::Numpad2;
		case sf::Keyboard::Key::Numpad3: return KeyCode::Numpad3;
		case sf::Keyboard::Key::Numpad4: return KeyCode::Numpad4;
		case sf::Keyboard::Key::Numpad5: return KeyCode::Numpad5;
		case sf::Keyboard::Key::Numpad6: return KeyCode::Numpad6;
		case sf::Keyboard::Key::Numpad7: return KeyCode::Numpad7;
		case sf::Keyboard::Key::Numpad8: return KeyCode::Numpad8;
		case sf::Keyboard::Key::Numpad9: return KeyCode::Numpad9;
		case sf::Keyboard::Key::F1: return KeyCode::F1;
		case sf::Keyboard::Key::F2: return KeyCode::F2;
		case sf::Keyboard::Key::F3: return KeyCode::F3;
		case sf::Keyboard::Key::F4: return KeyCode::F4;
		case sf::Keyboard::Key::F5: return KeyCode::F5;
		case sf::Keyboard::Key::F6: return KeyCode::F6;
		case sf::Keyboard::Key::F7: return KeyCode::F7;
		case sf::Keyboard::Key::F8: return KeyCode::F8;
		case sf::Keyboard::Key::F9: return KeyCode::F9;
		case sf::Keyboard::Key::F10: return KeyCode::F10;
		case sf::Keyboard::Key::F11: return KeyCode::F11;
		case sf::Keyboard::Key::F12: return KeyCode::F12;
		case sf::Keyboard::Key::F13: return KeyCode::F13;
		case sf::Keyboard::Key::F14: return KeyCode::F14;
		case sf::Keyboard::Key::F15: return KeyCode::F15;
		case sf::Keyboard::Key::Pause: return KeyCode::Pause;
		default: return KeyCode::Unknown;
		}
	}

	// The engine's picture of the object, as a texture stretched over the
	// object's size in the game's pixels.
	void SFMLWindow::build(Object& object, CachedVisual& visual)
	{
		const std::shared_ptr<const Bitmap> picture = object.drawnPicture(pictureScale());
		const auto width = static_cast<unsigned int>(std::max(picture->width, 1));
		const auto height = static_cast<unsigned int>(std::max(picture->height, 1));

		if (picture->rgba.empty())
		{
			const std::uint8_t clear[4] = { 0, 0, 0, 0 };
			if (!visual.texture.resize({ 1, 1 })) { std::cout << "error: SFML: failed to make a texture for '" << object.name << "'\n"; }
			visual.texture.update(clear);
		}
		else if (!visual.texture.loadFromImage(sf::Image(sf::Vector2u(width, height), picture->rgba.data())))
		{
			std::cout << "error: SFML: failed to make a texture from the picture of '" << object.name << "'\n";
		}
		visual.texture.setSmooth(object.drawnAtScale());

		visual.sprite = std::make_unique<sf::Sprite>(visual.texture);
		const Vector2f size = object.pictureSize();
		const sf::Vector2u made = visual.texture.getSize();
		visual.sprite->setScale({ size.x / static_cast<float>(made.x), size.y / static_cast<float>(made.y) });
		visual.sprite->setPosition({ object.position.x, object.position.y });
		visual.drawnAt = pictureScale();

		object.visualDirty = false;
	}
}
