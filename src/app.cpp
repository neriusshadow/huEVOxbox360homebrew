#include "app.h"
#include "config.h"
#include "renderer.h"
#include "input.h"
#include "network.h"
#include "ui.h"

#ifdef _XBOX
#include <xtl.h>
extern "C" void HalReturnToFirmware(unsigned int routine);
#define REBOOT_TO_DASHBOARD 1
#endif

namespace huEVO {

App g_App;
AppConfig g_Config;

App::App()
    : m_Running(false)
{
}

App::~App() {
    Shutdown();
}

bool App::Initialize() {
    if (!g_Renderer.Initialize()) {
        return false;
    }

    g_Network.Initialize();
    g_UI.Initialize();

    m_Running = true;
    return true;
}

void App::Run() {
    while (m_Running) {
        g_Input.Update();

        // Exit to dashboard on Back button when on root sidebar
        if (g_Input.IsPressed(XINPUT_GAMEPAD_BACK)) {
            m_Running = false;
            break;
        }

        g_UI.Update();

        g_Renderer.BeginFrame();
        g_UI.Render();
        g_Renderer.EndFrame();

#ifdef _XBOX
        Sleep(1);
#endif
    }

#ifdef _XBOX
    // Clean return to Aurora / Freestyle / Official Dashboard
    HalReturnToFirmware(REBOOT_TO_DASHBOARD);
#endif
}

void App::Shutdown() {
    if (!m_Running) return;
    g_Network.Shutdown();
    g_Renderer.Shutdown();
    m_Running = false;
}

} // namespace huEVO
