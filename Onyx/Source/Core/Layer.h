#pragma once

namespace Onyx {

    class Layer
    {
    public:
        Layer();

        virtual void OnUpdate();
        virtual void OnImGui();
    };

}