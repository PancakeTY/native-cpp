#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include "faasm/serialization.h"

#include <random>
#include <string>
#include <vector>
#include <iostream> 

int main(int argc, char* argv[])
{
    auto inputMap = faasm::getMapInput();
    auto inputSentence = inputMap["sentence"];
    // Print the input Sentence
    // std::cout << inputSentence << std::endl;

    std::string word;
    if (inputSentence.empty()) {
        printf("WordCount Split: No input sentence\n");
        return 0;
    }
    for (char x : inputSentence) {
        if (x == ' ') {
            if (!word.empty()) {
                std::map<std::string, std::string> chainedInput;
                chainedInput["word"] = word;
                std::vector<uint8_t> chainedInputBytes;
                faasm::serializeMap(chainedInputBytes, chainedInput);
                faasmChainNamed("wc_count",
                                chainedInputBytes.data(),
                                chainedInputBytes.size());
                word.clear();
            }
        } else {
            word = word + x;
        }
    }
    if (!word.empty()) {
        std::map<std::string, std::string> chainedInput;
        chainedInput["word"] = word;
        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        faasmChainNamed("wc_count",
                        chainedInputBytes.data(),
                        chainedInputBytes.size());
    }
    return 0;
}