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
#include <limits>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

template<typename T>
void serializePrimitive(std::vector<uint8_t>& buffer, const T& value)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
}

void serializeVector(std::vector<uint8_t>& buffer,
                     const std::vector<uint8_t>& vec)
{
    serializePrimitive(buffer, vec.size());
    buffer.insert(buffer.end(), vec.begin(), vec.end());
}

void serializeTuple(std::vector<uint8_t>& buffer,
                    const std::tuple<int, double, double, long>& tuple)
{
    serializePrimitive(buffer, std::get<0>(tuple));
    serializePrimitive(buffer, std::get<1>(tuple));
    serializePrimitive(buffer, std::get<2>(tuple));
    serializePrimitive(buffer, std::get<3>(tuple));
}

std::vector<uint8_t> serialize(
  const std::tuple<long,
                   double,
                   double,
                   std::vector<std::tuple<int, double, double, long>>>& data)
{

    std::vector<uint8_t> buffer;

    // Serialize the first long
    serializePrimitive(buffer, std::get<0>(data));

    // Serialize the first double
    serializePrimitive(buffer, std::get<1>(data));

    // Serialize the second double
    serializePrimitive(buffer, std::get<2>(data));

    // Serialize the vector of tuples
    const auto& vec = std::get<3>(data);
    serializePrimitive(buffer, vec.size()); // Serialize the size of the vector

    for (const auto& tuple : vec) {
        serializeTuple(buffer, tuple);
    }

    return buffer;
}

template<typename T>
void deserializePrimitive(const std::vector<uint8_t>& buffer,
                          size_t& offset,
                          T& value)
{
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
}

std::tuple<int, double, double, long> deserializeTuple(
  const std::vector<uint8_t>& buffer,
  size_t& offset)
{
    int first;
    double second, third;
    long fourth;

    deserializePrimitive(buffer, offset, first);
    deserializePrimitive(buffer, offset, second);
    deserializePrimitive(buffer, offset, third);
    deserializePrimitive(buffer, offset, fourth);

    return std::make_tuple(first, second, third, fourth);
}

std::tuple<long,
           double,
           double,
           std::vector<std::tuple<int, double, double, long>>>
deserialize(const std::vector<uint8_t>& buffer)
{

    size_t offset = 0;

    // Deserialize the first long
    long first;
    deserializePrimitive(buffer, offset, first);

    // Deserialize the first double
    double second;
    deserializePrimitive(buffer, offset, second);

    // Deserialize the second double
    double third;
    deserializePrimitive(buffer, offset, third);

    // Deserialize the vector of tuples
    size_t vecSize;
    deserializePrimitive(buffer, offset, vecSize);

    std::vector<std::tuple<int, double, double, long>> vec;
    vec.reserve(vecSize);
    for (size_t i = 0; i < vecSize; ++i) {
        vec.push_back(deserializeTuple(buffer, offset));
    }

    return std::make_tuple(first, second, third, vec);
}

int partition(std::vector<std::tuple<int, double, double, long>>& arr,
              int left,
              int right)
{
    int pivotIdx = right;
    auto pivot = arr[pivotIdx];
    int bar = left - 1;

    for (int i = left; i < right; ++i) {
        if (std::get<2>(arr[i]) < std::get<2>(pivot)) {
            bar++;
            std::swap(arr[bar], arr[i]);
        }
    }
    std::swap(arr[bar + 1], arr[pivotIdx]);
    return bar + 1;
}

// Select the i-th smallest element based on the third element in the tuple
std::tuple<int, double, double, long> select(
  std::vector<std::tuple<int, double, double, long>>& arr,
  int i,
  int left,
  int right)
{
    if (left == right) {
        return arr[right];
    }

    int p = partition(arr, left, right);

    if (p == i) {
        return arr[p];
    } else if (p < i) {
        return select(arr, i, p + 1, right);
    } else {
        return select(arr, i, left, p - 1);
    }
}

void BFPRT(std::vector<std::tuple<int, double, double, long>>& arr, int k)
{
    auto index = select(arr, k, 0, arr.size() - 1);
}

void identifyAbnormal(
  std::vector<std::tuple<int, double, double, long>>& streamList)
{
    int median = streamList.size() / 2;
    BFPRT(streamList, median);
}

// <machineId, score, sumscore, timestamp>
// std::vector<std::tuple<int, double, double, long>> streamList;

// <prevTimestamp, minScore, maxScore, streamList>
// std::tuple<long, double, double, std::vector<>>

int main(int argc, char* argv[])
{
    std::string function = "mo_alert";
    int maxRecords = 36;
    int maximumStateLength = 32 + 28 * maxRecords;
    const double dupper = std::sqrt(2);

    // get the inputMap (inputdata)
    auto inputMap = faasm::getMapInput();
    int machineId = std::stoi(inputMap["machineId"]);
    double score = std::stod(inputMap["score"]);
    double sumScore = std::stod(inputMap["sumScore"]);
    long timestamp = std::stol(inputMap["timestamp"]);

    // Get the Function
    long prevTimestamp = 0;
    double minScore = std::numeric_limits<double>::max();
    double maxScore = 0;
    // <machineId, score, sumscore, timestamp>
    std::vector<std::tuple<int, double, double, long>> streamList;
    faasmLockStateWriteSize(function.data(), maximumStateLength);
    faasmPullState(function.data(), maximumStateLength);
    std::vector<uint8_t> stateBuffer(maximumStateLength);
    faasmReadState(function.data(), stateBuffer.data(), maximumStateLength);
    if (!faasm::all_elements_are_zero(stateBuffer)) {
        auto functionState = deserialize(stateBuffer);
        prevTimestamp = std::get<0>(functionState);
        minScore = std::get<1>(functionState);
        maxScore = std::get<2>(functionState);
        streamList = std::get<3>(functionState);
    }

    // Print the prevTimestamp, minScore, maxScore, streamList.size()
    // std::ostringstream oss;
    // oss << "prevTimestamp: " << prevTimestamp << " minScore: " << minScore
    //     << " maxScore: " << maxScore
    //     << " streamList.size(): " << streamList.size();
    // std::cout << oss.str() << std::endl;

    // Process Logic
    if (timestamp > prevTimestamp) {
        if (streamList.size() != 0) {
            // Print the streamList sumScore before reorder
            // std::ostringstream oss1;
            // oss1 << "Before Reorder: ";
            // for (auto streamProfile : streamList) {
            //     oss1 << std::get<2>(streamProfile) << " ";
            // }
            // std::cout << oss1.str() << std::endl;
            identifyAbnormal(streamList);
            // Print the streamList sumScore after reorder
            // std::ostringstream oss2;
            // oss2 << "After Reorder: ";
            // for (auto streamProfile : streamList) {
            //     oss2 << std::get<2>(streamProfile) << " ";
            // }
            // std::cout << oss2.str() << std::endl;
            std::string output;
            int median = streamList.size() / 2;
            int minScoreTemp = std::get<2>(streamList[0]);
            int medianScoreTemp = std::get<2>(streamList[median]);
            for (auto streamProfile : streamList) {
                double streamScore = std::get<2>(streamProfile);
                double dataScore = std::get<1>(streamProfile);
                bool isAbnormal = false;
                if ((streamScore > 2 * medianScoreTemp - minScoreTemp) &&
                    (streamScore > minScoreTemp + 2 * dupper)) {
                    if (dataScore > 0.1 + minScore) {
                        isAbnormal = true;
                    }
                }

                // Print the streamScore and dataScore of the data
                // std::ostringstream oss3;
                // oss3 << "Stream Score: " << streamScore
                //      << " Data Score: " << dataScore;
                // std::cout << oss3.str() << std::endl;
                if (isAbnormal) {
                    output += std::to_string(std::get<0>(streamProfile)) + " " +
                              std::to_string(std::get<3>(streamProfile)) + " ";
                }
            }
            if (output.size() > 0) {
                faasmSetOutput(output.c_str(), output.size());
            }

            streamList.clear();
            double minScore = std::numeric_limits<double>::max();
            maxScore = 0;
        }
        prevTimestamp = timestamp;
    }

    if (score > maxScore) {
        maxScore = score;
    }
    if (score < minScore) {
        minScore = score;
    }
    streamList.push_back(
      std::make_tuple(machineId, score, sumScore, timestamp));

    auto dataTuple =
      std::make_tuple(prevTimestamp, minScore, maxScore, streamList);
    std::vector<uint8_t> stateBufferUpdated = serialize(dataTuple);
    faasmWriteState(
      function.data(), stateBufferUpdated.data(), stateBufferUpdated.size());
    faasmPushState(function.data());
    faasmUnlockStateWrite(function.data());
    return 0;
}
