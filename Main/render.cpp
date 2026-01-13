#include "mdx9_api.h"

#pragma warning(push)
#pragma warning(disable: 26451)
#pragma warning(disable: 26812)
#pragma warning(disable: 6387)
#pragma warning(pop)

namespace g_MDX9 {
    void CleanupRenderResources() {
        g_InitState::g_Initialized = false;

        if (ImGui::GetCurrentContext()) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }

        if (g_D3D9Resources::g_pd3dDevice) {
            g_D3D9Resources::g_pd3dDevice->Release();
            g_D3D9Resources::g_pd3dDevice = nullptr;
        }

        rawinputhook::Remove();
        cursorhook::Remove();
    }

    void CleanupRenderResources_NoInput() {
        if (ImGui::GetCurrentContext()) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
        }

        if (g_D3D9Resources::g_pd3dDevice) {
            g_D3D9Resources::g_pd3dDevice->Release();
            g_D3D9Resources::g_pd3dDevice = nullptr;
        }

        g_InitState::g_Initialized = false;
    }

    void FinalCleanupAll() {
        std::lock_guard<std::mutex> lock(g_InitState::g_InitMutex);

        if (g_InitState::g_Initialized) {
            CleanupRenderResources();
        }

        if (g_ProcessWindow::g_mainWindow) {
            inputhook::Remove(g_ProcessWindow::g_mainWindow);
        }

        if (g_D3D9Resources::g_pd3dDevice) {
            g_D3D9Resources::g_pd3dDevice->Release();
            g_D3D9Resources::g_pd3dDevice = nullptr;
        }
    }

    void InitProcessName() {
        // Implementation here if needed
    }

    void Initialize() {
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
}

// Export C functions for external usage
extern "C" {
    __declspec(dllexport) void SetOverlayWaitTimeout(UINT ms) {
        g_MDX9::g_InitState::g_waitTimeoutMs = ms;
    }

    __declspec(dllexport) void SetSetupImGuiCallback(g_MDX9::SetupImGuiCallback callback) {
        g_MDX9::SetSetupImGuiCallback(callback);
    }
}