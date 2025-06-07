#pragma once

#include "Valkyrion/Core/Application.h"

namespace Valkyrion {

class SandboxApp : public Application
{
public:
    SandboxApp();
    ~SandboxApp() override = default;

    void OnUpdate() override;
    void OnRender() override;
};

} // namespace Valkyrion
