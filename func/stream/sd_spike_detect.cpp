#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <cmath>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char* argv[])
{
    double spikeThreshold = 0.03; // Replace with actual value
    // get the inputMap
    auto inputMap = faasm::getMapInput();
    
    double movingAverage = std::stod(inputMap["movingAverage"]);
    double temperature = std::stod(inputMap["temperature"]);
    if (std::abs(temperature - movingAverage) >
        spikeThreshold * movingAverage) {
        std::string output = "detected spike";
        faasmSetOutput(output.c_str(), output.size());
        // std::cout << "Moving Average = " << movingAverage
        //           << ", Temperature = " << temperature << ", Status = Detected"
        //           << std::endl;
    } else {
        std::string output = "no spike";
        faasmSetOutput(output.c_str(), output.size());
        // std::cout << "Moving Average = " << movingAverage
        //           << ", Temperature = " << temperature
        //           << ", Status = Undetected" << std::endl;
    }
    return 0;
}