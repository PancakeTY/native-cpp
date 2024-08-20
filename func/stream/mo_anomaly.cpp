#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

// Function to serialize a std::list<double> to std::vector<uint8_t>
std::vector<uint8_t> serialize(const std::list<double>& data)
{
    size_t listSize = data.size();
    size_t totalSize = sizeof(listSize) + listSize * sizeof(double);

    // Allocate the entire buffer in one go
    std::vector<uint8_t> buffer(totalSize);

    // Store the size of the list
    std::memcpy(buffer.data(), &listSize, sizeof(listSize));

    // Store each double in the list
    size_t offset = sizeof(listSize);
    for (double value : data) {
        std::memcpy(buffer.data() + offset, &value, sizeof(double));
        offset += sizeof(double);
    }

    return buffer;
}

// Function to deserialize a std::vector<uint8_t> back to std::list<double>
std::list<double> deserialize(const std::vector<uint8_t>& buffer)
{
    std::list<double> data;

    if (buffer.size() < sizeof(size_t)) {
        throw std::runtime_error("Buffer is too small to contain list size.");
    }

    // Extract the size of the list
    size_t listSize;
    std::memcpy(&listSize, buffer.data(), sizeof(listSize));

    size_t offset = sizeof(size_t);
    for (size_t i = 0; i < listSize; ++i) {
        if (offset + sizeof(double) > buffer.size()) {
            throw std::runtime_error(
              "Buffer is too small to contain all list elements.");
        }

        double value;
        std::memcpy(&value, buffer.data() + offset, sizeof(double));
        data.push_back(value);
        offset += sizeof(double);
    }

    return data;
}

int main(int argc, char* argv[])
{
    int windowlength = 100;
    // get the inputMap (inputdata)
    int maximumStateLength = 8 + 8 * windowlength;
    auto inputMap = faasm::getMapInput();
    std::string machineId = inputMap.at("machineId");
    double score = std::stod(inputMap.at("score"));
    long timestamp = std::stol(inputMap.at("timestamp"));

    // Prepare states of partitioned attribute 'key'
    std::list<double> pastScores;
    faasmLockStateWriteSize(machineId.data(), maximumStateLength);
    faasmPullState(machineId.data(), maximumStateLength);
    std::vector<uint8_t> stateBuffer(maximumStateLength);
    faasmReadState(machineId.data(), stateBuffer.data(), maximumStateLength);
    if (!faasm::all_elements_are_zero(stateBuffer)) {
        pastScores = deserialize(stateBuffer);
    }

    // Function Logic
    pastScores.push_back(score);
    if (pastScores.size() == windowlength) {
        pastScores.pop_front();
    }

    double sumScore = 0.0;
    for (const double& pastScore : pastScores) {
        sumScore += pastScore;
    }

    // Chained call next function
    std::map<std::string, std::string> chainedInput;
    chainedInput["machineId"] = machineId;
    chainedInput["score"] = std::to_string(score);
    chainedInput["sumScore"] = std::to_string(sumScore);
    chainedInput["timestamp"] = std::to_string(timestamp);
    std::vector<uint8_t> chainedInputBytes;
    faasm::serializeMap(chainedInputBytes, chainedInput);
    faasmChainNamed(
      "mo_alert", chainedInputBytes.data(), chainedInputBytes.size());

    std::vector<uint8_t> newPastScoresBytes = serialize(pastScores);
    faasmWriteState(
      machineId.data(), newPastScoresBytes.data(), newPastScoresBytes.size());
    faasmPushState(machineId.data());
    faasmUnlockStateWrite(machineId.data());
    return 0;
}
