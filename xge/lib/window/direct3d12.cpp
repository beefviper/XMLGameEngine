// direct3d12.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/direct3d12.h"

#include "picture/color.h"
#include "window/win32.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace xge
{
	namespace
	{
		// The back buffers the window swaps between.
		constexpr UINT kBuffers = 2;
		// The most pictures shown at once (a slot each in the one heap of
		// them the shaders read).
		constexpr UINT kPictureSlots = 8192;

		// What the vertex shader is given for each rectangle (root constants):
		// where it is in the game's pixels, the game's size, and whether the
		// picture is sampled smoothly.
		struct Place
		{
			float left;
			float top;
			float right;
			float bottom;
			float gameWidth;
			float gameHeight;
			float soft;
			float unused;
		};

		// One rectangle from four corners and no vertex buffer, turned into
		// Direct3D's -1 to 1; the picture as it is (straight alpha, blended
		// over what is there), hard-edged or smooth.
		constexpr char kShaders[] = R"(
cbuffer Place : register(b0) { float2 topLeft; float2 bottomRight; float2 gameSize; float soft; float unused; };
struct Shaded { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
Shaded vs(uint corner : SV_VertexID)
{
	Shaded shaded;
	shaded.uv = float2(corner & 1, corner >> 1);
	float2 at = lerp(topLeft, bottomRight, shaded.uv) / gameSize;
	shaded.position = float4(at.x * 2.0 - 1.0, 1.0 - at.y * 2.0, 0.0, 1.0);
	return shaded;
}
Texture2D picture : register(t0);
SamplerState hardEdges : register(s0);
SamplerState smoothly : register(s1);
float4 ps(Shaded shaded) : SV_TARGET
{
	return soft > 0.5 ? picture.Sample(smoothly, shaded.uv) : picture.Sample(hardEdges, shaded.uv);
}
)";

		ComPtr<ID3DBlob> compile(const char* entry, const char* target)
		{
			ComPtr<ID3DBlob> code;
			ComPtr<ID3DBlob> errors;
			if (FAILED(D3DCompile(kShaders, sizeof(kShaders) - 1, "xge", nullptr, nullptr, entry, target, 0, 0, &code, &errors)))
			{
				std::string why = "Direct3D 12 could not compile its shaders";
				if (errors) { why += ": " + std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize()); }
				throw std::runtime_error(why);
			}
			return code;
		}

		void check(HRESULT result, const char* what)
		{
			if (FAILED(result)) { throw std::runtime_error(std::string("Direct3D 12 could not ") + what); }
		}

		D3D12_RESOURCE_BARRIER change(ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = resource;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = from;
			barrier.Transition.StateAfter = to;
			return barrier;
		}

		// A buffer in memory of the given kind (upload, to write; readback, to read).
		ComPtr<ID3D12Resource> buffer(ID3D12Device* device, D3D12_HEAP_TYPE kind, UINT64 bytes, D3D12_RESOURCE_STATES state)
		{
			D3D12_HEAP_PROPERTIES heap{};
			heap.Type = kind;
			D3D12_RESOURCE_DESC description{};
			description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			description.Width = bytes;
			description.Height = 1;
			description.DepthOrArraySize = 1;
			description.MipLevels = 1;
			description.Format = DXGI_FORMAT_UNKNOWN;
			description.SampleDesc.Count = 1;
			description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			ComPtr<ID3D12Resource> made;
			check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description, state, nullptr, IID_PPV_ARGS(&made)), "make a buffer");
			return made;
		}
	}

	struct Direct3D12Window::Parts
	{
		// One object's picture on the graphics card, and its slot in the heap
		// the pixel shader reads.
		struct Picture
		{
			ComPtr<ID3D12Resource> texture;
			UINT slot{ kPictureSlots };   // none
		};

		struct Visual
		{
			Picture picture;
			bool soft{ false };
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		// Where a frame is: nothing yet, being recorded, or handed to the
		// graphics card (capture() does that early, to read it back).
		enum class Stage { Idle, Recording, Submitted };

		explicit Parts(const WindowDesc& windowDesc) : frame(windowDesc, "Direct3D 12") {}

		~Parts()
		{
			if (queue && fence && done) { waitIdle(); }
			if (done) { CloseHandle(done); }
		}

		Win32Window frame;

		ComPtr<ID3D12Device> device;
		ComPtr<ID3D12CommandQueue> queue;
		ComPtr<IDXGISwapChain3> swapChain;
		ComPtr<ID3D12DescriptorHeap> targetHeap;
		UINT targetStep{ 0 };
		ComPtr<ID3D12Resource> backBuffers[kBuffers];
		ComPtr<ID3D12DescriptorHeap> pictureHeap;
		UINT pictureStep{ 0 };
		std::vector<UINT> freeSlots;
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipeline;
		ComPtr<ID3D12CommandAllocator> frameAllocator;
		ComPtr<ID3D12GraphicsCommandList> frameList;
		ComPtr<ID3D12CommandAllocator> uploadAllocator;
		ComPtr<ID3D12GraphicsCommandList> uploadList;
		ComPtr<ID3D12Fence> fence;
		UINT64 fenceValue{ 0 };
		UINT64 frameDone{ 0 };   // the fence value the last frame signals
		HANDLE done{ nullptr };
		UINT shownBuffer{ 0 };
		Stage stage{ Stage::Idle };

		std::unordered_map<std::string, Visual> visuals;
		// Pictures replaced while a frame may still draw them: let go once it
		// is done.
		std::vector<Picture> retired;

		// Waits until the graphics card has passed `value`.
		void waitFor(UINT64 value)
		{
			if (fence->GetCompletedValue() >= value) { return; }
			fence->SetEventOnCompletion(value, done);
			WaitForSingleObject(done, INFINITE);
		}

		UINT64 signal()
		{
			++fenceValue;
			queue->Signal(fence.Get(), fenceValue);
			return fenceValue;
		}

		void waitIdle() { waitFor(signal()); }

		void letGo(Picture& picture)
		{
			if (picture.slot != kPictureSlots) { freeSlots.push_back(picture.slot); }
			picture = {};
		}

		void makeTargets()
		{
			D3D12_CPU_DESCRIPTOR_HANDLE target = targetHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT index = 0; index < kBuffers; ++index)
			{
				check(swapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffers[index])), "draw into its window");
				device->CreateRenderTargetView(backBuffers[index].Get(), nullptr, target);
				target.ptr += targetStep;
			}
		}

		// After a resize: the window's buffers made again at its new size.
		void resize()
		{
			waitIdle();
			for (auto& each : backBuffers) { each.Reset(); }
			check(swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0), "resize its window");
			makeTargets();
		}

		D3D12_CPU_DESCRIPTOR_HANDLE targetOf(UINT index) const
		{
			D3D12_CPU_DESCRIPTOR_HANDLE target = targetHeap->GetCPUDescriptorHandleForHeapStart();
			target.ptr += static_cast<SIZE_T>(index) * targetStep;
			return target;
		}

		// The engine's picture of the object, at `scale`, copied to the
		// graphics card through an upload buffer.
		void build(Object& object, float scale)
		{
			object.visualDirty = false;
			Visual& visual = visuals[object.name];
			visual.drawnAt = scale;
			visual.soft = object.drawnAtScale();
			if (visual.picture.texture) { retired.push_back(visual.picture); }
			visual.picture = {};

			const std::shared_ptr<const Bitmap> source = object.drawnPicture(scale);
			if (source->rgba.empty() || source->width <= 0 || source->height <= 0) { return; }
			if (freeSlots.empty()) { throw std::runtime_error("Direct3D 12 has no room for another picture"); }

			D3D12_HEAP_PROPERTIES onCard{};
			onCard.Type = D3D12_HEAP_TYPE_DEFAULT;
			D3D12_RESOURCE_DESC description{};
			description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			description.Width = static_cast<UINT64>(source->width);
			description.Height = static_cast<UINT>(source->height);
			description.DepthOrArraySize = 1;
			description.MipLevels = 1;
			description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			Picture picture;
			check(device->CreateCommittedResource(&onCard, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr, IID_PPV_ARGS(&picture.texture)), "make a picture");

			// The upload buffer's rows are as wide as Direct3D 12 wants them.
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
			UINT rows = 0;
			UINT64 rowBytes = 0;
			UINT64 total = 0;
			device->GetCopyableFootprints(&description, 0, 1, 0, &layout, &rows, &rowBytes, &total);
			const ComPtr<ID3D12Resource> upload = buffer(device.Get(), D3D12_HEAP_TYPE_UPLOAD, total, D3D12_RESOURCE_STATE_GENERIC_READ);
			void* mapped = nullptr;
			const D3D12_RANGE nothingRead{ 0, 0 };
			check(upload->Map(0, &nothingRead, &mapped), "copy a picture");
			std::uint8_t* written = static_cast<std::uint8_t*>(mapped);
			const std::size_t row = static_cast<std::size_t>(source->width) * 4;
			for (std::size_t y = 0; y < static_cast<std::size_t>(source->height); ++y)
			{
				std::memcpy(written + layout.Offset + y * layout.Footprint.RowPitch, source->rgba.data() + y * row, row);
			}
			upload->Unmap(0, nullptr);

			check(uploadAllocator->Reset(), "copy a picture");
			check(uploadList->Reset(uploadAllocator.Get(), nullptr), "copy a picture");
			D3D12_TEXTURE_COPY_LOCATION to{};
			to.pResource = picture.texture.Get();
			to.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			to.SubresourceIndex = 0;
			D3D12_TEXTURE_COPY_LOCATION from{};
			from.pResource = upload.Get();
			from.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			from.PlacedFootprint = layout;
			uploadList->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
			const D3D12_RESOURCE_BARRIER ready = change(picture.texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			uploadList->ResourceBarrier(1, &ready);
			check(uploadList->Close(), "copy a picture");
			ID3D12CommandList* lists[] = { uploadList.Get() };
			queue->ExecuteCommandLists(1, lists);
			waitIdle();   // the upload buffer goes when this returns

			picture.slot = freeSlots.back();
			freeSlots.pop_back();
			D3D12_CPU_DESCRIPTOR_HANDLE slot = pictureHeap->GetCPUDescriptorHandleForHeapStart();
			slot.ptr += static_cast<SIZE_T>(picture.slot) * pictureStep;
			device->CreateShaderResourceView(picture.texture.Get(), nullptr, slot);
			visual.picture = picture;
		}

		// Ends the frame's drawing and hands it to the graphics card; with
		// `readBack`, the picture is copied into it too, as `layout` says.
		void submit(ID3D12Resource* readBack, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout)
		{
			ID3D12Resource* back = backBuffers[shownBuffer].Get();
			if (readBack)
			{
				const D3D12_RESOURCE_BARRIER toCopy = change(back, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
				frameList->ResourceBarrier(1, &toCopy);
				D3D12_TEXTURE_COPY_LOCATION to{};
				to.pResource = readBack;
				to.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				to.PlacedFootprint = layout;
				D3D12_TEXTURE_COPY_LOCATION from{};
				from.pResource = back;
				from.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				from.SubresourceIndex = 0;
				frameList->CopyTextureRegion(&to, 0, 0, 0, &from, nullptr);
				const D3D12_RESOURCE_BARRIER toShow = change(back, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
				frameList->ResourceBarrier(1, &toShow);
			}
			else
			{
				const D3D12_RESOURCE_BARRIER toShow = change(back, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
				frameList->ResourceBarrier(1, &toShow);
			}
			check(frameList->Close(), "draw a frame");
			ID3D12CommandList* lists[] = { frameList.Get() };
			queue->ExecuteCommandLists(1, lists);
			frameDone = signal();
			stage = Stage::Submitted;
		}
	};

	Direct3D12Window::Direct3D12Window(const WindowDesc& windowDesc) :
		parts(std::make_unique<Parts>(windowDesc))
	{
		Parts& p = *parts;

#ifndef NDEBUG
		// A Debug build checks every call when Windows has the debug layer
		// (the optional feature "Graphics Tools"); what it finds goes to the
		// debugger's output window.
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) { debug->EnableDebugLayer(); }
#endif

		ComPtr<IDXGIFactory4> factory;
		check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "start (no DXGI)");
		if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&p.device))))
		{
			throw std::runtime_error("Direct3D 12 could not start on this graphics card");
		}

		D3D12_COMMAND_QUEUE_DESC queueDescription{};
		queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		check(p.device->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&p.queue)), "start a command queue");

		// Two buffers flipped on the screen's refresh, so a picture is never
		// torn; the engine keeps the game's own pace (Engine::loop).
		DXGI_SWAP_CHAIN_DESC1 chain{};
		chain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		chain.SampleDesc.Count = 1;
		chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		chain.BufferCount = kBuffers;
		chain.Scaling = DXGI_SCALING_STRETCH;
		chain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		ComPtr<IDXGISwapChain1> made;
		check(factory->CreateSwapChainForHwnd(p.queue.Get(), p.frame.handle(), &chain, nullptr, nullptr, &made), "draw into its window");
		check(made.As(&p.swapChain), "draw into its window");
		factory->MakeWindowAssociation(p.frame.handle(), DXGI_MWA_NO_ALT_ENTER);   // the game says what Alt+Enter does

		D3D12_DESCRIPTOR_HEAP_DESC targets{};
		targets.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		targets.NumDescriptors = kBuffers;
		check(p.device->CreateDescriptorHeap(&targets, IID_PPV_ARGS(&p.targetHeap)), "draw into its window");
		p.targetStep = p.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		p.makeTargets();

		D3D12_DESCRIPTOR_HEAP_DESC pictures{};
		pictures.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		pictures.NumDescriptors = kPictureSlots;
		pictures.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		check(p.device->CreateDescriptorHeap(&pictures, IID_PPV_ARGS(&p.pictureHeap)), "make room for its pictures");
		p.pictureStep = p.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		p.freeSlots.reserve(kPictureSlots);
		for (UINT slot = kPictureSlots; slot > 0; --slot) { p.freeSlots.push_back(slot - 1); }

		// What the shaders are given: the rectangle's numbers, its picture,
		// and the two ways of sampling it.
		D3D12_DESCRIPTOR_RANGE pictureRange{};
		pictureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		pictureRange.NumDescriptors = 1;
		pictureRange.BaseShaderRegister = 0;
		pictureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		D3D12_ROOT_PARAMETER given[2]{};
		given[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		given[0].Constants.ShaderRegister = 0;
		given[0].Constants.Num32BitValues = sizeof(Place) / 4;
		given[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		given[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		given[1].DescriptorTable.NumDescriptorRanges = 1;
		given[1].DescriptorTable.pDescriptorRanges = &pictureRange;
		given[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[2]{};
		for (UINT index = 0; index < 2; ++index)
		{
			samplers[index].Filter = index == 0 ? D3D12_FILTER_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			samplers[index].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplers[index].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplers[index].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplers[index].MaxLOD = D3D12_FLOAT32_MAX;
			samplers[index].ShaderRegister = index;
			samplers[index].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		}
		D3D12_ROOT_SIGNATURE_DESC signature{};
		signature.NumParameters = 2;
		signature.pParameters = given;
		signature.NumStaticSamplers = 2;
		signature.pStaticSamplers = samplers;
		ComPtr<ID3DBlob> serialized;
		ComPtr<ID3DBlob> errors;
		check(D3D12SerializeRootSignature(&signature, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors), "set up its drawing");
		check(p.device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(), IID_PPV_ARGS(&p.rootSignature)),
			"set up its drawing");

		const ComPtr<ID3DBlob> vertexCode = compile("vs", "vs_5_0");
		const ComPtr<ID3DBlob> pixelCode = compile("ps", "ps_5_0");
		D3D12_GRAPHICS_PIPELINE_STATE_DESC drawing{};
		drawing.pRootSignature = p.rootSignature.Get();
		drawing.VS = { vertexCode->GetBufferPointer(), vertexCode->GetBufferSize() };
		drawing.PS = { pixelCode->GetBufferPointer(), pixelCode->GetBufferSize() };
		D3D12_RENDER_TARGET_BLEND_DESC& over = drawing.BlendState.RenderTarget[0];
		over.BlendEnable = TRUE;
		over.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		over.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		over.BlendOp = D3D12_BLEND_OP_ADD;
		over.SrcBlendAlpha = D3D12_BLEND_ONE;
		over.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		over.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		over.LogicOp = D3D12_LOGIC_OP_NOOP;
		over.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		drawing.SampleMask = UINT_MAX;
		// Rectangles are drawn whichever way round their corners come.
		drawing.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		drawing.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		drawing.RasterizerState.DepthClipEnable = TRUE;
		drawing.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		drawing.NumRenderTargets = 1;
		drawing.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		drawing.SampleDesc.Count = 1;
		check(p.device->CreateGraphicsPipelineState(&drawing, IID_PPV_ARGS(&p.pipeline)), "set up its drawing");

		check(p.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&p.frameAllocator)), "set up its drawing");
		check(p.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, p.frameAllocator.Get(), nullptr, IID_PPV_ARGS(&p.frameList)),
			"set up its drawing");
		p.frameList->Close();
		check(p.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&p.uploadAllocator)), "set up its drawing");
		check(p.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, p.uploadAllocator.Get(), nullptr, IID_PPV_ARGS(&p.uploadList)),
			"set up its drawing");
		p.uploadList->Close();
		check(p.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&p.fence)), "set up its drawing");
		p.done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (!p.done) { throw std::runtime_error("Direct3D 12 could not set up its drawing"); }

		setPictureScale(p.frame.fit());
		p.frame.show();
	}

	Direct3D12Window::~Direct3D12Window()
	{
		close();
	}

	bool Direct3D12Window::isOpen() const
	{
		return parts->frame.isOpen();
	}

	void Direct3D12Window::close()
	{
		Parts& p = *parts;
		if (!p.frame.handle()) { return; }
		p.waitIdle();
		p.visuals.clear();
		p.retired.clear();
		p.frame.destroy();
	}

	std::pair<int, int> Direct3D12Window::position() const
	{
		return parts->frame.position();
	}

	void Direct3D12Window::setPosition(int x, int y)
	{
		parts->frame.setPosition(x, y);
	}

	void Direct3D12Window::setTitle(const std::string& title)
	{
		parts->frame.setTitle(title);
	}

	void Direct3D12Window::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			parts->build(object, pictureScale());
		}
	}

	std::vector<std::pair<KeyCode, bool>> Direct3D12Window::pollEvents()
	{
		return parts->frame.pollEvents();
	}

	Controls Direct3D12Window::controls()
	{
		return parts->frame.controls();
	}

	void Direct3D12Window::clear(const std::string& colorName)
	{
		Parts& p = *parts;
		if (!p.frame.handle() || p.stage != Parts::Stage::Idle) { return; }

		// One frame at a time: the last one is done before this one starts,
		// and so are the pictures it drew.
		p.waitFor(p.frameDone);
		for (Parts::Picture& picture : p.retired) { p.letGo(picture); }
		p.retired.clear();
		if (p.frame.takeResized()) { p.resize(); }

		p.shownBuffer = p.swapChain->GetCurrentBackBufferIndex();
		check(p.frameAllocator->Reset(), "draw a frame");
		check(p.frameList->Reset(p.frameAllocator.Get(), p.pipeline.Get()), "draw a frame");
		const D3D12_RESOURCE_BARRIER toDraw = change(p.backBuffers[p.shownBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		p.frameList->ResourceBarrier(1, &toDraw);

		// The whole window in the background's colour, bars included.
		const D3D12_CPU_DESCRIPTOR_HANDLE target = p.targetOf(p.shownBuffer);
		p.frameList->OMSetRenderTargets(1, &target, FALSE, nullptr);
		const Color c = colorFromName(colorName);
		const float background[4] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f };
		p.frameList->ClearRenderTargetView(target, background, 0, nullptr);

		// The game where it is shown; nothing drawn over the bars.
		const Win32Window::Shown& shown = p.frame.shown();
		const D3D12_VIEWPORT port{ shown.left, shown.top, shown.width, shown.height, 0.0f, 1.0f };
		p.frameList->RSSetViewports(1, &port);
		const D3D12_RECT inside{ static_cast<LONG>(shown.left), static_cast<LONG>(shown.top),
			static_cast<LONG>(shown.left + shown.width), static_cast<LONG>(shown.top + shown.height) };
		p.frameList->RSSetScissorRects(1, &inside);
		p.frameList->SetGraphicsRootSignature(p.rootSignature.Get());
		ID3D12DescriptorHeap* heaps[] = { p.pictureHeap.Get() };
		p.frameList->SetDescriptorHeaps(1, heaps);
		p.frameList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		p.stage = Parts::Stage::Recording;
	}

	void Direct3D12Window::prepare(Object& object)
	{
		Parts& p = *parts;
		if (!p.frame.handle()) { return; }
		const auto found = p.visuals.find(object.name);
		if (object.visualDirty || found == p.visuals.end() || (object.drawnAtScale() && found->second.drawnAt != pictureScale()))
		{
			p.build(object, pictureScale());
		}
	}

	void Direct3D12Window::draw(Object& object)
	{
		Parts& p = *parts;
		if (p.stage != Parts::Stage::Recording) { return; }
		prepare(object);
		const Parts::Visual& visual = p.visuals[object.name];
		if (!visual.picture.texture) { return; }

		// Whole game pixels, so a picture is never sampled between two of
		// its pixels; stretched over the object's size.
		const Vector2f size = object.pictureSize();
		const float left = std::floor(object.position.x);
		const float top = std::floor(object.position.y);
		const Place place{ left, top, left + size.x, top + size.y, p.frame.gameWidth(), p.frame.gameHeight(),
			visual.soft ? 1.0f : 0.0f, 0.0f };
		p.frameList->SetGraphicsRoot32BitConstants(0, sizeof(Place) / 4, &place, 0);
		D3D12_GPU_DESCRIPTOR_HANDLE slot = p.pictureHeap->GetGPUDescriptorHandleForHeapStart();
		slot.ptr += static_cast<UINT64>(visual.picture.slot) * p.pictureStep;
		p.frameList->SetGraphicsRootDescriptorTable(1, slot);
		p.frameList->DrawInstanced(4, 1, 0, 0);
	}

	void Direct3D12Window::display()
	{
		Parts& p = *parts;
		if (p.stage == Parts::Stage::Recording) { p.submit(nullptr, {}); }
		if (p.stage != Parts::Stage::Submitted) { return; }
		p.stage = Parts::Stage::Idle;
		p.swapChain->Present(1, 0);
	}

	float Direct3D12Window::fit() const
	{
		return parts->frame.fit();
	}

	Bitmap Direct3D12Window::capture()
	{
		Parts& p = *parts;
		if (p.stage != Parts::Stage::Recording) { return {}; }

		// The frame is finished now and copied out; display() then shows it.
		const D3D12_RESOURCE_DESC description = p.backBuffers[p.shownBuffer]->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
		UINT rows = 0;
		UINT64 rowBytes = 0;
		UINT64 total = 0;
		p.device->GetCopyableFootprints(&description, 0, 1, 0, &layout, &rows, &rowBytes, &total);
		const ComPtr<ID3D12Resource> readBack = buffer(p.device.Get(), D3D12_HEAP_TYPE_READBACK, total, D3D12_RESOURCE_STATE_COPY_DEST);
		p.submit(readBack.Get(), layout);
		p.waitFor(p.frameDone);

		Bitmap bitmap;
		bitmap.width = static_cast<int>(description.Width);
		bitmap.height = static_cast<int>(description.Height);
		const std::size_t row = static_cast<std::size_t>(bitmap.width) * 4;
		bitmap.rgba.resize(row * static_cast<std::size_t>(bitmap.height));
		void* mapped = nullptr;
		const D3D12_RANGE everything{ 0, static_cast<SIZE_T>(total) };
		if (FAILED(readBack->Map(0, &everything, &mapped))) { return {}; }
		const std::uint8_t* read = static_cast<const std::uint8_t*>(mapped);
		for (std::size_t y = 0; y < static_cast<std::size_t>(bitmap.height); ++y)
		{
			std::memcpy(bitmap.rgba.data() + y * row, read + layout.Offset + y * layout.Footprint.RowPitch, row);
		}
		const D3D12_RANGE nothingWritten{ 0, 0 };
		readBack->Unmap(0, &nothingWritten);
		for (std::size_t at = 3; at < bitmap.rgba.size(); at += 4) { bitmap.rgba[at] = 255; }   // the window has no see-through
		return bitmap;
	}
}
