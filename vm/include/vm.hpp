#pragma once

#include <initializer_list>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "component.hpp"
#include "thread.hpp"

namespace ailoy {

void vm_start(const std::string &url,
              std::span<std::shared_ptr<const module_t>> mods,
              const std::string &name = "default_vm");

void vm_stop(const std::string &url, const std::string &name = "default_vm");

} // namespace ailoy
