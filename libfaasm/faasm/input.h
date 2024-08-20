#ifndef FAASM_INPUT_H
#define FAASM_INPUT_H

#include "faasm/core.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace faasm {
const char* getStringInput(const char* defaultValue);

const std::map<std::string, std::string> getMapInput();

void setStringOutput(const char* val);

int getIntInput();

int* parseStringToIntArray(const char* inStr, int expected);

bool all_elements_are_zero(const std::vector<uint8_t>& buffer);
}

#endif
