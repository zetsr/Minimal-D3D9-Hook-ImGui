# Minimal-D3D9-Hook-ImGui

A lightweight DirectX 9 ImGui overlay hook library. Drop it into your DLL project and get a fully functional ImGui overlay with automatic input management — no manual hooking required.

---

## Features

- Automatic hooking of `IDirect3DDevice9::EndScene` and `Reset` via MinHook
- Full ImGui initialization and per-frame rendering handled internally
- Input management: blocks game mouse/keyboard input while menu is open
- Raw input interception (`GetRawInputData`, `GetRawInputBuffer`)
- Cursor management hooks (`GetCursorPos`, `SetCursorPos`, `SetCursor`, `ShowCursor`, `ClipCursor`, `GetMouseMovePointsEx`)
- WndProc subclassing for window message handling
- **F1** hotkey toggles the menu open/closed out of the box
- Bundled Alibaba PuHuiTi font family (Regular, Bold, Heavy, Light, Medium) with full Chinese glyph range
- Thread-safe initialization

---

## Requirements

- Windows (x86 or x64)
- DirectX 9 (`d3d9.lib`)
- [MinHook](https://github.com/TsudaKageyu/minhook) (source files included via `extern "C"` includes)
- [Dear ImGui](https://github.com/ocornut/imgui) with `imgui_impl_win32` and `imgui_impl_dx9` backends

---

## Project Structure

```
YourProject/
├── Hook/
│   ├── mdx9_api.h          # Public API header — include this in your DLL
│   ├── hook.cpp            # D3D9 vtable hook and initialization thread
│   ├── input.cpp           # WndProc, cursor, and raw input hooks
│   ├── render.cpp          # Cleanup and lifecycle management
│   ├── setup_imgui.cpp     # User callback dispatch
│   └── mdx9_globals.cpp    # Global variable definitions
├── ImGui/                  # Dear ImGui source
├── MinHook/                # MinHook source
└── Font/                   # Alibaba PuHuiTi font headers
```

---

## Quick Start

### 1. Include the API header

```cpp
#include "mdx9_api.h"
```

### 2. Include MinHook source files

MinHook must be compiled as C. Add these includes in **one** `.cpp` file (typically your `dllmain.cpp`):

```cpp
extern "C" {
#include "../MinHook/src/buffer.c"
#include "../MinHook/src/hook.c"
#include "../MinHook/src/trampoline.c"
#include "../MinHook/src/hde/hde32.c"
#include "../MinHook/src/hde/hde64.c"
}
```

### 3. Define your ImGui draw callback

Implement a function matching the signature `void(LPDIRECT3DDEVICE9 pDevice)`. This function is called every frame inside the ImGui render loop.

```cpp
void MyImGuiDraw(LPDIRECT3DDEVICE9 pDevice)
{
    // g_MDX9::g_MenuState::g_isOpen is toggled by pressing F1
    if (g_MDX9::g_MenuState::g_isOpen) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

        // Always call End() even if Begin() returns false (collapsed window)
        if (ImGui::Begin("My Menu")) {
            ImGui::PushFont(g_MDX9::g_Alibaba_PuHuiTi_Heavy);

            ImGui::Text("Hello World!");

            static bool option = false;
            ImGui::Checkbox("My Option", &option);

            ImGui::PopFont();
        }
        ImGui::End();
    }
}
```

### 4. Set up DllMain

```cpp
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_MDX9::Initialize();                          // Start the hook thread
        g_MDX9::SetSetupImGuiCallback(MyImGuiDraw);   // Register your draw callback
        break;

    case DLL_PROCESS_DETACH:
        g_MDX9::FinalCleanupAll();                     // Clean up all hooks and resources
        break;
    }
    return TRUE;
}
```

That's it. Inject the DLL into a DirectX 9 process and press **F1** to toggle the overlay.

---

## Public API Reference

All symbols live in the `g_MDX9` namespace unless noted otherwise.

### Lifecycle

| Function | Description |
|---|---|
| `g_MDX9::Initialize()` | Spawns the hook installation thread. Call once from `DLL_PROCESS_ATTACH`. |
| `g_MDX9::FinalCleanupAll()` | Removes all hooks, shuts down ImGui, releases D3D9 references. Call from `DLL_PROCESS_DETACH`. |
| `g_MDX9::SetSetupImGuiCallback(callback)` | Registers the per-frame ImGui draw callback. |

### Callback Type

```cpp
typedef void(*SetupImGuiCallback)(LPDIRECT3DDEVICE9 pDevice);
```

Your draw function must match this signature. It is invoked every frame between `ImGui::NewFrame()` and `ImGui::Render()`.

### Menu State

```cpp
g_MDX9::g_MenuState::g_isOpen        // bool — true when the overlay is visible
g_MDX9::g_MenuState::g_lastMousePos  // POINT — cursor position saved before menu opened
```

`g_isOpen` is toggled automatically by **F1**. You can also set it directly from your callback.

### Bundled Fonts

The following `ImFont*` pointers are valid after the first `EndScene` call:

```cpp
g_MDX9::g_Alibaba_PuHuiTi_Regular
g_MDX9::g_Alibaba_PuHuiTi_Bold
g_MDX9::g_Alibaba_PuHuiTi_Heavy
g_MDX9::g_Alibaba_PuHuiTi_Light
g_MDX9::g_Alibaba_PuHuiTi_Medium
```

All fonts are loaded at 18 px with the full Chinese glyph range. Use them with `ImGui::PushFont` / `ImGui::PopFont`.

### Input State

```cpp
g_MDX9::g_InputState::g_blockMouseInput    // atomic<bool>
g_MDX9::g_InputState::g_blockKeyboardInput // atomic<bool>
```

These are managed automatically. While the menu is open, mouse input is blocked from reaching the game. Keyboard input is blocked only when ImGui has keyboard focus (e.g. a text input is active).

### C Export (optional external use)

Two functions are exported with C linkage for use from languages or tools that load the DLL without direct C++ linkage:

```cpp
extern "C" __declspec(dllexport) void SetOverlayWaitTimeout(UINT ms);
extern "C" __declspec(dllexport) void SetSetupImGuiCallback(g_MDX9::SetupImGuiCallback callback);
```

`SetOverlayWaitTimeout` is currently reserved; its value is stored but not yet used in the default implementation.

---

## How It Works

1. **`g_MDX9::Initialize()`** creates a background thread (`MainThread`).
2. The thread creates a temporary hidden window and a throwaway `IDirect3DDevice9` solely to read its vtable.
3. MinHook patches vtable slot **42** (`EndScene`) and slot **16** (`Reset`) to redirect calls to the library's hooks.
4. On the **first `EndScene` call** from the real device, the library initializes ImGui contexts, loads fonts, and installs WndProc, cursor, and raw input hooks.
5. Every subsequent `EndScene` call drives a full ImGui frame: `NewFrame` → your callback → `EndFrame` → `Render` → `RenderDrawData`.
6. When `Reset` is called (e.g. on resolution change), ImGui device objects are invalidated and re-created on the next `EndScene`.
7. On DLL detach, `FinalCleanupAll` removes all hooks and releases resources in the correct order.

---

## Input Behavior

| Situation | Behavior |
|---|---|
| Menu closed | All input passes through to the game normally |
| Menu open | Mouse messages blocked from game; ImGui receives them |
| ImGui text field focused | Keyboard messages blocked from game |
| F1 pressed | Menu toggled; cursor warped to window center on open, restored on close |

---

## Notes

- The menu starts **open** by default (`g_isOpen = true`). Change the default in `mdx9_globals.cpp` if you prefer it closed on startup.
- `io.IniFilename` is set to `nullptr`, so ImGui will not write an `.ini` file to disk.
- `ImGuiConfigFlags_NoMouseCursorChange` is toggled per-frame so that ImGui draws its own software cursor only when the menu is open.
- The library is designed for injection into existing processes. Do not use it in a process you own end-to-end where you can initialize ImGui directly.

---

## Credits

- [Dear ImGui](https://github.com/ocornut/imgui) — Immediate-mode GUI library
- [MinHook](https://github.com/TsudaKageyu/minhook) — Lightweight API hook library
- [Universal-Dear-ImGui-Hook](https://github.com/Sh0ckFR/Universal-Dear-ImGui-Hook) — Reference hook implementation
