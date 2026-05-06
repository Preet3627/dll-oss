#pragma once

#include "../Module.hpp"
#include <d3d11.h>
#include <dxgi1_6.h>

class MetalSupport : public Module {
public:
    MetalSupport() : Module("Metal Support",
        "Enables native Metal GPU acceleration on macOS ARM64. "
        "Bypasses DirectX emulation layers for 100% native GPU performance "
        "on Apple Silicon and Intel Macs running Windows via VMware Fusion/Parallels.",
        IDR_FULLBRIGHT_PNG, "", false, {"metal", "gpu", "arm64", "mac"}) {
    }

    void onEnable() override;
    void onDisable() override;
    void defaultConfig() override;
    void settingsRender(float settingsOffset) override;

    void onRender(RenderEvent& event);
    void onTick(TickEvent& event);

    static bool IsMacEnvironment();
    static bool InitializeMetalBridge();
    static void ShutdownMetalBridge();

private:
    bool Initialize();
    void RenderFrame();

    bool m_initialized = false;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain1* m_swapChain = nullptr;
};
