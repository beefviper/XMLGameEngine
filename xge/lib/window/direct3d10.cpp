// direct3d10.cpp
// XML Game Engine
// author: beefviper
// date: Nov 15, 2026

#include "window/direct3d10.h"

#include "picture/color.h"
#include "window/win32.h"

#include <d3d10.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace xge
{
	namespace
	{
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
		// as it is (straight alpha, blended over what is there). The same
		// shaders as Direct3D 11's: shader model 4 is Direct3D 10's own.
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

		ComPtr<ID3DBlob> compile(const char* entry, const char* target)
		{
			ComPtr<ID3DBlob> code;
			ComPtr<ID3DBlob> errors;
			if (FAILED(D3DCompile(kShaders, sizeof(kShaders) - 1, "xge", nullptr, nullptr, entry, target, 0, 0, &code, &errors)))
			{
				std::string why = "Direct3D 10 could not compile its shaders";
				if (errors) { why += ": " + std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize()); }
				throw std::runtime_error(why);
			}
			return code;
		}
	}

	// Direct3D 10 has no device context: the device itself sets the pipeline
	// and draws, and buffers and textures map themselves.
	struct Direct3D10Window::Parts
	{
		// What an object shows: its picture on the graphics card, which
		// objects showing the same picture share.
		struct Visual
		{
			ComPtr<ID3D10ShaderResourceView> picture;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		Parts(Window& owner, const WindowDesc& windowDesc) : frame(windowDesc, "Direct3D 10"), textures(owner) {}

		Win32Window frame;

		ComPtr<ID3D10Device> device;
		ComPtr<IDXGISwapChain> swapChain;
		ComPtr<ID3D10RenderTargetView> target;
		ComPtr<ID3D10VertexShader> vertexShader;
		ComPtr<ID3D10PixelShader> pixelShader;
		ComPtr<ID3D10InputLayout> layout;
		ComPtr<ID3D10Buffer> corners;
		ComPtr<ID3D10Buffer> gameSize;
		ComPtr<ID3D10SamplerState> hardEdges;
		ComPtr<ID3D10SamplerState> smooth;
		ComPtr<ID3D10BlendState> blend;
		ComPtr<ID3D10RasterizerState> rasterizer;

		Window::Textures<ComPtr<ID3D10ShaderResourceView>> textures;
		std::unordered_map<std::string, Visual> visuals;  // by object name

		void makeTarget()
		{
			ComPtr<ID3D10Texture2D> back;
			if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&back))) || FAILED(device->CreateRenderTargetView(back.Get(), nullptr, &target)))
			{
				throw std::runtime_error("Direct3D 10 could not draw into its window");
			}
		}

		// After a resize: the window's pictures made again at its new size.
		void resize()
		{
			device->OMSetRenderTargets(0, nullptr, nullptr);
			target.Reset();
			if (FAILED(swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0)))
			{
				throw std::runtime_error("Direct3D 10 could not resize its window");
			}
			makeTarget();
		}

		// Direct3D 10's viewport is in whole pixels.
		D3D10_VIEWPORT viewport() const
		{
			const Win32Window::Shown& shown = frame.shown();
			D3D10_VIEWPORT port{};
			port.TopLeftX = static_cast<INT>(std::lround(shown.left));
			port.TopLeftY = static_cast<INT>(std::lround(shown.top));
			port.Width = static_cast<UINT>(std::max(1L, std::lround(shown.width)));
			port.Height = static_cast<UINT>(std::max(1L, std::lround(shown.height)));
			port.MinDepth = 0.0f;
			port.MaxDepth = 1.0f;
			return port;
		}

		void build(Object& object, float scale, PictureId showing)
		{
			object.drawn.visualDirty = false;
			Visual& visual = visuals[object.name];
			visual.drawnAt = scale;
			visual.picture = textures.of(showing, [this](const Bitmap& picture, ComPtr<ID3D10ShaderResourceView>& made) { made = cardPicture(picture); });
		}

		// A picture on the graphics card; none for an empty one.
		ComPtr<ID3D10ShaderResourceView> cardPicture(const Bitmap& picture)
		{
			if (picture.rgba.empty() || picture.width <= 0 || picture.height <= 0) { return {}; }

			D3D10_TEXTURE2D_DESC description{};
			description.Width = static_cast<UINT>(picture.width);
			description.Height = static_cast<UINT>(picture.height);
			description.MipLevels = 1;
			description.ArraySize = 1;
			description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.Usage = D3D10_USAGE_IMMUTABLE;
			description.BindFlags = D3D10_BIND_SHADER_RESOURCE;
			D3D10_SUBRESOURCE_DATA pixels{};
			pixels.pSysMem = picture.rgba.data();
			pixels.SysMemPitch = static_cast<UINT>(picture.width) * 4;

			ComPtr<ID3D10Texture2D> texture;
			ComPtr<ID3D10ShaderResourceView> view;
			if (FAILED(device->CreateTexture2D(&description, &pixels, &texture))) { return {}; }
			if (FAILED(device->CreateShaderResourceView(texture.Get(), nullptr, &view))) { return {}; }
			return view;
		}
	};

	Direct3D10Window::Direct3D10Window(const WindowDesc& windowDesc) :
		parts(std::make_unique<Parts>(*this, windowDesc))
	{
		Parts& p = *parts;
		DXGI_SWAP_CHAIN_DESC chain{};
		chain.BufferCount = 1;
		chain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		chain.OutputWindow = p.frame.handle();
		chain.SampleDesc.Count = 1;
		chain.Windowed = TRUE;
		chain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		if (FAILED(D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION, &chain, &p.swapChain, &p.device)))
		{
			throw std::runtime_error("Direct3D 10 could not start on this graphics card");
		}
		p.makeTarget();

		const ComPtr<ID3DBlob> vertexCode = compile("vs", "vs_4_0");
		const ComPtr<ID3DBlob> pixelCode = compile("ps", "ps_4_0");
		p.device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), &p.vertexShader);
		p.device->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), &p.pixelShader);
		const D3D10_INPUT_ELEMENT_DESC corner[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		};
		p.device->CreateInputLayout(corner, 2, vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), &p.layout);

		D3D10_BUFFER_DESC corners{};
		corners.ByteWidth = sizeof(Corner) * 4;
		corners.Usage = D3D10_USAGE_DYNAMIC;
		corners.BindFlags = D3D10_BIND_VERTEX_BUFFER;
		corners.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		p.device->CreateBuffer(&corners, nullptr, &p.corners);

		const Frame size{ p.frame.gameWidth(), p.frame.gameHeight(), { 0.0f, 0.0f } };
		D3D10_BUFFER_DESC frameBuffer{};
		frameBuffer.ByteWidth = sizeof(Frame);
		frameBuffer.Usage = D3D10_USAGE_IMMUTABLE;
		frameBuffer.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
		D3D10_SUBRESOURCE_DATA sizeData{};
		sizeData.pSysMem = &size;
		p.device->CreateBuffer(&frameBuffer, &sizeData, &p.gameSize);

		D3D10_SAMPLER_DESC sampler{};
		sampler.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
		sampler.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
		sampler.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
		sampler.MaxLOD = D3D10_FLOAT32_MAX;
		sampler.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
		p.device->CreateSamplerState(&sampler, &p.hardEdges);
		sampler.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
		p.device->CreateSamplerState(&sampler, &p.smooth);

		// Direct3D 10 blends every target the same way, so one switch.
		D3D10_BLEND_DESC blend{};
		blend.BlendEnable[0] = TRUE;
		blend.SrcBlend = D3D10_BLEND_SRC_ALPHA;
		blend.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
		blend.BlendOp = D3D10_BLEND_OP_ADD;
		blend.SrcBlendAlpha = D3D10_BLEND_ONE;
		blend.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
		blend.BlendOpAlpha = D3D10_BLEND_OP_ADD;
		blend.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
		p.device->CreateBlendState(&blend, &p.blend);

		// Rectangles are drawn whichever way round their corners come.
		D3D10_RASTERIZER_DESC rasterizer{};
		rasterizer.FillMode = D3D10_FILL_SOLID;
		rasterizer.CullMode = D3D10_CULL_NONE;
		rasterizer.DepthClipEnable = TRUE;
		p.device->CreateRasterizerState(&rasterizer, &p.rasterizer);

		if (!p.vertexShader || !p.pixelShader || !p.layout || !p.corners || !p.gameSize || !p.hardEdges || !p.smooth || !p.blend || !p.rasterizer)
		{
			throw std::runtime_error("Direct3D 10 could not set up its drawing");
		}

		setPictureScale(p.frame.fit());
		p.frame.show();
	}

	Direct3D10Window::~Direct3D10Window()
	{
		close();
	}

	bool Direct3D10Window::isOpen() const
	{
		return parts->frame.isOpen();
	}

	void Direct3D10Window::close()
	{
		Parts& p = *parts;
		if (!p.frame.handle()) { return; }
		p.visuals.clear();
		p.textures.clear();
		if (p.device) { p.device->ClearState(); }
		p.frame.destroy();
	}

	std::pair<int, int> Direct3D10Window::position() const
	{
		return parts->frame.position();
	}

	void Direct3D10Window::setPosition(int x, int y)
	{
		parts->frame.setPosition(x, y);
	}

	void Direct3D10Window::setTitle(const std::string& title)
	{
		parts->frame.setTitle(title);
	}

	void Direct3D10Window::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			parts->build(object, pictureScale(), pictureOf(object));
		}
	}

	std::vector<std::pair<KeyCode, bool>> Direct3D10Window::pollEvents()
	{
		return parts->frame.pollEvents();
	}

	Controls Direct3D10Window::controls()
	{
		return parts->frame.controls();
	}

	void Direct3D10Window::clear(const std::string& colorName)
	{
		Parts& p = *parts;
		if (p.frame.takeResized()) { p.resize(); }

		// The whole window in the background's colour, bars included.
		const Color c = colorFromName(colorName);
		const float background[4] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f };
		p.device->ClearRenderTargetView(p.target.Get(), background);

		ID3D10RenderTargetView* targets[] = { p.target.Get() };
		p.device->OMSetRenderTargets(1, targets, nullptr);
		const D3D10_VIEWPORT port = p.viewport();
		p.device->RSSetViewports(1, &port);
		p.device->RSSetState(p.rasterizer.Get());
		const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		p.device->OMSetBlendState(p.blend.Get(), blendFactor, 0xFFFFFFFF);
		p.device->IASetInputLayout(p.layout.Get());
		p.device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		const UINT stride = sizeof(Corner);
		const UINT offset = 0;
		ID3D10Buffer* vertices[] = { p.corners.Get() };
		p.device->IASetVertexBuffers(0, 1, vertices, &stride, &offset);
		ID3D10Buffer* constants[] = { p.gameSize.Get() };
		p.device->VSSetConstantBuffers(0, 1, constants);
		p.device->VSSetShader(p.vertexShader.Get());
		p.device->PSSetShader(p.pixelShader.Get());
	}

	void Direct3D10Window::prepare(Object& object)
	{
		Parts& p = *parts;
		const auto found = p.visuals.find(object.name);
		if (object.drawn.visualDirty || found == p.visuals.end() || (object.drawnAtScale() && found->second.drawnAt != pictureScale()))
		{
			p.build(object, pictureScale(), pictureOf(object));
		}
	}

	void Direct3D10Window::draw(Object& object)
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
		void* mapped = nullptr;
		if (FAILED(p.corners->Map(D3D10_MAP_WRITE_DISCARD, 0, &mapped))) { return; }
		std::memcpy(mapped, corners, sizeof(corners));
		p.corners->Unmap();

		ID3D10ShaderResourceView* pictures[] = { visual.picture.Get() };
		p.device->PSSetShaderResources(0, 1, pictures);
		ID3D10SamplerState* samplers[] = { object.drawnAtScale() ? p.smooth.Get() : p.hardEdges.Get() };
		p.device->PSSetSamplers(0, 1, samplers);
		p.device->Draw(4, 0);
	}

	void Direct3D10Window::display()
	{
		// Waits for the screen's refresh, so a picture is never torn; the
		// engine keeps the game's own pace (Engine::loop).
		parts->swapChain->Present(1, 0);
	}

	float Direct3D10Window::fit() const
	{
		return parts->frame.fit();
	}

	Bitmap Direct3D10Window::capture()
	{
		Parts& p = *parts;
		ComPtr<ID3D10Texture2D> back;
		if (FAILED(p.swapChain->GetBuffer(0, IID_PPV_ARGS(&back)))) { return {}; }
		D3D10_TEXTURE2D_DESC description{};
		back->GetDesc(&description);
		description.Usage = D3D10_USAGE_STAGING;
		description.BindFlags = 0;
		description.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
		description.MiscFlags = 0;
		ComPtr<ID3D10Texture2D> copy;
		if (FAILED(p.device->CreateTexture2D(&description, nullptr, &copy))) { return {}; }
		p.device->CopyResource(copy.Get(), back.Get());

		D3D10_MAPPED_TEXTURE2D mapped{};
		if (FAILED(copy->Map(0, D3D10_MAP_READ, 0, &mapped))) { return {}; }
		Bitmap bitmap;
		bitmap.width = static_cast<int>(description.Width);
		bitmap.height = static_cast<int>(description.Height);
		const std::size_t row = static_cast<std::size_t>(bitmap.width) * 4;
		bitmap.rgba.resize(row * static_cast<std::size_t>(bitmap.height));
		for (std::size_t y = 0; y < static_cast<std::size_t>(bitmap.height); ++y)
		{
			std::memcpy(bitmap.rgba.data() + y * row, static_cast<const std::uint8_t*>(mapped.pData) + y * mapped.RowPitch, row);
		}
		copy->Unmap(0);
		return bitmap;
	}
}
