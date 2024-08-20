#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>

#include <algorithm> // for std::all_of
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::vector<uint8_t> serialize_int(int value)
{
    std::vector<uint8_t> serialized_data(4);
    serialized_data[0] = static_cast<uint8_t>(value >> 24);
    serialized_data[1] = static_cast<uint8_t>(value >> 16);
    serialized_data[2] = static_cast<uint8_t>(value >> 8);
    serialized_data[3] = static_cast<uint8_t>(value);
    return serialized_data;
}

int deserialize_int(const std::vector<uint8_t>& data)
{
    if (data.size() != 4) {
        throw std::invalid_argument("Invalid data size for deserialization");
    }
    int value = (static_cast<int>(data[0]) << 24) |
                (static_cast<int>(data[1]) << 16) |
                (static_cast<int>(data[2]) << 8) | static_cast<int>(data[3]);
    return value;
}

int main(int argc, char* argv[])
{
    auto inputMap = faasm::getMapInput();
    auto inputWord = inputMap["word"];
    // Print the input word
    // std::cout << inputWord << std::endl;

    int count = 0;

    faasmLockStateWriteSize(inputWord.data(), 4);
    faasmPullState(inputWord.data(), 4);
    std::vector<uint8_t> stateBuffer(4);
    faasmReadState(inputWord.data(), stateBuffer.data(), 4);
    if (!faasm::all_elements_are_zero(stateBuffer)) {
        count = deserialize_int(stateBuffer);
    }

    count++;
    // Print the count for input word
    // std::cout << "Count for word " << inputWord << " is " << count << std::endl;

    stateBuffer = serialize_int(count);
    faasmWriteState(inputWord.data(), stateBuffer.data(), stateBuffer.size());
    faasmPushState(inputWord.data());
    faasmUnlockStateWrite(inputWord.data());
    return 0;
}
