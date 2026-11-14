// direct3d9.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/direct3d9.h"

#include "picture/color.h"
#include "window/win32.h"

#include <d3d9.h>
#include <wrl/client.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace xge
{
	namespace
	{
		// One rectangle's corner, already in the window's pixels (the fixed
		// pipeline draws it as it is), and where in the picture.
		struct Corner
		{
			float x;
			float y;
			float z;
			float rhw;
			float u;
			float v;
		};
		constexpr DWORD kCorner = D3DFVF_XYZRHW | D3DFVF_TEX1;
	}

	struct Direct3D9Window::Parts
	{
		// One object's picture, on the graphics card. A managed texture keeps
		// a copy in memory, so it outlives a reset of the device.
		struct Visual
		{
			ComPtr<IDirect3DTexture9> picture;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		explicit Parts(const WindowDesc& windowDesc) : frame(windowDesc, "Direct3D 9") {}

		Win32Window frame;

		ComPtr<IDirect3D9> direct3d;
		ComPtr<IDirect3DDevice9> device;
		D3DPRESENT_PARAMETERS present{};
		bool drawing{ false };   // between BeginScene and EndScene

		std::unordered_map<std::string, Visual> visuals;

		// Fits the window's drawing to its size again, after a resize or when
		// the device was lost (another program took the screen) and is back.
		// False while it cannot be yet.
		bool reset()
		{
			present.BackBufferWidth = 0;    // the window's size
			present.BackBufferHeight = 0;
			return SUCCEEDED(device->Reset(&present));
		}

		// Whether this frame can be drawn: the device is there, and not lost.
		bool ready()
		{
			if (!device) { return false; }
			const HRESULT state = device->TestCooperativeLevel();
			if (state == D3DERR_DEVICENOTRESET) { return reset(); }
			return SUCCEEDED(state);
		}

		void build(Object& object, float scale)
		{
			object.visualDirty = false;
			Visual& visual = visuals[object.name];
			visual.drawnAt = scale;
			visual.picture.Reset();

			const std::shared_ptr<const Bitmap> picture = object.drawnPicture(scale);
			if (picture->rgba.empty() || picture->width <= 0 || picture->height <= 0) { return; }

			ComPtr<IDirect3DTexture9> texture;
			if (FAILED(device->CreateTexture(static_cast<UINT>(picture->width), static_cast<UINT>(picture->height), 1, 0,
				D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr))) { return; }
			D3DLOCKED_RECT locked{};
			if (FAILED(texture->LockRect(0, &locked, nullptr, 0))) { return; }

			// The picture is red, green, blue, alpha; the texture holds each
			// pixel as blue, green, red, alpha.
			const std::size_t wide = static_cast<std::size_t>(picture->width);
			for (std::size_t y = 0; y < static_cast<std::size_t>(picture->height); ++y)
			{
				const std::uint8_t* from = picture->rgba.data() + y * wide * 4;
				std::uint8_t* to = static_cast<std::uint8_t*>(locked.pBits) + y * static_cast<std::size_t>(locked.Pitch);
				for (std::size_t x = 0; x < wide; ++x)
				{
					to[x * 4 + 0] = from[x * 4 + 2];
					to[x * 4 + 1] = from[x * 4 + 1];
					to[x * 4 + 2] = from[x * 4 + 0];
					to[x * 4 + 3] = from[x * 4 + 3];
				}
			}
			texture->UnlockRect(0);
			visual.picture = texture;
		}
	};

	Direct3D9Window::Direct3D9Window(const WindowDesc& windowDesc) :
		parts(std::make_unique<Parts>(windowDesc))
	{
		Parts& p = *parts;
		p.direct3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));
		if (!p.direct3d) { throw std::runtime_error("Direct3D 9 is not on this computer"); }

		// A window's worth, swapped on the screen's refresh so a picture is
		// never torn; the engine keeps the game's own pace (Engine::loop).
		p.present.Windowed = TRUE;
		p.present.SwapEffect = D3DSWAPEFFECT_DISCARD;
		p.present.BackBufferFormat = D3DFMT_UNKNOWN;   // the screen's own
		p.present.BackBufferCount = 1;
		p.present.hDeviceWindow = p.frame.handle();
		p.present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

		// FPU_PRESERVE: Direct3D 9 would otherwise turn the processor's sums
		// to single precision for the whole program, and the game's numbers
		// with them.
		const DWORD how = D3DCREATE_FPU_PRESERVE;
		if (FAILED(p.direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, p.frame.handle(), how | D3DCREATE_HARDWARE_VERTEXPROCESSING, &p.present, &p.device))
			&& FAILED(p.direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, p.frame.handle(), how | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &p.present, &p.device)))
		{
			throw std::runtime_error("Direct3D 9 could not start on this graphics card");
		}

		setPictureScale(p.frame.fit());
		p.frame.show();
	}

	Direct3D9Window::~Direct3D9Window()
	{
		close();
	}

	bool Direct3D9Window::isOpen() const
	{
		return parts->frame.isOpen();
	}

	void Direct3D9Window::close()
	{
		Parts& p = *parts;
		if (!p.frame.handle()) { return; }
		p.visuals.clear();
		p.device.Reset();
		p.direct3d.Reset();
		p.frame.destroy();
	}

	std::pair<int, int> Direct3D9Window::position() const
	{
		return parts->frame.position();
	}

	void Direct3D9Window::setPosition(int x, int y)
	{
		parts->frame.setPosition(x, y);
	}

	void Direct3D9Window::setTitle(const std::string& title)
	{
		parts->frame.setTitle(title);
	}

	void Direct3D9Window::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			parts->build(object, pictureScale());
		}
	}

	std::vector<std::pair<KeyCode, bool>> Direct3D9Window::pollEvents()
	{
		return parts->frame.pollEvents();
	}

	Controls Direct3D9Window::controls()
	{
		return parts->frame.controls();
	}

	void Direct3D9Window::clear(const std::string& colorName)
	{
		Parts& p = *parts;
		if (!p.device) { return; }
		if (p.frame.takeResized()) { p.reset(); }
		if (!p.ready()) { return; }

		// The whole window in the background's colour, bars included; a reset
		// forgets every setting, so they are all set each frame.
		const Color c = colorFromName(colorName);
		p.device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		p.device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(c.r, c.g, c.b), 1.0f, 0);

		// Nothing is drawn over the bars.
		const Win32Window::Shown& shown = p.frame.shown();
		const RECT game{ static_cast<LONG>(shown.left), static_cast<LONG>(shown.top),
			static_cast<LONG>(shown.left + shown.width), static_cast<LONG>(shown.top + shown.height) };
		p.device->SetScissorRect(&game);
		p.device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

		p.device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		p.device->SetRenderState(D3DRS_LIGHTING, FALSE);
		p.device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);   // whichever way round the corners come
		p.device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);    // straight alpha, over what is there
		p.device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		p.device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		p.device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);   // the picture as it is
		p.device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		p.device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		p.device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		p.device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		p.device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		p.device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		p.device->SetFVF(kCorner);

		p.drawing = SUCCEEDED(p.device->BeginScene());
	}

	void Direct3D9Window::prepare(Object& object)
	{
		Parts& p = *parts;
		if (!p.device) { return; }
		const auto found = p.visuals.find(object.name);
		if (object.visualDirty || found == p.visuals.end() || (object.drawnAtScale() && found->second.drawnAt != pictureScale()))
		{
			p.build(object, pictureScale());
		}
	}

	void Direct3D9Window::draw(Object& object)
	{
		Parts& p = *parts;
		if (!p.drawing) { return; }
		prepare(object);
		const Parts::Visual& visual = p.visuals[object.name];
		if (!visual.picture) { return; }

		// Whole game pixels, so a picture is never sampled between two of
		// its pixels; stretched over the object's size, then put where the
		// game is shown. Direct3D 9 puts a pixel's centre on whole numbers,
		// hence the half.
		const Win32Window::Shown& shown = p.frame.shown();
		const float acrossX = shown.width / p.frame.gameWidth();
		const float acrossY = shown.height / p.frame.gameHeight();
		const Vector2f size = object.pictureSize();
		const float x0 = shown.left + std::floor(object.position.x) * acrossX - 0.5f;
		const float y0 = shown.top + std::floor(object.position.y) * acrossY - 0.5f;
		const float x1 = x0 + size.x * acrossX;
		const float y1 = y0 + size.y * acrossY;
		const Corner corners[4] = {
			{ x0, y0, 0.0f, 1.0f, 0.0f, 0.0f },
			{ x1, y0, 0.0f, 1.0f, 1.0f, 0.0f },
			{ x0, y1, 0.0f, 1.0f, 0.0f, 1.0f },
			{ x1, y1, 0.0f, 1.0f, 1.0f, 1.0f },
		};

		const DWORD filter = object.drawnAtScale() ? D3DTEXF_LINEAR : D3DTEXF_POINT;
		p.device->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
		p.device->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);
		p.device->SetTexture(0, visual.picture.Get());
		p.device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, corners, sizeof(Corner));
	}

	void Direct3D9Window::display()
	{
		Parts& p = *parts;
		if (!p.drawing) { return; }
		p.device->EndScene();
		p.drawing = false;
		// A lost device (D3DERR_DEVICELOST) is found and reset by the next
		// clear().
		p.device->Present(nullptr, nullptr, nullptr, nullptr);
	}

	float Direct3D9Window::fit() const
	{
		return parts->frame.fit();
	}

	Bitmap Direct3D9Window::capture()
	{
		Parts& p = *parts;
		if (!p.drawing) { return {}; }
		ComPtr<IDirect3DSurface9> back;
		if (FAILED(p.device->GetRenderTarget(0, &back))) { return {}; }
		D3DSURFACE_DESC description{};
		back->GetDesc(&description);
		ComPtr<IDirect3DSurface9> copy;
		if (FAILED(p.device->CreateOffscreenPlainSurface(description.Width, description.Height, description.Format,
			D3DPOOL_SYSTEMMEM, &copy, nullptr))) { return {}; }

		// What was drawn is copied out of the scene, which then goes on.
		p.device->EndScene();
		const bool copied = SUCCEEDED(p.device->GetRenderTargetData(back.Get(), copy.Get()));
		p.drawing = SUCCEEDED(p.device->BeginScene());
		D3DLOCKED_RECT locked{};
		if (!copied || FAILED(copy->LockRect(&locked, nullptr, D3DLOCK_READONLY))) { return {}; }

		// Blue, green, red and an unused byte (the screen's X8R8G8B8, or
		// A8R8G8B8) to red, green, blue, alpha.
		Bitmap bitmap;
		bitmap.width = static_cast<int>(description.Width);
		bitmap.height = static_cast<int>(description.Height);
		const std::size_t wide = static_cast<std::size_t>(bitmap.width);
		bitmap.rgba.resize(wide * 4 * static_cast<std::size_t>(bitmap.height));
		for (std::size_t y = 0; y < static_cast<std::size_t>(bitmap.height); ++y)
		{
			const std::uint8_t* from = static_cast<const std::uint8_t*>(locked.pBits) + y * static_cast<std::size_t>(locked.Pitch);
			std::uint8_t* to = bitmap.rgba.data() + y * wide * 4;
			for (std::size_t x = 0; x < wide; ++x)
			{
				to[x * 4 + 0] = from[x * 4 + 2];
				to[x * 4 + 1] = from[x * 4 + 1];
				to[x * 4 + 2] = from[x * 4 + 0];
				to[x * 4 + 3] = 255;
			}
		}
		copy->UnlockRect();
		return bitmap;
	}
}
