#include "Overlay.h"

#include "Util/Assert.h"
#include "Util/Offsets.h"
#include "Util/Typedefs.h"

#include "nixxes_dxgi.h"
#include "nixxes_d3d.h"

#include <detours.h>
#include <d3d12.h>
#include <dxgi1_4.h>

// #define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <vector>

namespace nx {
    INxD3D *INxD3D::Instance() {
        return *Offsets::ResolveID<"NxD3DImpl::Instance", NxD3DImpl **>();
    }

    INxDXGI *INxDXGI::Instance() {
        return *Offsets::ResolveID<"NxDXGIImpl::Instance", NxDXGIImpl **>();
    }

}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static WNDPROC WndProc;

static LRESULT APIENTRY WndProc_Hook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;
    return CallWindowProcW(WndProc, hwnd, uMsg, wParam, lParam);
}

static bool (*NxDXGIImpl_Present)(nx::NxDXGIImpl *, pVoid);

static bool NxDXGIImpl_Present_Hook(nx::NxDXGIImpl *dxgi, pVoid inData) {
    struct BackBuffer {
        ID3D12CommandAllocator *Allocator;
        ID3D12Resource *Resource;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle;
    };

    static IDXGISwapChain3 *swapChain = nullptr;
    static ID3D12Device *device = nullptr;
    static ID3D12DescriptorHeap *rtvDescHeap = nullptr;
    static ID3D12DescriptorHeap *srvDescHeap = nullptr;
    static ID3D12GraphicsCommandList *commandList = nullptr;
    static std::vector<BackBuffer> backBuffers;

    static bool initialized = [&]() {
        dxgi->SwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
        assert(swapChain != nullptr);

        dxgi->SwapChain->GetDevice(IID_PPV_ARGS(&device));
        assert(device != nullptr);

        HRESULT hr = S_OK;

        DXGI_SWAP_CHAIN_DESC sdesc;
        hr |= swapChain->GetDesc(&sdesc);

        backBuffers.resize(sdesc.BufferCount);

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = sdesc.BufferCount;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            desc.NodeMask = 1;

            hr |= device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescHeap));

            SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

            for (auto i = 0; i < sdesc.BufferCount; i++) {
                backBuffers[i].CPUDescriptorHandle = rtvHandle;
                rtvHandle.ptr += rtvDescriptorSize;
            }
        }

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            hr |= device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvDescHeap));
        }

        for (auto i = 0; i < sdesc.BufferCount; i++)
            hr |= device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&backBuffers[i].Allocator));

        hr |= device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, backBuffers[0].Allocator, nullptr, IID_PPV_ARGS(&commandList));
        hr |= commandList->Close();

        for (auto i = 0; i < sdesc.BufferCount; i++) {
            auto &backBuffer = backBuffers[i];
            hr |= swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer.Resource));
            device->CreateRenderTargetView(backBuffer.Resource, nullptr, backBuffer.CPUDescriptorHandle);
        }

        if (!SUCCEEDED(hr))
            DebugBreak();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto& io = ImGui::GetIO();
        io.WantCaptureMouse = true;
        io.WantCaptureKeyboard = true;

        ImGui_ImplDX12_InitInfo info;
        info.Device = device;
        info.CommandQueue = nx::INxD3D::Instance()->GetCommandQueue(0);
        info.NumFramesInFlight = backBuffers.size();
        info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        info.DSVFormat = DXGI_FORMAT_UNKNOWN;
        info.LegacySingleSrvCpuDescriptor = srvDescHeap->GetCPUDescriptorHandleForHeapStart();
        info.LegacySingleSrvGpuDescriptor = srvDescHeap->GetGPUDescriptorHandleForHeapStart();

        ImGui_ImplWin32_Init(sdesc.OutputWindow);
        ImGui_ImplDX12_Init(&info);
        WndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(sdesc.OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc_Hook)));

        return true;
    }();

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool showing = true;
    if (showing)
        ImGui::ShowDemoWindow(&showing);

    ImGui::EndFrame();
    ImGui::Render();

    auto &backBuffer = backBuffers[swapChain->GetCurrentBackBufferIndex()];
    backBuffer.Allocator->Reset();

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer.Resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    commandList->Reset(backBuffer.Allocator, nullptr);
    commandList->ResourceBarrier(1, &barrier);
    commandList->OMSetRenderTargets(1, &backBuffer.CPUDescriptorHandle, false, nullptr);
    commandList->SetDescriptorHeaps(1, &srvDescHeap);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);
    commandList->Close();

    ID3D12CommandQueue *commandQueue = nx::INxD3D::Instance()->GetCommandQueue(0);
    commandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList *const *>(&commandList));

    return NxDXGIImpl_Present(dxgi, inData);
}

void Overlay::Attach() {
    Offsets::MapSignature("NxDXGIImpl::Present", "40 55 56 57 41 54 41 56 48 8D AC 24 00 FD FF FF 48 81 EC 00 04 00 00 48 8B F1 33 FF 48");
    Offsets::MapAddress("NxD3DImpl::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 48 8B 01 FF 90 B8 00 00 00 84 C0 75 13 48 8B 0D", 3));
    Offsets::MapAddress("NxDXGIImpl::Instance", Offsets::OffsetFromInstruction("48 8B 0D ? ? ? ? 48 8B 01 FF 50 08 0F B6 D8 84 C0 74 64 48 8B 0D", 3));

    NxDXGIImpl_Present = Offsets::ResolveID<"NxDXGIImpl::Present", decltype(NxDXGIImpl_Present)>();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(reinterpret_cast<PVOID *>(&NxDXGIImpl_Present), static_cast<PVOID>(NxDXGIImpl_Present_Hook));
    DetourTransactionCommit();
}
