#include "faasm/input.h"
#include "faasm/core.h"
#include "faasm/serialization.h"

#include <stdint.h>
#include <string.h>

namespace faasm {
const char* getStringInput(const char* defaultValue)
{
    long inputSize = faasmGetInputSize();
    if (inputSize == 0) {
        return defaultValue;
    }

    auto inputBuffer = new uint8_t[inputSize + 1];
    faasmGetInput(inputBuffer, inputSize);

    // Force null-termination
    inputBuffer[inputSize] = 0;

    char* strIn = reinterpret_cast<char*>(inputBuffer);

    return strIn;
}

const std::map<std::string, std::string> getMapInput(){
    long inputSize = faasmGetInputSize();
    std::vector<uint8_t> inputBuffer(inputSize);
    faasmGetInput(inputBuffer.data(), inputSize);
    size_t index = 0;
    auto deserializedMap = deserializeMap(inputBuffer, index);
    return deserializedMap;
}

int getIntInput()
{
    const char* inputStr = faasm::getStringInput("0");
    int intVal = std::stoi(inputStr);
    return intVal;
}

void setStringOutput(const char* val)
{
    faasmSetOutput(val, strlen(val));
}

int* parseStringToIntArray(const char* strIn, int nInts)
{
    char* strCopy = new char[strlen(strIn)];
    strcpy(strCopy, strIn);

    char* nextSubstr = strtok(strCopy, " ");
    int* result = new int[nInts];

    int i = 0;
    while (nextSubstr != NULL) {
        result[i] = std::stoi(nextSubstr);
        nextSubstr = strtok(NULL, " ");
        i++;
    }

    return result;
}

bool all_elements_are_zero(const std::vector<uint8_t>& buffer) {
    return std::all_of(buffer.begin(), buffer.end(), [](uint8_t value) {
        return value == 0;
    });
}
} // namespace faasm
