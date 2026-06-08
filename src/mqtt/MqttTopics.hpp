#pragma once

#include <cstddef>
#include <cstdio>

#include "config.hpp"

namespace vending::mqtt {

inline void formatActionsTopic(char *buffer, size_t bufferSize) {
    std::snprintf(buffer, bufferSize, "vending/%d/actions", config::MachineId);
}

inline void formatEventsTopic(char *buffer, size_t bufferSize) {
    std::snprintf(buffer, bufferSize, "vending/%d/events", config::MachineId);
}

inline void formatStatusTopic(char *buffer, size_t bufferSize) {
    std::snprintf(buffer, bufferSize, "vending/%d/status", config::MachineId);
}

}  // namespace vending::mqtt
