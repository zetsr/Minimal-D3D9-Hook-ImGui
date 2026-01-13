#include "mdx9_api.h"

#pragma warning(push)
#pragma warning(disable: 26451)
#pragma warning(disable: 26812)
#pragma warning(disable: 6387)
#pragma warning(pop)

namespace g_MDX9 {
    HRESULT STDMETHODCALLTYPE hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
        g_InitState::g_AfterFirstEndScene = true;

        if (!pDevice) {
            if (g_HookFunctions::g_oEndScene) return g_HookFunctions::g_oEndScene(pDevice);
            return D3D_OK;
        }

        std::lock_guard<std::mutex> lock(g_InitState::g_InitMutex);

        if (!g_InitState::g_Initialized) {
            // Store device
            if (!g_D3D9Resources::g_pd3dDevice) {
                pDevice->AddRef();
                g_D3D9Resources::g_pd3dDevice = pDevice;
            }

            // Get window handle
            D3DDEVICE_CREATION_PARAMETERS params;
            if (SUCCEEDED(pDevice->GetCreationParameters(&params))) {
                HWND newWindow = params.hFocusWindow;
                GetWindowRect(newWindow, &g_ProcessWindow::g_windowRect);

                if (g_ProcessWindow::g_mainWindow != newWindow) {
                    if (g_ProcessWindow::g_mainWindow) {
                        inputhook::Remove(g_ProcessWindow::g_mainWindow);
                    }
                    g_ProcessWindow::g_mainWindow = newWindow;
                    inputhook::Init(g_ProcessWindow::g_mainWindow);
                }
                else if (!g_ProcessWindow::g_mainWindow) {
                    g_ProcessWindow::g_mainWindow = newWindow;
                    inputhook::Init(g_ProcessWindow::g_mainWindow);
                }
            }

            // Initialize ImGui only if context doesn't exist
            if (!ImGui::GetCurrentContext()) {
                ImGui::CreateContext();
                ImGui::StyleColorsDark();
                ImGui_ImplWin32_Init(g_ProcessWindow::g_mainWindow);
            }

            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            // Initialize DX9 backend
            ImGui_ImplDX9_Init(pDevice);

            // Initialize input hooks
            rawinputhook::Init();
            cursorhook::Init();

            g_InitState::g_Initialized = true;
        }

        // Start ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        bool cursorStateChanged = (g_MenuState::g_isOpen != g_MenuState::g_wasOpenLastFrame);

        g_MenuState::g_wasOpenLastFrame = g_MenuState::g_isOpen;
        inputhook::UpdateInputBlockState();
        cursorhook::UpdateCursorState();

        if (g_MenuState::g_isOpen) {
            io.MouseDrawCursor = true;
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        }
        else {
            io.MouseDrawCursor = false;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        }

        if (cursorStateChanged) {
            if (g_MenuState::g_isOpen) {
                while (ShowCursor(FALSE) >= 0);
                SetCursor(nullptr);
                GetCursorPos(&g_MenuState::g_lastMousePos);
                RECT rect;
                GetWindowRect(g_ProcessWindow::g_mainWindow, &rect);
                int centerX = rect.left + (rect.right - rect.left) / 2;
                int centerY = rect.top + (rect.bottom - rect.top) / 2;
                SetCursorPos(centerX, centerY);
            }
        }

        if (g_MenuState::g_isOpen) {
            static int frameCounter = 0;
            frameCounter++;

            if (frameCounter % 30 == 0) {
                RECT rect;
                GetWindowRect(g_ProcessWindow::g_mainWindow, &rect);
                int centerX = rect.left + (rect.right - rect.left) / 2;
                int centerY = rect.top + (rect.bottom - rect.top) / 2;
                SetCursorPos(centerX, centerY);
            }
        }

        // Call user callback
        SetupImGui(pDevice);

        // Render ImGui
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        if (g_HookFunctions::g_oEndScene) return g_HookFunctions::g_oEndScene(pDevice);
        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
        std::lock_guard<std::mutex> lock(g_InitState::g_InitMutex);

        if (g_InitState::g_Initialized) {
            CleanupRenderResources_NoInput();
        }

        HRESULT hr = g_HookFunctions::g_oReset ? g_HookFunctions::g_oReset(pDevice, pPresentationParameters) : D3D_OK;

        g_InputState::g_blockMouseInput = false;
        g_InputState::g_blockKeyboardInput = false;

        return hr;
    }

    DWORD WINAPI MainThread(LPVOID) {
        if (MH_Initialize() != MH_OK) return 0;

        while (true) {
            WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProcW, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"TempDX9", nullptr };
            RegisterClassEx(&wc);
            HWND tempWnd = CreateWindow(wc.lpszClassName, L"Temp", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

            if (!tempWnd) {
                UnregisterClass(wc.lpszClassName, wc.hInstance);
                Sleep(1);
                continue;
            }

            LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
            LPDIRECT3DDEVICE9 pDevice = nullptr;

            if (!pD3D) {
                DestroyWindow(tempWnd);
                UnregisterClass(wc.lpszClassName, wc.hInstance);
                Sleep(1);
                continue;
            }

            D3DPRESENT_PARAMETERS d3dpp = {};
            d3dpp.Windowed = TRUE;
            d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            d3dpp.hDeviceWindow = tempWnd;
            d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

            HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tempWnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);

            if (SUCCEEDED(hr) && pDevice) {
                void** vTable = *reinterpret_cast<void***>(pDevice);

                // Hook EndScene (index 42)
                if (vTable && vTable[42]) {
                    MH_CreateHook(vTable[42], reinterpret_cast<LPVOID>(hkEndScene), reinterpret_cast<LPVOID*>(&g_HookFunctions::g_oEndScene));
                }

                // Hook Reset (index 16)
                if (vTable && vTable[16]) {
                    MH_CreateHook(vTable[16], reinterpret_cast<LPVOID>(hkReset), reinterpret_cast<LPVOID*>(&g_HookFunctions::g_oReset));
                }

                MH_EnableHook(MH_ALL_HOOKS);

                pDevice->Release();
                pD3D->Release();
                DestroyWindow(tempWnd);
                UnregisterClass(wc.lpszClassName, wc.hInstance);
                break;
            }

            if (pDevice) pDevice->Release();
            if (pD3D) pD3D->Release();

            DestroyWindow(tempWnd);
            UnregisterClass(wc.lpszClassName, wc.hInstance);
            Sleep(1);
        }

        return 0;
    }
}