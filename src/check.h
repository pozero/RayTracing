#pragma once

#include "vulkan/vulkan_device.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#pragma clang diagnostic pop

#define CHECK(exp, ...)                                                        \
    do {                                                                       \
        if (!(exp)) {                                                          \
            fmt::println(                                                      \
                "{} check failed: {}", #exp, fmt::format(__VA_ARGS__));        \
            std::terminate();                                                  \
        }                                                                      \
    } while (false)

#define VK_CHECK(result, call)                                                 \
    do {                                                                       \
        result = call;                                                         \
        if (result != vk::Result::eSuccess) {                                  \
            fmt::println("Vulkan call failed: {} returned {}", #call,          \
                vk::to_string(result));                                        \
            __builtin_debugtrap();                                             \
        }                                                                      \
    } while (false)

#define VK_CHECK_CREATE(result, object, call)                                  \
    do {                                                                       \
        std::tie(result, object) = call;                                       \
        if (result != vk::Result::eSuccess) {                                  \
            fmt::println("Vulkan call failed: {} returned {}", #call,          \
                vk::to_string(result));                                        \
            __builtin_debugtrap();                                             \
        }                                                                      \
    } while (false)
