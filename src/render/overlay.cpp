#include "overlay.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Overlay::Overlay() : m_hwnd(nullptr), m_device(nullptr), m_context(nullptr),
                     m_swapChain(nullptr), m_renderTargetView(nullptr), m_initialized(false) {}

Overlay::~Overlay() {
    CleanupRenderTarget();
    CleanupDeviceD3D();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool Overlay::CreateDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &m_swapChain,
        &m_device, &featureLevel, &m_context
    );
    
    return SUCCEEDED(hr);
}

void Overlay::CleanupDeviceD3D() {
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

void Overlay::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_renderTargetView);
    pBackBuffer->Release();
}

void Overlay::CleanupRenderTarget() {
    if (m_renderTargetView) { m_renderTargetView->Release(); m_renderTargetView = nullptr; }
}

void Overlay::GetScreenSize(int& width, int& height) const {
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

bool Overlay::Create(HINSTANCE hInstance, WNDPROC wndProc) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DeadUnlockOverlay";
    RegisterClassEx(&wc);
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        L"DeadUnlockOverlay", L"DeadUnlock",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (!m_hwnd) return false;
    
    SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(m_hwnd, SW_SHOW);
    
    if (!CreateDeviceD3D()) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        return false;
    }
    CreateRenderTarget();
    
    m_initialized = true;
    return true;
}

void Overlay::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_device, m_context);
    
    ImGui::StyleColorsDark(); // base reset; ImGuiMenu::ApplyStyle() overrides on construction
}

void Overlay::BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Overlay::EndFrame() {
    ImGui::Render();
    
    float clearColor[4] = {0, 0, 0, 0};
    m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
    
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_swapChain->Present(1, 0);
}