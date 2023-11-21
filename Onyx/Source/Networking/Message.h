#pragma once

#include <vector>

namespace Onyx {

    template<typename T>
    struct Header
    {
        T id{};
        uint32_t size = 0;
    };

    template<typename T>
    struct Message 
    {
        Header<T> header;
        std::vector<uint8_t> data; 

        size_t Size() const;
    };

}