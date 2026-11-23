// web.cpp
// XML Game Engine
// author: beefviper
// date: Nov 23, 2026

#include "window/web.h"

#include "picture/color.h"

#include <emscripten.h>

#include <cmath>
#include <cstdint>
#include <utility>

// The page's side, kept in window.xgeWeb: the canvas, a canvas a picture
// (by number, from 1), the keys heard since the last pollEvents (a key's
// number twice, plus one when it went down), and the mouse.

EM_JS(void, xge_web_open, (int width, int height, int scale, const char* title), {
	var web = window.xgeWeb = { width: width, height: height, pictures: [null], keys: [], codes: {}, held: {},
		mouse: [false, false, false], pointerX: 0, pointerY: 0, inside: false };
	document.title = UTF8ToString(title);
	var canvas = document.getElementById('game');
	if (!canvas) {
		canvas = document.createElement('canvas');
		canvas.id = 'game';
		document.body.appendChild(canvas);
	}
	web.canvas = canvas;
	web.context = canvas.getContext('2d');

	// As large as the browser's window holds, the game's proportions kept,
	// with a canvas pixel for each of the screen's.
	function fit() {
		var ratio = window.devicePixelRatio || 1;
		var size = Math.min(window.innerWidth / width, window.innerHeight / height);
		if (!(size > 0)) { size = scale; }
		canvas.style.width = Math.floor(width * size) + 'px';
		canvas.style.height = Math.floor(height * size) + 'px';
		canvas.width = Math.max(1, Math.round(width * size * ratio));
		canvas.height = Math.max(1, Math.round(height * size * ratio));
	}
	fit();
	window.addEventListener('resize', fit);

	// The page keeps a key the game uses (arrows and Space would scroll it),
	// but not the browser's own: reload, full screen, its tools, Ctrl with a key.
	var browsers = { F5: true, F11: true, F12: true };
	function key(event, down) {
		var code = web.codes[event.code];
		if (code === undefined) { return; }
		if (!browsers[event.code] && !event.ctrlKey && !event.metaKey) { event.preventDefault(); }
		if (event.repeat || !!web.held[code] === down) { return; }
		web.held[code] = down;
		web.keys.push(code * 2 + (down ? 1 : 0));
	}
	window.addEventListener('keydown', function(event) { key(event, true); });
	window.addEventListener('keyup', function(event) { key(event, false); });

	// Away from the page, nothing stays held.
	window.addEventListener('blur', function() {
		for (var code in web.held) {
			if (web.held[code]) { web.held[code] = false; web.keys.push(Number(code) * 2); }
		}
		web.mouse = [false, false, false];
	});

	function point(event) {
		var box = canvas.getBoundingClientRect();
		web.pointerX = (event.clientX - box.left) / box.width * width;
		web.pointerY = (event.clientY - box.top) / box.height * height;
		web.inside = web.pointerX >= 0 && web.pointerY >= 0 && web.pointerX < width && web.pointerY < height;
	}
	var buttons = [0, 2, 1];   // the page's left, middle, right as the engine's left, right, middle
	canvas.addEventListener('pointermove', point);
	canvas.addEventListener('pointerleave', function() { web.inside = false; });
	canvas.addEventListener('pointerdown', function(event) { point(event); if (event.button < 3) { web.mouse[buttons[event.button]] = true; } event.preventDefault(); });
	window.addEventListener('pointerup', function(event) { if (event.button < 3) { web.mouse[buttons[event.button]] = false; } });
	canvas.addEventListener('contextmenu', function(event) { event.preventDefault(); });
});

EM_JS(void, xge_web_key_code, (const char* code, int key), {
	window.xgeWeb.codes[UTF8ToString(code)] = key;
});

EM_JS(void, xge_web_title, (const char* title), {
	document.title = UTF8ToString(title);
});

EM_JS(int, xge_web_next_key, (void), {
	var keys = window.xgeWeb.keys;
	return keys.length ? keys.shift() : -1;
});

EM_JS(double, xge_web_fit, (void), {
	return window.xgeWeb.canvas.width / window.xgeWeb.width;
});

// The mouse: whether it is over the game, its buttons as bits (left, right,
// middle), where it is in the game's pixels into `where`.
EM_JS(int, xge_web_mouse, (float* where), {
	var web = window.xgeWeb;
	HEAPF32[where >> 2] = web.pointerX;
	HEAPF32[(where >> 2) + 1] = web.pointerY;
	return (web.inside ? 8 : 0) | (web.mouse[0] ? 1 : 0) | (web.mouse[1] ? 2 : 0) | (web.mouse[2] ? 4 : 0);
});

// A pad in the browser's standard layout: its buttons as bits in PadButton's
// order and its left stick into `stick`, or -1 when it is not there.
EM_JS(int, xge_web_pad, (int pad, float* stick), {
	var pads = navigator.getGamepads ? navigator.getGamepads() : [];
	var found = pads && pads[pad];
	if (!found || !found.connected) { return -1; }
	var order = [0, 1, 2, 3, 4, 5, 8, 9, 12, 13, 14, 15];
	var bits = 0;
	for (var i = 0; i < order.length; ++i) {
		var button = found.buttons[order[i]];
		if (button && button.pressed) { bits |= 1 << i; }
	}
	HEAPF32[stick >> 2] = found.axes.length > 0 ? found.axes[0] : 0;
	HEAPF32[(stick >> 2) + 1] = found.axes.length > 1 ? found.axes[1] : 0;
	return bits;
});

// A picture's canvas made, or made again in place: its number.
EM_JS(int, xge_web_picture, (int number, const std::uint8_t* rgba, int width, int height), {
	var web = window.xgeWeb;
	var picture = number > 0 ? web.pictures[number] : null;
	if (!picture) {
		picture = document.createElement('canvas');
		number = web.pictures.length;
		web.pictures.push(picture);
	}
	picture.width = width;
	picture.height = height;
	var pixels = new Uint8ClampedArray(HEAPU8.subarray(rgba, rgba + width * height * 4));
	picture.getContext('2d').putImageData(new ImageData(pixels, width, height), 0, 0);
	return number;
});

EM_JS(void, xge_web_clear, (int red, int green, int blue), {
	var web = window.xgeWeb;
	web.context.fillStyle = 'rgb(' + red + ',' + green + ',' + blue + ')';
	web.context.fillRect(0, 0, web.canvas.width, web.canvas.height);
});

// A picture over the game's pixels from (x, y), width by height, on whole
// canvas pixels.
EM_JS(void, xge_web_draw, (int number, float x, float y, float width, float height, int smooth), {
	var web = window.xgeWeb;
	var picture = web.pictures[number];
	if (!picture) { return; }
	var each = web.canvas.width / web.width;
	var left = Math.round(x * each);
	var top = Math.round(y * each);
	web.context.imageSmoothingEnabled = smooth !== 0;
	web.context.drawImage(picture, left, top, Math.round((x + width) * each) - left, Math.round((y + height) * each) - top);
});

namespace xge
{
	namespace
	{
		// The page's names for keys (KeyboardEvent.code), as the engine's.
		constexpr std::pair<const char*, KeyCode> keyTable[] = {
			{ "KeyA", KeyCode::A }, { "KeyB", KeyCode::B }, { "KeyC", KeyCode::C }, { "KeyD", KeyCode::D },
			{ "KeyE", KeyCode::E }, { "KeyF", KeyCode::F }, { "KeyG", KeyCode::G }, { "KeyH", KeyCode::H },
			{ "KeyI", KeyCode::I }, { "KeyJ", KeyCode::J }, { "KeyK", KeyCode::K }, { "KeyL", KeyCode::L },
			{ "KeyM", KeyCode::M }, { "KeyN", KeyCode::N }, { "KeyO", KeyCode::O }, { "KeyP", KeyCode::P },
			{ "KeyQ", KeyCode::Q }, { "KeyR", KeyCode::R }, { "KeyS", KeyCode::S }, { "KeyT", KeyCode::T },
			{ "KeyU", KeyCode::U }, { "KeyV", KeyCode::V }, { "KeyW", KeyCode::W }, { "KeyX", KeyCode::X },
			{ "KeyY", KeyCode::Y }, { "KeyZ", KeyCode::Z },

			{ "Digit0", KeyCode::Num0 }, { "Digit1", KeyCode::Num1 }, { "Digit2", KeyCode::Num2 },
			{ "Digit3", KeyCode::Num3 }, { "Digit4", KeyCode::Num4 }, { "Digit5", KeyCode::Num5 },
			{ "Digit6", KeyCode::Num6 }, { "Digit7", KeyCode::Num7 }, { "Digit8", KeyCode::Num8 },
			{ "Digit9", KeyCode::Num9 },

			{ "Escape", KeyCode::Escape },
			{ "ControlLeft", KeyCode::LControl }, { "ShiftLeft", KeyCode::LShift },
			{ "AltLeft", KeyCode::LAlt }, { "MetaLeft", KeyCode::LSystem },
			{ "ControlRight", KeyCode::RControl }, { "ShiftRight", KeyCode::RShift },
			{ "AltRight", KeyCode::RAlt }, { "MetaRight", KeyCode::RSystem },
			{ "ContextMenu", KeyCode::Menu },

			{ "BracketLeft", KeyCode::LBracket }, { "BracketRight", KeyCode::RBracket },
			{ "Semicolon", KeyCode::Semicolon }, { "Comma", KeyCode::Comma }, { "Period", KeyCode::Period },
			{ "Quote", KeyCode::Apostrophe }, { "Slash", KeyCode::Slash },
			{ "Backslash", KeyCode::Backslash }, { "Backquote", KeyCode::Grave },
			{ "Equal", KeyCode::Equal }, { "Minus", KeyCode::Hyphen },

			{ "Space", KeyCode::Space }, { "Enter", KeyCode::Enter }, { "NumpadEnter", KeyCode::Enter },
			{ "Backspace", KeyCode::Backspace }, { "Tab", KeyCode::Tab },
			{ "PageUp", KeyCode::PageUp }, { "PageDown", KeyCode::PageDown },
			{ "End", KeyCode::End }, { "Home", KeyCode::Home },
			{ "Insert", KeyCode::Insert }, { "Delete", KeyCode::Delete },

			{ "NumpadAdd", KeyCode::Add }, { "NumpadSubtract", KeyCode::Subtract },
			{ "NumpadMultiply", KeyCode::Multiply }, { "NumpadDivide", KeyCode::Divide },

			{ "ArrowLeft", KeyCode::Left }, { "ArrowRight", KeyCode::Right },
			{ "ArrowUp", KeyCode::Up }, { "ArrowDown", KeyCode::Down },

			{ "Numpad0", KeyCode::Numpad0 }, { "Numpad1", KeyCode::Numpad1 }, { "Numpad2", KeyCode::Numpad2 },
			{ "Numpad3", KeyCode::Numpad3 }, { "Numpad4", KeyCode::Numpad4 }, { "Numpad5", KeyCode::Numpad5 },
			{ "Numpad6", KeyCode::Numpad6 }, { "Numpad7", KeyCode::Numpad7 }, { "Numpad8", KeyCode::Numpad8 },
			{ "Numpad9", KeyCode::Numpad9 },

			{ "F1", KeyCode::F1 }, { "F2", KeyCode::F2 }, { "F3", KeyCode::F3 }, { "F4", KeyCode::F4 },
			{ "F5", KeyCode::F5 }, { "F6", KeyCode::F6 }, { "F7", KeyCode::F7 }, { "F8", KeyCode::F8 },
			{ "F9", KeyCode::F9 }, { "F10", KeyCode::F10 }, { "F11", KeyCode::F11 }, { "F12", KeyCode::F12 },
			{ "F13", KeyCode::F13 }, { "F14", KeyCode::F14 }, { "F15", KeyCode::F15 },

			{ "Pause", KeyCode::Pause },
		};
	}

	WebWindow::WebWindow(const WindowDesc& windowDesc)
	{
		xge_web_open(static_cast<int>(windowDesc.width), static_cast<int>(windowDesc.height), windowDesc.scale, windowDesc.name.c_str());
		for (const auto& [code, key] : keyTable)
		{
			xge_web_key_code(code, static_cast<int>(key));
		}
		setPictureScale(fit());
	}

	WebWindow::~WebWindow() = default;

	void WebWindow::setTitle(const std::string& title)
	{
		xge_web_title(title.c_str());
	}

	void WebWindow::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			build(object, visuals[object.name]);
		}
	}

	std::vector<std::pair<KeyCode, bool>> WebWindow::pollEvents()
	{
		std::vector<std::pair<KeyCode, bool>> events;
		for (int heard = xge_web_next_key(); heard >= 0; heard = xge_web_next_key())
		{
			events.emplace_back(static_cast<KeyCode>(heard / 2), heard % 2 == 1);
		}
		return events;
	}

	Controls WebWindow::controls()
	{
		Controls controls;
		float where[2]{};
		const int mouse = xge_web_mouse(where);
		controls.pointerInside = (mouse & 8) != 0;
		controls.pointerX = where[0];
		controls.pointerY = where[1];
		controls.mouse = { (mouse & 1) != 0, (mouse & 2) != 0, (mouse & 4) != 0 };

		for (std::size_t pad = 0; pad < padCount; ++pad)
		{
			float stick[2]{};
			const int buttons = xge_web_pad(static_cast<int>(pad), stick);
			if (buttons < 0) { continue; }
			Pad& now = controls.pads[pad];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = (buttons & (1 << button)) != 0;
			}
			now.x = stick[0];
			now.y = stick[1];
		}
		return controls;
	}

	void WebWindow::clear(const std::string& colorName)
	{
		const Color color = colorFromName(colorName);
		xge_web_clear(color.r, color.g, color.b);
	}

	void WebWindow::prepare(Object& object)
	{
		CachedVisual& visual = visuals[object.name];
		if (object.drawn.visualDirty || !visual.built || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}
	}

	void WebWindow::draw(Object& object)
	{
		prepare(object);
		const CachedVisual& visual = visuals[object.name];
		if (visual.canvas == 0)
		{
			return;
		}

		// Whole game pixels, as every window places them.
		const Vector2f size = object.pictureSize();
		xge_web_draw(visual.canvas, std::floor(object.position.x), std::floor(object.position.y), size.x, size.y, visual.smooth ? 1 : 0);
	}

	float WebWindow::fit() const
	{
		return static_cast<float>(xge_web_fit());
	}

	void WebWindow::build(Object& object, CachedVisual& visual)
	{
		object.drawn.visualDirty = false;
		visual.drawnAt = pictureScale();
		visual.built = true;
		visual.smooth = object.drawnAtScale();
		visual.canvas = textures.of(pictureOf(object), [](const Bitmap& picture, int& made)
		{
			if (picture.rgba.empty())
			{
				made = 0;
				return;
			}
			made = xge_web_picture(made, picture.rgba.data(), picture.width, picture.height);
		});
	}
}
