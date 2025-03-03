#include "Overlay.h"

#include "Util/Assert.h"
#include "Util/Offsets.h"

#include "nixxes_dxgi.h"
#include "nixxes_d3d.h"

#include <Windows.h>
#include <detours.h>
#include <dxgi1_4.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace nx {
    INxD3D *INxD3D::Instance() {
        return *Offsets::ResolveID<"NxD3DImpl::Instance", NxD3DImpl **>();
    }

    INxDXGI *INxDXGI::Instance() {
        return *Offsets::ResolveID<"NxDXGIImpl::Instance", NxDXGIImpl **>();
    }
}

namespace Overlay {
    std::vector<ID3D12CommandAllocator *> CommandAllocators;
    ID3D12GraphicsCommandList *CommandList;
    ID3D12DescriptorHeap *SrvDescriptorHeap;
    ID3D12DescriptorHeap *RtvDescriptorHeap;

    static void Initialize(nx::NxDXGIImpl *, nx::NxD3DImpl *);

    static void Render();

    static void Present(nx::NxDXGIImpl *, nx::NxD3DImpl *);
}

static WNDPROC WndProc;

static LRESULT APIENTRY WndProc_Hook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;
    return CallWindowProcW(WndProc, hwnd, uMsg, wParam, lParam);
}

static bool (*NxDXGIImpl_Present)(nx::NxDXGIImpl *, void *);

static bool NxDXGIImpl_Present_Hook(nx::NxDXGIImpl *inDXGI, void *inData) {
    static bool initialized = [&]() {
        Overlay::Initialize(inDXGI, reinterpret_cast<nx::NxD3DImpl *>(nx::INxD3D::Instance()));
        return true;
    }();

    if (initialized) {
        Overlay::Render();
        Overlay::Present(inDXGI, reinterpret_cast<nx::NxD3DImpl *>(nx::INxD3D::Instance()));
    }

    return NxDXGIImpl_Present(inDXGI, inData);
}

void Overlay::Initialize(nx::NxDXGIImpl *inDXGI, nx::NxD3DImpl *inD3D) {
    ID3D12Device *device = nullptr;
    inDXGI->SwapChain->GetDevice(IID_PPV_ARGS(&device));

    HWND window = nullptr;
    inDXGI->SwapChain->GetHwnd(&window);

    HRESULT hr = S_OK;

    const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 16,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };

    hr |= device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&SrvDescriptorHeap));

    const D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = 8,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    hr |= device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RtvDescriptorHeap));

    CommandAllocators.resize(inDXGI->NumBuffers);

    ID3D12Resource *tempBackBuffer = nullptr;
    hr |= inDXGI->SwapChain->GetBuffer(0, IID_PPV_ARGS(&tempBackBuffer));
    D3D12_RESOURCE_DESC backBufferDesc = tempBackBuffer->GetDesc();
    tempBackBuffer->Release();

    for (auto &CommandAllocator: CommandAllocators)
        hr |= device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));

    hr |= device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], nullptr, IID_PPV_ARGS(&CommandList));

    if (FAILED(hr))
        __debugbreak();

    CommandList->Close();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto &io = ImGui::GetIO();
    io.WantCaptureMouse = true;
    io.WantCaptureKeyboard = true;

    ImGui_ImplDX12_InitInfo info;
    info.Device = device;
    info.CommandQueue = inD3D->GetCommandQueue(0);
    info.NumFramesInFlight = static_cast<int>(inDXGI->NumBuffers);
    info.RTVFormat = backBufferDesc.Format;
    info.SrvDescriptorHeap = SrvDescriptorHeap;
    info.LegacySingleSrvCpuDescriptor = SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    info.LegacySingleSrvGpuDescriptor = SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX12_Init(&info);
    WndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc_Hook)));
}

void Overlay::Render() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool showing = true;
    if (showing)
        ImGui::ShowDemoWindow(&showing);

    ImGui::Render();
}

void Overlay::Present(nx::NxDXGIImpl *inDXGI, nx::NxD3DImpl *inD3D) {
    ImDrawData *drawData = ImGui::GetDrawData();
    if (!drawData->Valid || drawData->CmdListsCount == 0)
        return;

    const auto bufferIndex = inDXGI->SwapChain->GetCurrentBackBufferIndex();
    const auto buffer = inD3D->GetBackBuffer(bufferIndex);

    // Create a brand new RTV each frame. Tracking this in engine code is too difficult.
    const auto &rtvHandle = RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    inD3D->GetDevice()->CreateRenderTargetView(buffer, nullptr, rtvHandle);

    // Reset command allocator, command list, then draw
    auto allocator = CommandAllocators[bufferIndex];
    allocator->Reset();

    D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = buffer,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        },
    };

    CommandList->Reset(allocator, nullptr);
    CommandList->ResourceBarrier(1, &barrier);

    CommandList->SetDescriptorHeaps(1, &SrvDescriptorHeap);
    CommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    ImGui_ImplDX12_RenderDrawData(drawData, CommandList);

    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    CommandList->ResourceBarrier(1, &barrier);
    CommandList->Close();

    inD3D->GetCommandQueue(0)->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList *const *>(&CommandList));
}

void Overlay::Attach() {
    // @formatter:off
    Offsets::MapSignature("NxDXGIImpl::Present", "40 55 56 57 41 54 41 56 48 8D AC 24 00 FD FF FF 48 81 EC 00 04 00 00 48 8B F1 33 FF 48");
    Offsets::MapAddress("NxD3DImpl::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 48 8B 01 FF 90 B8 00 00 00 84 C0 75 13 48 8B 0D", 3));
    Offsets::MapAddress("NxDXGIImpl::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 48 8B 01 FF 50 08 0F B6 D8 84 C0 74 64 48 8B 0D", 3));
    // @formatter:on

    NxDXGIImpl_Present = Offsets::ResolveID<"NxDXGIImpl::Present", decltype(NxDXGIImpl_Present)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&NxDXGIImpl_Present), static_cast<PVOID>(NxDXGIImpl_Present_Hook));
    DetourTransactionCommit();
}
