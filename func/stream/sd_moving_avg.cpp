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
#include <string>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(const std::pair<double, std::list<double>>& p)
{
    std::vector<uint8_t> buffer;

    // Serialize the first double
    const uint8_t* double_data = reinterpret_cast<const uint8_t*>(&p.first);
    buffer.insert(buffer.end(), double_data, double_data + sizeof(double));

    // Serialize the size of the list
    size_t size = p.second.size();
    const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
    buffer.insert(buffer.end(), size_data, size_data + sizeof(size));

    // Serialize each double in the list
    for (double val : p.second) {
        const uint8_t* val_data = reinterpret_cast<const uint8_t*>(&val);
        buffer.insert(buffer.end(), val_data, val_data + sizeof(double));
    }

    return buffer;
}

std::pair<double, std::list<double>> deserialize(
  const std::vector<uint8_t>& buffer)
{
    size_t offset = 0;

    // Deserialize the first double
    double first;
    std::memcpy(&first, buffer.data() + offset, sizeof(double));
    offset += sizeof(double);

    // Deserialize the size of the list
    size_t size;
    std::memcpy(&size, buffer.data() + offset, sizeof(size));
    offset += sizeof(size);

    // Deserialize each double and populate the list
    std::list<double> second;
    for (size_t i = 0; i < size; ++i) {
        double val;
        std::memcpy(&val, buffer.data() + offset, sizeof(double));
        offset += sizeof(double);
        second.push_back(val);
    }

    return std::move(std::make_pair(first, second));
}

int main(int argc, char* argv[])
{
    int windowlength = 100;
    int maximumStateLength = 16 + 8 * windowlength;
    // get the inputMap (inputdata) and value
    auto inputMap = faasm::getMapInput();
    std::string sensorId = inputMap["sensorId"];
    double todoValue = std::stod(inputMap["temperature"]);
    // Print the input data
    // std::cout << "Sensor ID: " << sensorId << ", Value: " << todoValue
    //           << std::endl;

    uint64_t start = faasmGetMicros();
    
    // Prepare the states
    std::pair<double, std::list<double>> statistics;
    faasmLockStateWriteSize(sensorId.data(), maximumStateLength);
    faasmPullState(sensorId.data(), maximumStateLength);
    std::vector<uint8_t> stateBuffer(maximumStateLength);
    faasmReadState(sensorId.data(), stateBuffer.data(), maximumStateLength);
    if (!faasm::all_elements_are_zero(stateBuffer)) {
        statistics = deserialize(stateBuffer);
    } else {
        statistics = { 0.0, std::list<double>() };
    }
    double& sum = statistics.first;
    std::list<double>& values = statistics.second;

    if (values.size() >= windowlength) {
        sum -= values.front();
        values.pop_front();
    }
    sum += todoValue;
    values.push_back(todoValue);
    double avg = sum / values.size();
    // Chained call next function
    std::map<std::string, std::string> chainedInput;
    chainedInput["movingAverage"] = std::to_string(avg);
    chainedInput["temperature"] = std::to_string(todoValue);
    std::vector<uint8_t> chainedInputBytes;
    faasm::serializeMap(chainedInputBytes, chainedInput);
    faasmChainNamed(
      "sd_spike_detect", chainedInputBytes.data(), chainedInputBytes.size());

    std::pair<double, std::list<double>> newStatistics = { sum, values };
    std::vector<uint8_t> newStatisticsBytes = serialize(newStatistics);

    faasmWriteState(
      sensorId.data(), newStatisticsBytes.data(), newStatisticsBytes.size());
    faasmPushState(sensorId.data());
    faasmUnlockStateWrite(sensorId.data());

    uint64_t end = faasmGetMicros();
    uint64_t diff = end - start;

    std::string output =
      "sd_moving_avg_lock_input_size: 1 and duration:" + std::to_string(diff);

    faasmSetOutput(output.c_str(), output.size());

    return 0;
}
