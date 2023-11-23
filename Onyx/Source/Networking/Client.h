#pragma once

#include "Message.h"

namespace Onyx {

    template<typename T>
    class Client
    {
    public:
        Client();
        ~Client();

        void Disconect();

        bool IsConnected();

        void Send(const Message<T>& message);
    }

}