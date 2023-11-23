#pragma once

#include "Message.h"

namespace Onyx {

    template<typename T>
    class Connection
    {
    public:
        Connection();
        ~Connection();

        void Send(const Message<T>& message);
    };
    
} 