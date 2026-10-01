// window_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window_sfml.h"

#include "builtin_font.h"
#include "color.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace xge
{
	namespace
	{
		// The platform's window handle is a pointer on Windows (HWND) and a
		// number on Linux (an X11 window id).
		sf::WindowHandle toSfmlHandle(void* handle)
		{
			// A C-style cast is a pointer cast where the handle is a pointer
			// and an integer cast where it is a number.
			return (sf::WindowHandle)(handle);
		}
	}

	SFMLWindow::SFMLWindow(const WindowDesc& windowDesc, const WindowTarget& target)
	{
		if (target.kind == WindowTarget::Kind::NativeWindow)
		{
			// The front end's window: no frame limit either, the front end
			// decides when a frame is due.
			window.create(toSfmlHandle(target.nativeHandle));

			if (!window.isOpen())
			{
				throw std::runtime_error("SFML could not draw into the window");
			}

			// The window is whatever size the front end made it; the game is
			// drawn whole, scaled to fit it.
			window.setView(sf::View(sf::FloatRect({ 0.f, 0.f },
				{ static_cast<float>(windowDesc.width), static_cast<float>(windowDesc.height) })));

			return;
		}

		const auto width = static_cast<unsigned int>(windowDesc.width);
		const auto height = static_cast<unsigned int>(windowDesc.height);
		const sf::VideoMode videoMode({ width, height });

		const auto windowState = (windowDesc.fullscreen == "true") ? sf::State::Fullscreen : sf::State::Windowed;

		window.create(videoMode, windowDesc.name, sf::Style::Default, windowState);
		window.setFramerateLimit(windowDesc.framerate);
	}

	SFMLWindow::~SFMLWindow()
	{
		// The render textures are freed in whichever context is current, so
		// make it this window's, not another library's.
		static_cast<void>(window.setActive(true));
		visuals.clear();
	}

	bool SFMLWindow::isOpen() const
	{
		return window.isOpen();
	}

	void SFMLWindow::close()
	{
		window.close();
	}

	void SFMLWindow::init(std::vector<Object>& objects)
	{
		// Object::position is already final by now - including any <grid>
		// spacing - finalized entirely within Game's own construction (see
		// game_expr.cpp, and main.cpp for why Engine/Window no longer needs
		// to exist first). All that's left here is building each object's
		// actual visual and measuring its real rendered Object::size.
		for (auto& object : objects)
		{
			CachedVisual& visual = visuals[object.name];

			buildShapeOnly(object, visual);
			finalizeVisual(object, visual);
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

	void SFMLWindow::clear(const std::string& colorName)
	{
		const Color c = colorFromName(colorName);
		window.clear(sf::Color(c.r, c.g, c.b, c.a));
	}

	void SFMLWindow::draw(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.sprite)
		{
			// A rebuild after init() (e.g. a bound text display's number just
			// changed - see Game::incrementText/resetObject/resetAll) never
			// repeats the grid math init() did: object.position already holds
			// a real, finalized screen position by now, not a grid index.
			buildShapeOnly(object, visual);
			finalizeVisual(object, visual);
		}

		// The cached sprite's position has to be refreshed every frame, not
		// just on rebuild: Object::position changes constantly from plain
		// movement (velocity integration, paddle input) with visualDirty
		// never set for that (visualDirty is only for text-content rebuilds -
		// see Game::incrementText/resetObject/resetAll), so without this the
		// sprite you actually see would stay frozen at wherever it was last
		// rebuilt while the real Object::position (and therefore collisions/
		// scoring) kept moving underneath it.
		visual.sprite->setPosition({ object.position.x, object.position.y });

		window.draw(*visual.sprite);
	}

	void SFMLWindow::display()
	{
		window.display();
	}

	void SFMLWindow::activate()
	{
		// SFML remembers which context it made current and would not notice
		// another library having taken it since, so let go and take it again.
		static_cast<void>(window.setActive(false));
		static_cast<void>(window.setActive(true));
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

	const sf::Font& SFMLWindow::getFont()
	{
		if (!fontLoaded)
		{
			const std::string fontFile{ "assets/tuffy.ttf" };
			if (!font.openFromFile(fontFile))
			{
				std::cout << "error: failed to load font: " << fontFile << " - drawing text with the built-in 8x8 font instead" << std::endl;
				fontMissing = true;
			}
			fontLoaded = true;
		}

		return font;
	}

	void SFMLWindow::buildShapeOnly(Object& object, CachedVisual& visual)
	{
		switch (object.shapeKind)
		{
		case ShapeKind::Circle:    buildCircle(object, visual); break;
		case ShapeKind::Rectangle: buildRectangle(object, visual); break;
		case ShapeKind::Text:      buildText(object, visual); break;
		case ShapeKind::Image:     buildImage(object, visual); break;
		case ShapeKind::Line:      buildLines(object, visual); break;
		case ShapeKind::Unknown:   break;
		}
	}

	void SFMLWindow::buildCircle(Object& object, CachedVisual& visual)
	{
		sf::CircleShape circle;

		const float radius = std::stof(object.spriteParams.at(1));
		circle.setRadius(radius);

		const Color c = colorFromName(object.spriteParams.at(3));
		circle.setFillColor(sf::Color(c.r, c.g, c.b, c.a));

		const int width = static_cast<int>(std::ceil(circle.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(circle.getLocalBounds().size.y));

		if (!visual.renderTexture.resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		visual.renderTexture.draw(circle);
	}

	void SFMLWindow::buildRectangle(Object& object, CachedVisual& visual)
	{
		sf::RectangleShape rectangle;

		const float recWidth = std::stof(object.spriteParams.at(1));
		const float recHeight = std::stof(object.spriteParams.at(2));
		rectangle.setSize(sf::Vector2f(recWidth, recHeight));

		const Color c = colorFromName(object.spriteParams.at(3));
		rectangle.setFillColor(sf::Color(c.r, c.g, c.b, c.a));

		const int width = static_cast<int>(std::ceil(rectangle.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(rectangle.getLocalBounds().size.y));

		if (!visual.renderTexture.resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		visual.renderTexture.draw(rectangle);
	}

	void SFMLWindow::buildText(Object& object, CachedVisual& visual)
	{
		const sf::Font& f = getFont();

		if (fontMissing)
		{
			buildBitmap(object, visual, rasterizeText(object.spriteParams.at(1), std::stoi(object.spriteParams.at(2)), colorFromName(object.spriteParams.at(3))));
			return;
		}

		sf::Text text(f);
		text.setString(object.spriteParams.at(1));
		text.setCharacterSize(std::stoi(object.spriteParams.at(2)));

		const Color c = colorFromName(object.spriteParams.at(3));
		text.setFillColor(sf::Color(c.r, c.g, c.b, c.a));
		text.setPosition({ -text.getLocalBounds().position.x, -text.getLocalBounds().position.y });

		const int width = static_cast<int>(std::ceil(text.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(text.getLocalBounds().size.y));

		if (!visual.renderTexture.resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		visual.renderTexture.draw(text);
	}

	void SFMLWindow::buildImage(Object& object, CachedVisual& visual)
	{
		sf::Texture texture;

		auto& imageFile = object.spriteParams.at(1);

		if (!texture.loadFromFile(imageFile))
		{
			std::cout << "error: SFML Image: failed to load " << imageFile << '\n';
			exit(EXIT_FAILURE);
		}

		sf::Sprite sprite(texture);

		const int width = static_cast<int>(std::ceil(sprite.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(sprite.getLocalBounds().size.y));

		if (object.spriteParams.at(2) == "flip.horizontal")
		{
			sprite.setTextureRect(sf::IntRect({ width, 0 }, { -width, height }));
		}
		else if (object.spriteParams.at(2) == "flip.vertical")
		{
			sprite.setTextureRect(sf::IntRect({ 0, height }, { width, -height }));
		}

		if (!visual.renderTexture.resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		visual.renderTexture.draw(sprite);
	}

	// A sprite of lines was already drawn, pixel by pixel, when the game loaded
	// (see Bitmap); all that is left is to show that picture.
	void SFMLWindow::buildLines(Object& object, CachedVisual& visual)
	{
		buildBitmap(object, visual, *object.bitmap);
	}

	// Shows a picture the engine drew itself: a sprite of lines, or text in
	// the built-in font.
	void SFMLWindow::buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap)
	{
		const auto width = static_cast<unsigned int>(bitmap.width > 0 ? bitmap.width : 1);
		const auto height = static_cast<unsigned int>(bitmap.height > 0 ? bitmap.height : 1);

		if (!visual.renderTexture.resize({ width, height }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}

		if (bitmap.rgba.empty())
		{
			return;
		}

		const sf::Image image(sf::Vector2u(width, height), bitmap.rgba.data());

		sf::Texture texture;
		if (!texture.loadFromImage(image))
		{
			std::cout << "error: SFML: failed to make a texture from the picture of '" << object.name << "'\n";
			return;
		}

		visual.renderTexture.draw(sf::Sprite(texture));
	}

	void SFMLWindow::finalizeVisual(Object& object, CachedVisual& visual)
	{
		object.size.x = static_cast<float>(visual.renderTexture.getSize().x);
		object.size.y = static_cast<float>(visual.renderTexture.getSize().y);
		object.sizeKnown = true;

		visual.renderTexture.display();
		visual.sprite = std::make_unique<sf::Sprite>(visual.renderTexture.getTexture());
		visual.sprite->setPosition({ object.position.x, object.position.y });

		object.visualDirty = false;
	}
}
