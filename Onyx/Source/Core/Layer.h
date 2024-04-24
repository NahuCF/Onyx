#pragma once

namespace Onyx {

    class Layer
    {
    public:
        Layer() {};

        virtual void OnUpdate() = 0;
        virtual void OnImGui() {};
        virtual void OnAttach() {};
    };

}