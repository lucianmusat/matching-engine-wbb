#pragma once

#include "matching_engine.hpp"

#include <vector>
#include <string>

inline std::vector<std::string> run(const std::vector<std::string>& input) {
    MatchingEngine matchingEngine;
    return matchingEngine.processOrders(input);
}