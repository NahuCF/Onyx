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

        // Return the size of the message in bytes
        size_t Size() const
        {
            return sizeof(Header<T>) + data.size();
        };

        template<typename DataType>
        friend Message<T>& operator<<(Message<T>& message, DataType& data)
        {
            // Cache current size vector, as this will be the point where we insert data
            size_t sizeData = message.Size();

            // Resize vector to hold new data
            message.data.resize(message.data.size() + sizeof(DataType));

            // Copy the data into the newly resized vector
            std::memcpy(message.data.data() + sizeData, &data, sizeof(DataType));

            // Update size of message
            message.header.size = message.Size();

            // Return the message so it can be chained
            return message;
        }

        template<typename DataType>
        friend Message<T>& operator>>(Message<T>& message, DataType& data)
        {
            // Cache current size vector, as this will be the point where we insert data
            size_t sizeData = message.Size() - sizeof(DataType);

            // Copy the data into the newly resized vector
            std::memcpy(&data, message.data.data() + sizeData, sizeof(DataType));

            // Resize vector to hold new data
            message.data.resize(message.data.size() + sizeof(DataType));

            // Update size of message
            message.header.size = message.Size();

            // Return the message so it can be chained
            return message;
        }
    };

}