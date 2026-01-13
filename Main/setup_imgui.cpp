#include "mdx9_api.h"

namespace g_MDX9 {
    void SetSetupImGuiCallback(SetupImGuiCallback callback) {
        g_Callbacks::g_setupImGuiCallback = callback;
    }

    void SetupImGui(LPDIRECT3DDEVICE9 pDevice) {
        // Call user-defined callback if set
        if (g_Callbacks::g_setupImGuiCallback) {
            g_Callbacks::g_setupImGuiCallback(pDevice);
        }
    }
}