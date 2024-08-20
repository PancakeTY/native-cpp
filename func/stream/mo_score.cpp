#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(
  const std::vector<std::tuple<int, double, double, long>>& observationList,
  long prevTimestamp)
{
    std::vector<uint8_t> buffer;

    // Serialize the size of the vector
    size_t vectorSize = observationList.size();
    buffer.resize(buffer.size() + sizeof(vectorSize));
    std::memcpy(buffer.data() + buffer.size() - sizeof(vectorSize),
                &vectorSize,
                sizeof(vectorSize));

    // Serialize each tuple in the vector
    for (const auto& tuple : observationList) {
        int machineId;
        double cpu, mem;
        long timestamp;

        std::tie(machineId, cpu, mem, timestamp) = tuple;

        // Serialize each element of the tuple
        buffer.resize(buffer.size() + sizeof(machineId));
        std::memcpy(buffer.data() + buffer.size() - sizeof(machineId),
                    &machineId,
                    sizeof(machineId));

        buffer.resize(buffer.size() + sizeof(cpu));
        std::memcpy(
          buffer.data() + buffer.size() - sizeof(cpu), &cpu, sizeof(cpu));

        buffer.resize(buffer.size() + sizeof(mem));
        std::memcpy(
          buffer.data() + buffer.size() - sizeof(mem), &mem, sizeof(mem));

        buffer.resize(buffer.size() + sizeof(timestamp));
        std::memcpy(buffer.data() + buffer.size() - sizeof(timestamp),
                    &timestamp,
                    sizeof(timestamp));
    }

    // Serialize the long value
    buffer.resize(buffer.size() + sizeof(prevTimestamp));
    std::memcpy(buffer.data() + buffer.size() - sizeof(prevTimestamp),
                &prevTimestamp,
                sizeof(prevTimestamp));

    return buffer;
}

void deserialize(
  const std::vector<uint8_t>& buffer,
  std::vector<std::tuple<int, double, double, long>>& observationList,
  long& prevTimestamp)
{
    size_t offset = 0;

    // Deserialize the size of the vector
    size_t vectorSize;
    std::memcpy(&vectorSize, buffer.data() + offset, sizeof(vectorSize));
    offset += sizeof(vectorSize);

    // Resize the vector to hold the deserialized data
    observationList.resize(vectorSize);

    // Deserialize each tuple in the vector
    for (size_t i = 0; i < vectorSize; ++i) {
        int machineId;
        double cpu, mem;
        long timestamp;

        std::memcpy(&machineId, buffer.data() + offset, sizeof(machineId));
        offset += sizeof(machineId);

        std::memcpy(&cpu, buffer.data() + offset, sizeof(cpu));
        offset += sizeof(cpu);

        std::memcpy(&mem, buffer.data() + offset, sizeof(mem));
        offset += sizeof(mem);

        std::memcpy(&timestamp, buffer.data() + offset, sizeof(timestamp));
        offset += sizeof(timestamp);

        observationList[i] = std::make_tuple(machineId, cpu, mem, timestamp);
    }

    // Deserialize the long value
    std::memcpy(&prevTimestamp, buffer.data() + offset, sizeof(prevTimestamp));
}

std::vector<double> calculateDistance(std::vector<std::vector<double>>& matrix)
{
    int numCol = matrix[0].size(); // It is 2 in this case

    // CPU and Mem normalization
    std::vector<double> mins = { 0.0, 0.0 };
    std::vector<double> maxs = { 100.0, 100.0 };
    std::vector<double> centers(numCol, 0.0);

    for (int col = 0; col < numCol; ++col) {
        for (int row = 0; row < matrix.size(); ++row) {
            matrix[row][col] =
              (matrix[row][col] - mins[col]) / (maxs[col] - mins[col]);
            centers[col] += matrix[row][col];
        }
        centers[col] /= matrix.size();
    }

    std::vector<std::vector<double>> distances(
      matrix.size(), std::vector<double>(numCol, 0.0));

    // Calculate the absolute distance from the center
    for (int row = 0; row < matrix.size(); ++row) {
        for (int col = 0; col < numCol; ++col) {
            distances[row][col] = std::abs(matrix[row][col] - centers[col]);
        }
    }

    std::vector<double> l2distances(matrix.size(), 0.0);

    // Calculate the L2 distance (Euclidean distance)
    for (int row = 0; row < l2distances.size(); ++row) {
        for (int col = 0; col < numCol; ++col) {
            l2distances[row] += std::pow(distances[row][col], 2);
        }
        l2distances[row] = std::sqrt(l2distances[row]);
    }

    return l2distances;
}

/***
 * Observation std::tuple<int, double, double, long> = (machineId, cpu, mem,
 * timestamp)
 * Score std::tuple<int, double, long> = (machineId, score,
 * timestamp)
 * Matrix is like:
 * x / y  1   2
 * 1    c1  m1
 * 2    c2  m2
 * 3    c3  m3
 * ***/

std::vector<double> getScores(
  std::vector<std::tuple<int, double, double, long>>& observationList)
{
    // Initialize the matrix
    std::vector<std::vector<double>> matrix(observationList.size(),
                                            std::vector<double>(2));
    for (size_t i = 0; i < observationList.size(); ++i) {
        matrix[i][0] = std::get<1>(observationList[i]); // cpu
        matrix[i][1] = std::get<2>(observationList[i]); // memory
    }

    std::vector<double> l2distances = calculateDistance(matrix);

    for (double& distance : l2distances) {
        distance += 1.0;
    }

    return l2distances;
}

// Observation std::tuple<int, double, double, long> = (machineId, cpu, mem,
// timestamp)
// Score std::tuple<int, double, long> = (machineId, score, timestamp)

int main(int argc, char* argv[])
{
    std::string function = "mo_score";
    int maxRecords = 36;
    int maximumStateLength = 16 + 28 * maxRecords;
    // get the inputMap (inputdata)
    auto inputMap = faasm::getMapInput();
    int machineId = std::stoi(inputMap["machineId"]);
    double cpu = std::stod(inputMap["cpu"]);
    double mem = std::stod(inputMap["mem"]);
    long timestamp = std::stol(inputMap["timestamp"]);

    // Print the input data and previous timestamp
    // std::ostringstream oss;
    // oss << "Input " << i << ": MachineId = " << machineId
    //     << ", CPU = " << cpu << ", Memory = " << mem
    //     << ", Timestamp = " << timestamp
    //     << ", Previous Timestamp = " << prevTimestamp;
    // std::cout << oss.str() << std::endl;

    // Get the function state
    std::vector<std::tuple<int, double, double, long>> observationList;
    long prevTimestamp;

    faasmLockStateWriteSize(function.data(), maximumStateLength);
    faasmPullState(function.data(), maximumStateLength);
    std::vector<uint8_t> stateBuffer(maximumStateLength);
    faasmReadState(function.data(), stateBuffer.data(), maximumStateLength);

    if (!faasm::all_elements_are_zero(stateBuffer)) {
        deserialize(stateBuffer, observationList, prevTimestamp);
    } else {
        prevTimestamp = 0;
    }

    if (timestamp > prevTimestamp) {
        // After getting a new batch, calculate the scores of old batch
        if (observationList.size() > 0) {
            // Calculate the score
            auto scores = getScores(observationList);
            // Print the scores
            // std::ostringstream oss1;
            // oss1 << "Scores: ";
            // for (const auto& score : scores) {
            //     oss1 << score << " ";
            // }
            // std::cout << oss1.str() << std::endl;
            // Send messages to the next function
            for (size_t j = 0; j < observationList.size(); j++) {
                int machineId = std::get<0>(observationList[j]);
                double score = scores[j];
                long timestamp = std::get<3>(observationList[j]);

                // Prepare for chain call
                std::map<std::string, std::string> chainedInput;
                chainedInput["machineId"] =
                  std::to_string(machineId);
                chainedInput["score"] = std::to_string(score);
                chainedInput["timestamp"] = std::to_string(timestamp);
                std::vector<uint8_t> chainedInputBytes;
                faasm::serializeMap(chainedInputBytes, chainedInput);

                faasmChainNamed("mo_anomaly",
                                chainedInputBytes.data(),
                                chainedInputBytes.size());
            }

            // Clear the observationList
            observationList.clear();
        }
        prevTimestamp = timestamp;
    }
    observationList.push_back(std::make_tuple(machineId, cpu, mem, timestamp));

    // print the machine id in the observation list and previous timestamp
    // std::ostringstream oss2;
    // oss2 << "MachineId in the observation list: ";
    // for (const auto& tuple : observationList) {
    //     oss2 << std::get<0>(tuple) << " ";
    // }
    // oss2 << ", Previous Timestamp = " << prevTimestamp;
    // std::cout << oss2.str() << std::endl;

    std::vector<uint8_t> stateBytesUpdated =
      serialize(observationList, prevTimestamp);

    faasmWriteState(
      function.data(), stateBytesUpdated.data(), stateBytesUpdated.size());
    faasmPushState(function.data());
    faasmUnlockStateWrite(function.data());

    return 0;
}