// direct3d11.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/direct3d11.h"

#include "picture/color.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <xinput.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace xge
{
	namespace
	{
		constexpr wchar_t kClassName[] = L"xge-direct3d11";

		// One rectangle's corner: where it is in the game's pixels, and where
		// in the picture.
		struct Corner
		{
			float x;
			float y;
			float u;
			float v;
		};

		// The game's size, for the vertex shader to turn its pixels into
		// Direct3D's -1 to 1 (a constant buffer is a multiple of 16 bytes).
		struct Frame
		{
			float width;
			float height;
			float unused[2];
		};

		// One textured rectangle: positions in the game's pixels, the picture
		// as it is (straight alpha, blended over what is there).
		constexpr char kShaders[] = R"(
cbuffer Frame : register(b0) { float2 gameSize; float2 unused; };
struct Corner { float2 position : POSITION; float2 uv : TEXCOORD0; };
struct Shaded { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
Shaded vs(Corner corner)
{
	Shaded shaded;
	shaded.position = float4(corner.position.x / gameSize.x * 2.0 - 1.0, 1.0 - corner.position.y / gameSize.y * 2.0, 0.0, 1.0);
	shaded.uv = corner.uv;
	return shaded;
}
Texture2D picture : register(t0);
SamplerState how : register(s0);
float4 ps(Shaded shaded) : SV_TARGET { return picture.Sample(how, shaded.uv); }
)";

		std::wstring wide(const std::string& text)
		{
			if (text.empty()) { return {}; }
			const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
			std::wstring result(static_cast<std::size_t>(length), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
			return result;
		}

		ComPtr<ID3DBlob> compile(const char* entry, const char* target)
		{
			ComPtr<ID3DBlob> code;
			ComPtr<ID3DBlob> errors;
			if (FAILED(D3DCompile(kShaders, sizeof(kShaders) - 1, "xge", nullptr, nullptr, entry, target, 0, 0, &code, &errors)))
			{
				std::string why = "Direct3D 11 could not compile its shaders";
				if (errors) { why += ": " + std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize()); }
				throw std::runtime_error(why);
			}
			return code;
		}

		// A key as Win32 reports it. Shift, Control and Alt say which side
		// from the scan code and the extended-key bit.
		KeyCode keyFromWin32(WPARAM key, LPARAM detail) noexcept
		{
			const bool extended = (detail & (1 << 24)) != 0;
			if (key >= 'A' && key <= 'Z') { return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + static_cast<int>(key - 'A')); }
			if (key >= '0' && key <= '9') { return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + static_cast<int>(key - '0')); }
			if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) { return static_cast<KeyCode>(static_cast<int>(KeyCode::Numpad0) + static_cast<int>(key - VK_NUMPAD0)); }
			if (key >= VK_F1 && key <= VK_F15) { return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + static_cast<int>(key - VK_F1)); }
			switch (key)
			{
			case VK_SHIFT:
			{
				const UINT scan = static_cast<UINT>((detail >> 16) & 0xFF);
				return MapVirtualKeyW(scan, MAPVK_VSC_TO_VK_EX) == static_cast<UINT>(VK_RSHIFT) ? KeyCode::RShift : KeyCode::LShift;
			}
			case VK_CONTROL: return extended ? KeyCode::RControl : KeyCode::LControl;
			case VK_MENU: return extended ? KeyCode::RAlt : KeyCode::LAlt;
			case VK_LWIN: return KeyCode::LSystem;
			case VK_RWIN: return KeyCode::RSystem;
			case VK_APPS: return KeyCode::Menu;
			case VK_ESCAPE: return KeyCode::Escape;
			case VK_OEM_4: return KeyCode::LBracket;
			case VK_OEM_6: return KeyCode::RBracket;
			case VK_OEM_1: return KeyCode::Semicolon;
			case VK_OEM_COMMA: return KeyCode::Comma;
			case VK_OEM_PERIOD: return KeyCode::Period;
			case VK_OEM_7: return KeyCode::Apostrophe;
			case VK_OEM_2: return KeyCode::Slash;
			case VK_OEM_5: return KeyCode::Backslash;
			case VK_OEM_3: return KeyCode::Grave;
			case VK_OEM_PLUS: return KeyCode::Equal;
			case VK_OEM_MINUS: return KeyCode::Hyphen;
			case VK_SPACE: return KeyCode::Space;
			case VK_RETURN: return KeyCode::Enter;
			case VK_BACK: return KeyCode::Backspace;
			case VK_TAB: return KeyCode::Tab;
			case VK_PRIOR: return KeyCode::PageUp;
			case VK_NEXT: return KeyCode::PageDown;
			case VK_END: return KeyCode::End;
			case VK_HOME: return KeyCode::Home;
			case VK_INSERT: return KeyCode::Insert;
			case VK_DELETE: return KeyCode::Delete;
			case VK_ADD: return KeyCode::Add;
			case VK_SUBTRACT: return KeyCode::Subtract;
			case VK_MULTIPLY: return KeyCode::Multiply;
			case VK_DIVIDE: return KeyCode::Divide;
			case VK_LEFT: return KeyCode::Left;
			case VK_RIGHT: return KeyCode::Right;
			case VK_UP: return KeyCode::Up;
			case VK_DOWN: return KeyCode::Down;
			case VK_PAUSE: return KeyCode::Pause;
			default: return KeyCode::Unknown;
			}
		}
	}

	struct Direct3D11Window::Parts
	{
		// One object's picture, on the graphics card.
		struct Visual
		{
			ComPtr<ID3D11ShaderResourceView> picture;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		HWND window{ nullptr };
		float gameWidth{ 0.0f };
		float gameHeight{ 0.0f };
		bool closeRequested{ false };
		bool resized{ false };
		std::vector<std::pair<KeyCode, bool>> keys;

		// Where the game is drawn in the window: as large as fits with its
		// proportions, centred on whole pixels (the rest is bars).
		float fit{ 1.0f };
		D3D11_VIEWPORT shown{};

		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		ComPtr<IDXGISwapChain> swapChain;
		ComPtr<ID3D11RenderTargetView> target;
		ComPtr<ID3D11VertexShader> vertexShader;
		ComPtr<ID3D11PixelShader> pixelShader;
		ComPtr<ID3D11InputLayout> layout;
		ComPtr<ID3D11Buffer> corners;
		ComPtr<ID3D11Buffer> frame;
		ComPtr<ID3D11SamplerState> hardEdges;
		ComPtr<ID3D11SamplerState> smooth;
		ComPtr<ID3D11BlendState> blend;
		ComPtr<ID3D11RasterizerState> rasterizer;

		std::unordered_map<std::string, Visual> visuals;

		static LRESULT CALLBACK receive(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
		{
			if (message == WM_NCCREATE)
			{
				const auto* made = reinterpret_cast<const CREATESTRUCTW*>(lParam);
				SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(made->lpCreateParams));
			}
			auto* self = reinterpret_cast<Parts*>(GetWindowLongPtrW(window, GWLP_USERDATA));
			if (self) { return self->handle(window, message, wParam, lParam); }
			return DefWindowProcW(window, message, wParam, lParam);
		}

		LRESULT handle(HWND from, UINT message, WPARAM wParam, LPARAM lParam)
		{
			switch (message)
			{
			case WM_CLOSE:
				// Not torn down here: the frame that reads this still draws.
				closeRequested = true;
				return 0;
			case WM_SIZE:
				resized = true;
				return 0;
			case WM_SYSCHAR:
				return 0;   // Alt with a letter: no beep for a menu there is not
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{
				const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
				const bool repeat = down && (lParam & (1 << 30)) != 0;
				const KeyCode key = keyFromWin32(wParam, lParam);
				if (!repeat && key != KeyCode::Unknown) { keys.emplace_back(key, down); }
				// Alt and F10 would otherwise open the window's menu and pause
				// the game; Alt+F4 still closes it.
				if (message == WM_SYSKEYDOWN && wParam == VK_F4) { break; }
				return 0;
			}
			default:
				break;
			}
			return DefWindowProcW(from, message, wParam, lParam);
		}

		void makeTarget()
		{
			ComPtr<ID3D11Texture2D> back;
			if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&back))) || FAILED(device->CreateRenderTargetView(back.Get(), nullptr, &target)))
			{
				throw std::runtime_error("Direct3D 11 could not draw into its window");
			}
		}

		void fitGame()
		{
			RECT client{};
			GetClientRect(window, &client);
			const float wide = static_cast<float>(std::max<LONG>(client.right - client.left, 1));
			const float high = static_cast<float>(std::max<LONG>(client.bottom - client.top, 1));
			fit = std::min(wide / gameWidth, high / gameHeight);
			shown.Width = std::floor(gameWidth * fit);
			shown.Height = std::floor(gameHeight * fit);
			shown.TopLeftX = std::floor((wide - shown.Width) / 2.0f);
			shown.TopLeftY = std::floor((high - shown.Height) / 2.0f);
			shown.MinDepth = 0.0f;
			shown.MaxDepth = 1.0f;
		}

		// After a resize: the window's pictures made again at its new size.
		void resize()
		{
			resized = false;
			RECT client{};
			GetClientRect(window, &client);
			if (client.right <= client.left || client.bottom <= client.top) { return; }   // minimised
			context->OMSetRenderTargets(0, nullptr, nullptr);
			target.Reset();
			if (FAILED(swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)))
			{
				throw std::runtime_error("Direct3D 11 could not resize its window");
			}
			makeTarget();
			fitGame();
		}

		void build(Object& object, float scale)
		{
			object.visualDirty = false;
			Visual& visual = visuals[object.name];
			visual.drawnAt = scale;
			visual.picture.Reset();

			const std::shared_ptr<const Bitmap> picture = object.drawnPicture(scale);
			if (picture->rgba.empty() || picture->width <= 0 || picture->height <= 0) { return; }

			D3D11_TEXTURE2D_DESC description{};
			description.Width = static_cast<UINT>(picture->width);
			description.Height = static_cast<UINT>(picture->height);
			description.MipLevels = 1;
			description.ArraySize = 1;
			description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.Usage = D3D11_USAGE_IMMUTABLE;
			description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			D3D11_SUBRESOURCE_DATA pixels{};
			pixels.pSysMem = picture->rgba.data();
			pixels.SysMemPitch = static_cast<UINT>(picture->width) * 4;

			ComPtr<ID3D11Texture2D> texture;
			if (FAILED(device->CreateTexture2D(&description, &pixels, &texture))) { return; }
			device->CreateShaderResourceView(texture.Get(), nullptr, &visual.picture);
		}
	};

	Direct3D11Window::Direct3D11Window(const WindowDesc& windowDesc) :
		parts(std::make_unique<Parts>())
	{
		Parts& p = *parts;
		p.gameWidth = std::max(windowDesc.width, 1.0f);
		p.gameHeight = std::max(windowDesc.height, 1.0f);
		const int scale = std::max(1, windowDesc.scale);

		const HINSTANCE instance = GetModuleHandleW(nullptr);
		WNDCLASSEXW kind{};
		kind.cbSize = sizeof(kind);
		kind.style = CS_HREDRAW | CS_VREDRAW;
		kind.lpfnWndProc = &Parts::receive;
		kind.hInstance = instance;
		kind.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));   // the arrow (IDC_ARROW)
		kind.lpszClassName = kClassName;
		RegisterClassExW(&kind);   // already registered by an earlier window: fine

		// The game's size times its scale inside the frame, or the whole
		// screen.
		const bool fullscreen = windowDesc.fullscreen == "true";
		const DWORD style = fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
		RECT frame{ 0, 0, static_cast<LONG>(p.gameWidth) * scale, static_cast<LONG>(p.gameHeight) * scale };
		if (fullscreen) { frame = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) }; }
		else { AdjustWindowRect(&frame, style, FALSE); }

		p.window = CreateWindowExW(0, kClassName, wide(windowDesc.name).c_str(), style,
			fullscreen ? 0 : CW_USEDEFAULT, fullscreen ? 0 : CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top,
			nullptr, nullptr, instance, &p);
		if (!p.window) { throw std::runtime_error("Direct3D 11 could not make a window"); }

		DXGI_SWAP_CHAIN_DESC chain{};
		chain.BufferCount = 1;
		chain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		chain.OutputWindow = p.window;
		chain.SampleDesc.Count = 1;
		chain.Windowed = TRUE;
		chain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
		if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, static_cast<UINT>(std::size(levels)),
			D3D11_SDK_VERSION, &chain, &p.swapChain, &p.device, nullptr, &p.context)))
		{
			DestroyWindow(p.window);
			p.window = nullptr;
			throw std::runtime_error("Direct3D 11 could not start on this graphics card");
		}
		p.makeTarget();

		const ComPtr<ID3DBlob> vertexCode = compile("vs", "vs_4_0");
		const ComPtr<ID3DBlob> pixelCode = compile("ps", "ps_4_0");
		p.device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, &p.vertexShader);
		p.device->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, &p.pixelShader);
		const D3D11_INPUT_ELEMENT_DESC corner[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		p.device->CreateInputLayout(corner, 2, vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), &p.layout);

		D3D11_BUFFER_DESC corners{};
		corners.ByteWidth = sizeof(Corner) * 4;
		corners.Usage = D3D11_USAGE_DYNAMIC;
		corners.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		corners.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		p.device->CreateBuffer(&corners, nullptr, &p.corners);

		const Frame size{ p.gameWidth, p.gameHeight, { 0.0f, 0.0f } };
		D3D11_BUFFER_DESC frameBuffer{};
		frameBuffer.ByteWidth = sizeof(Frame);
		frameBuffer.Usage = D3D11_USAGE_IMMUTABLE;
		frameBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		D3D11_SUBRESOURCE_DATA sizeData{};
		sizeData.pSysMem = &size;
		p.device->CreateBuffer(&frameBuffer, &sizeData, &p.frame);

		D3D11_SAMPLER_DESC sampler{};
		sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler.MaxLOD = D3D11_FLOAT32_MAX;
		sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		p.device->CreateSamplerState(&sampler, &p.hardEdges);
		sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		p.device->CreateSamplerState(&sampler, &p.smooth);

		D3D11_BLEND_DESC blend{};
		blend.RenderTarget[0].BlendEnable = TRUE;
		blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		p.device->CreateBlendState(&blend, &p.blend);

		// Rectangles are drawn whichever way round their corners come.
		D3D11_RASTERIZER_DESC rasterizer{};
		rasterizer.FillMode = D3D11_FILL_SOLID;
		rasterizer.CullMode = D3D11_CULL_NONE;
		rasterizer.DepthClipEnable = TRUE;
		p.device->CreateRasterizerState(&rasterizer, &p.rasterizer);

		if (!p.vertexShader || !p.pixelShader || !p.layout || !p.corners || !p.frame || !p.hardEdges || !p.smooth || !p.blend || !p.rasterizer)
		{
			throw std::runtime_error("Direct3D 11 could not set up its drawing");
		}

		p.fitGame();
		setPictureScale(p.fit);
		ShowWindow(p.window, SW_SHOW);
		UpdateWindow(p.window);
	}

	Direct3D11Window::~Direct3D11Window()
	{
		close();
	}

	bool Direct3D11Window::isOpen() const
	{
		return parts->window != nullptr && !parts->closeRequested;
	}

	void Direct3D11Window::close()
	{
		Parts& p = *parts;
		if (!p.window) { return; }
		p.visuals.clear();
		if (p.context) { p.context->ClearState(); }
		DestroyWindow(p.window);
		p.window = nullptr;
	}

	std::pair<int, int> Direct3D11Window::position() const
	{
		RECT where{};
		if (!parts->window || !GetWindowRect(parts->window, &where)) { return { 0, 0 }; }
		return { static_cast<int>(where.left), static_cast<int>(where.top) };
	}

	void Direct3D11Window::setPosition(int x, int y)
	{
		if (parts->window) { SetWindowPos(parts->window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE); }
	}

	void Direct3D11Window::setTitle(const std::string& title)
	{
		if (parts->window) { SetWindowTextW(parts->window, wide(title).c_str()); }
	}

	void Direct3D11Window::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			parts->build(object, pictureScale());
		}
	}

	std::vector<std::pair<KeyCode, bool>> Direct3D11Window::pollEvents()
	{
		MSG message{};
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
		std::vector<std::pair<KeyCode, bool>> events;
		events.swap(parts->keys);
		return events;
	}

	Controls Direct3D11Window::controls()
	{
		Controls controls;
		const Parts& p = *parts;
		if (!p.window || GetForegroundWindow() != p.window) { return controls; }

		POINT at{};
		GetCursorPos(&at);
		ScreenToClient(p.window, &at);
		RECT client{};
		GetClientRect(p.window, &client);
		controls.pointerInside = at.x >= 0 && at.y >= 0 && at.x < client.right && at.y < client.bottom;
		controls.pointerX = (static_cast<float>(at.x) - p.shown.TopLeftX) / p.fit;
		controls.pointerY = (static_cast<float>(at.y) - p.shown.TopLeftY) / p.fit;
		controls.mouse = { (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0,
			(GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0 };

		// XInput numbers its pads 0 to 3, each an Xbox layout.
		constexpr WORD buttons[padButtonCount] = {
			XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
			XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START,
			XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
		};
		std::size_t pad = 0;
		for (DWORD user = 0; user < XUSER_MAX_COUNT && pad < padCount; ++user)
		{
			XINPUT_STATE state{};
			if (XInputGetState(user, &state) != ERROR_SUCCESS) { continue; }
			Pad& now = controls.pads[pad++];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = (state.Gamepad.wButtons & buttons[button]) != 0;
			}
			now.x = std::clamp(static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f, -1.0f, 1.0f);
			now.y = std::clamp(-static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f, -1.0f, 1.0f);
		}
		return controls;
	}

	void Direct3D11Window::clear(const std::string& colorName)
	{
		Parts& p = *parts;
		if (p.resized) { p.resize(); }

		// The whole window in the background's colour, bars included.
		const Color c = colorFromName(colorName);
		const float background[4] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f };
		p.context->ClearRenderTargetView(p.target.Get(), background);

		ID3D11RenderTargetView* targets[] = { p.target.Get() };
		p.context->OMSetRenderTargets(1, targets, nullptr);
		p.context->RSSetViewports(1, &p.shown);
		p.context->RSSetState(p.rasterizer.Get());
		p.context->OMSetBlendState(p.blend.Get(), nullptr, 0xFFFFFFFF);
		p.context->IASetInputLayout(p.layout.Get());
		p.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		const UINT stride = sizeof(Corner);
		const UINT offset = 0;
		ID3D11Buffer* vertices[] = { p.corners.Get() };
		p.context->IASetVertexBuffers(0, 1, vertices, &stride, &offset);
		ID3D11Buffer* constants[] = { p.frame.Get() };
		p.context->VSSetConstantBuffers(0, 1, constants);
		p.context->VSSetShader(p.vertexShader.Get(), nullptr, 0);
		p.context->PSSetShader(p.pixelShader.Get(), nullptr, 0);
	}

	void Direct3D11Window::prepare(Object& object)
	{
		Parts& p = *parts;
		const auto found = p.visuals.find(object.name);
		if (object.visualDirty || found == p.visuals.end() || (object.drawnAtScale() && found->second.drawnAt != pictureScale()))
		{
			p.build(object, pictureScale());
		}
	}

	void Direct3D11Window::draw(Object& object)
	{
		prepare(object);
		Parts& p = *parts;
		const Parts::Visual& visual = p.visuals[object.name];
		if (!visual.picture) { return; }

		// Whole game pixels, so a picture is never sampled between two of
		// its pixels; stretched over the object's size.
		const Vector2f size = object.pictureSize();
		const float x0 = std::floor(object.position.x);
		const float y0 = std::floor(object.position.y);
		const Corner corners[4] = {
			{ x0, y0, 0.0f, 0.0f },
			{ x0 + size.x, y0, 1.0f, 0.0f },
			{ x0, y0 + size.y, 0.0f, 1.0f },
			{ x0 + size.x, y0 + size.y, 1.0f, 1.0f },
		};
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(p.context->Map(p.corners.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) { return; }
		std::memcpy(mapped.pData, corners, sizeof(corners));
		p.context->Unmap(p.corners.Get(), 0);

		ID3D11ShaderResourceView* pictures[] = { visual.picture.Get() };
		p.context->PSSetShaderResources(0, 1, pictures);
		ID3D11SamplerState* samplers[] = { object.drawnAtScale() ? p.smooth.Get() : p.hardEdges.Get() };
		p.context->PSSetSamplers(0, 1, samplers);
		p.context->Draw(4, 0);
	}

	void Direct3D11Window::display()
	{
		// Waits for the screen's refresh, so a picture is never torn; the
		// engine keeps the game's own pace (Engine::loop).
		parts->swapChain->Present(1, 0);
	}

	float Direct3D11Window::fit() const
	{
		return parts->fit;
	}

	Bitmap Direct3D11Window::capture()
	{
		Parts& p = *parts;
		ComPtr<ID3D11Texture2D> back;
		if (FAILED(p.swapChain->GetBuffer(0, IID_PPV_ARGS(&back)))) { return {}; }
		D3D11_TEXTURE2D_DESC description{};
		back->GetDesc(&description);
		description.Usage = D3D11_USAGE_STAGING;
		description.BindFlags = 0;
		description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		description.MiscFlags = 0;
		ComPtr<ID3D11Texture2D> copy;
		if (FAILED(p.device->CreateTexture2D(&description, nullptr, &copy))) { return {}; }
		p.context->CopyResource(copy.Get(), back.Get());

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(p.context->Map(copy.Get(), 0, D3D11_MAP_READ, 0, &mapped))) { return {}; }
		Bitmap bitmap;
		bitmap.width = static_cast<int>(description.Width);
		bitmap.height = static_cast<int>(description.Height);
		const std::size_t row = static_cast<std::size_t>(bitmap.width) * 4;
		bitmap.rgba.resize(row * static_cast<std::size_t>(bitmap.height));
		for (std::size_t y = 0; y < static_cast<std::size_t>(bitmap.height); ++y)
		{
			std::memcpy(bitmap.rgba.data() + y * row, static_cast<const std::uint8_t*>(mapped.pData) + y * mapped.RowPitch, row);
		}
		p.context->Unmap(copy.Get(), 0);
		return bitmap;
	}
}
