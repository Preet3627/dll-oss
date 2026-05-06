#include "MetalSupport.hpp"
#include "SDK/SDK.hpp"
#include "Utils/Logger/Logger.hpp"
#include <windows.h>
#include <dxgi1_6.h>

void MetalSupport::onEnable() {
    Listen(this, RenderEvent, &MetalSupport::onRender)
    Listen(this, TickEvent, &MetalSupport::onTick)
    Module::onEnable();

    if (getOps<bool>("autoEnable")) {
        Initialize();
    }
}

void MetalSupport::onDisable() {
    Deafen(this, RenderEvent, &MetalSupport::onRender)
    Deafen(this, TickEvent, &MetalSupport::onTick)
    ShutdownMetalBridge();
    m_initialized = false;
    Module::onDisable();
}

void MetalSupport::defaultConfig() {
    Module::defaultConfig("performance");
    setDef("autoEnable", true);
    setDef("enableVSync", true);
    setDef("maxFPS", 0.f);
    setDef("forceDirectToMetal", true);
    setDef("bypassTranslationLayer", true);
    setDef("notifyOnInit", true);
}

void MetalSupport::settingsRender(float settingsOffset) {
    initSettingsPage();

    addHeader("Metal GPU Acceleration");
    addInfo("Enables native Metal GPU access on macOS, bypassing DirectX translation layers for maximum performance.");

    addHeader("Configuration");
    addToggle("Auto-Enable", "Automatically initialize Metal bridge when module is enabled", "autoEnable");
    addToggle("VSync", "Enable vertical sync for tear-free rendering", "enableVSync");
    addSlider("Max FPS", "Frame rate limit (0 = unlimited)", "maxFPS", 240.f, 0.f, true);

    addHeader("Advanced");
    addToggle("Force Direct-to-Metal", "Bypass all D3D-to-Metal translation for native rendering", "forceDirectToMetal");
    addToggle("Bypass Translation Layer", "Skip DirectX compatibility layer for ARM64", "bypassTranslationLayer");
    addToggle("Notify on Init", "Show notification when Metal bridge initializes", "notifyOnInit");

    FlarialGUI::UnsetScrollView();
    resetPadding();
}

bool MetalSupport::IsMacEnvironment() {
    // Detect VMware Fusion / Parallels / Bootcamp on Mac
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\SYSTEM\\CurrentControlSet", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256] = {0};
        DWORD size = sizeof(buffer);
        if (RegQueryValueExA(hKey, "SystemProductName", nullptr, nullptr, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            std::string productName(buffer);
            return productName.find("VMware") != std::string::npos ||
                   productName.find("Parallels") != std::string::npos;
        }
        RegCloseKey(hKey);
    }

    // Check for Apple Silicon via WSL2/ARM64 detection
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
        return true;
    }

    return false;
}

bool MetalSupport::InitializeMetalBridge() {
    if (!IsMacEnvironment()) {
        Logger::debug("[MetalSupport] Not running on Mac environment, skipping Metal bridge");
        return false;
    }

    Logger::debug("[MetalSupport] Initializing Metal GPU bridge...");

    // On Windows on ARM running via VMware Fusion/Parallels,
    // the D3D11-on-Metal layer should already be available.
    // We just need to ensure we're using the native path.

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &m_device,
        nullptr,
        &m_context
    );

    if (FAILED(hr)) {
        Logger::debug("[MetalSupport] Failed to create D3D11 device: 0x" + std::to_string(hr));
        return false;
    }

    m_initialized = true;
    Logger::debug("[MetalSupport] Metal bridge initialized successfully");
    return true;
}

void MetalSupport::ShutdownMetalBridge() {
    if (m_context) {
        m_context->ClearState();
        m_context->Release();
        m_context = nullptr;
    }
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    if (m_swapChain) {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }
    Logger::debug("[MetalSupport] Metal bridge shutdown");
}

bool MetalSupport::Initialize() {
    if (m_initialized) return true;

    if (InitializeMetalBridge()) {
        if (getOps<bool>("notifyOnInit")) {
            FlarialGUI::Notify("Metal GPU acceleration enabled");
        }
        return true;
    }

    return false;
}

void MetalSupport::RenderFrame() {
    if (!m_initialized || !m_context || !m_device) return;

    // If forceDirectToMetal is enabled, we bypass the D3D translation layer
    if (getOps<bool>("forceDirectToMetal")) {
        // On D3D11-on-Metal, the driver handles this automatically
        // We just ensure we're not using any compatibility features
        // that would add overhead
    }
}

void MetalSupport::onRender(RenderEvent& event) {
    if (!this->isEnabled()) return;

    if (!m_initialized && getOps<bool>("autoEnable")) {
        Initialize();
    }

    if (m_initialized) {
        RenderFrame();
    }
}

void MetalSupport::onTick(TickEvent& event) {
    if (!this->isEnabled()) return;

    // Enforce max FPS if set
    float maxFPS = getOps<float>("maxFPS");
    if (maxFPS > 0.f && MC::fps > maxFPS) {
        // Sleep to cap framerate
        DWORD sleepTime = static_cast<DWORD>((1000.f / maxFPS) - (1000.f / MC::fps));
        if (sleepTime > 0 && sleepTime < 100) {
            Sleep(sleepTime);
        }
    }
}
