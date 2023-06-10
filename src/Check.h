#pragma once

#include "VulkanHeader.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "fmt/core.h"
#pragma clang diagnostic pop

#if defined(_NDEBUG)
    #define ASSERT(exp, ...)

    #define CHECK(exp, ...)                                                    \
        do {                                                                   \
            if (!(exp)) {                                                      \
                fmt::println(                                                  \
                    "{} check failed: {}", #exp, fmt::format(__VA_ARGS__));    \
                std::terminate();                                              \
            }                                                                  \
        } while (false)
#else
    #define ASSERT(exp, ...)                                                   \
        do {                                                                   \
            if (!(exp)) {                                                      \
                fmt::println("{} asserstion failed: {}", #exp,                 \
                    fmt::format(__VA_ARGS__));                                 \
                __builtin_debugtrap();                                         \
            }                                                                  \
        } while (false)

    #define CHECK(exp, ...)                                                    \
        do {                                                                   \
            if (!(exp)) {                                                      \
                fmt::println(                                                  \
                    "{} check failed: {}", #exp, fmt::format(__VA_ARGS__));    \
                __builtin_debugtrap();                                         \
            }                                                                  \
        } while (false)
#endif

#define VK_CHECK(result, object, call)                                         \
    do {                                                                       \
        std::tie(result, object) = call;                                       \
        if (result != vk::Result::eSuccess) {                                  \
            fmt::println("Vulkan call failed: {} returned {}", #call,          \
                vk::to_string(result));                                        \
            __builtin_debugtrap();                                             \
        }                                                                      \
    } while (false)
