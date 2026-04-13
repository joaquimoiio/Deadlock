#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

class Overlay {
private:
    HWND m_hwnd;
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGISwapChain* m_swapChain;
    ID3D11RenderTargetView* m_renderTargetView;
    bool m_initialized;
    
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    
public:
    Overlay();
    ~Overlay();
    
    bool Create(HINSTANCE hInstance, WNDPROC wndProc);
    void InitImGui();
    void BeginFrame();
    void EndFrame();
    
    ID3D11Device* GetDevice() const { return m_device; }
    ID3D11DeviceContext* GetContext() const { return m_context; }
    HWND GetHWND() const { return m_hwnd; }
    bool IsInitialized() const { return m_initialized; }
    void GetScreenSize(int& width, int& height) const;
};