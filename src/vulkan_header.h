#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
// remove the constructors of create info classes to use designated initializers
#define VULKAN_HPP_NO_CONSTRUCTORS 1
#define VULKAN_HPP_NO_EXCEPTIONS 1
// enable default function pointer dispatch loader
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR 1
#include "vulkan/vulkan.hpp"
#pragma clang diagnostic pop
