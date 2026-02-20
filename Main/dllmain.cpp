#include "mdx9_api.h"

extern "C" {
#include "../MinHook/src/buffer.c"
#include "../MinHook/src/hook.c"
#include "../MinHook/src/trampoline.c"
#include "../MinHook/src/hde/hde32.c"
#include "../MinHook/src/hde/hde64.c"
}

void MyImGuiDraw(LPDIRECT3DDEVICE9 pDevice)
{
    // 检查菜单是否打开（按 F1 切换）
    if (g_MDX9::g_MenuState::g_isOpen) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

        // 注意：即使 Begin 返回 false（窗口被折叠），你依然需要调用 End()
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // 初始化 Hook 系统
        g_MDX9::Initialize();

        // 设置自定义绘制回调
        g_MDX9::SetSetupImGuiCallback(MyImGuiDraw);
        break;

    case DLL_PROCESS_DETACH:
        // 清理资源
        g_MDX9::FinalCleanupAll();
        break;
    }
    return TRUE;
}