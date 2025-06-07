#include "pch.h"
#include "SandboxApp.h"

int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        auto app = std::make_unique<Valkyrion::SandboxApp>();
        app->Run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
