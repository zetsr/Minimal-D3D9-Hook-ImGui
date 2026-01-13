#include "mdx9_api.h"

namespace g_MDX9 {
    // Hook original function pointers
    namespace g_HookFunctions {
        PFN_EndScene g_oEndScene = nullptr;
        PFN_Reset g_oReset = nullptr;
        PFN_GetRawInputData g_oGetRawInputData = nullptr;
        PFN_GetRawInputBuffer g_oGetRawInputBuffer = nullptr;
        PFN_GetCursorPos g_oGetCursorPos = nullptr;
        PFN_SetCursorPos g_oSetCursorPos = nullptr;
        PFN_SetCursor g_oSetCursor = nullptr;
        PFN_ShowCursor g_oShowCursor = nullptr;
        PFN_GetClipCursor g_oGetClipCursor = nullptr;
        PFN_ClipCursor g_oClipCursor = nullptr;
        PFN_GetMouseMovePointsEx g_oGetMouseMovePointsEx = nullptr;
    }

    // Direct3D 9 resources
    namespace g_D3D9Resources {
        LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
    }

    // Initialization state
    namespace g_InitState {
        bool g_Initialized = false;
        bool g_AfterFirstEndScene = false;
        std::mutex g_InitMutex;
        UINT g_waitTimeoutMs = 2000;
    }

    // Process and window
    namespace g_ProcessWindow {
        std::string g_processName;
        HWND g_mainWindow = nullptr;
        RECT g_windowRect = { 0 };
    }

    // Input state
    namespace g_InputState {
        std::atomic<bool> g_blockMouseInput{ false };
        std::atomic<bool> g_blockKeyboardInput{ false };
    }

    // Menu state
    namespace g_MenuState {
        bool g_isOpen = true;
        bool g_wasOpenLastFrame = true;
        POINT g_lastMousePos = { 0, 0 };
    }

    // Callback functions
    namespace g_Callbacks {
        SetupImGuiCallback g_setupImGuiCallback = nullptr;
    }
}