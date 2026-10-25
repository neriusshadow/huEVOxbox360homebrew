#pragma once

namespace huEVO {

class App {
public:
    App();
    ~App();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    bool m_Running;
};

extern App g_App;

} // namespace huEVO
