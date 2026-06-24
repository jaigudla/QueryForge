#pragma once

#include "queryforge/core/result.hpp"

#include <string>

namespace queryforge {

struct CompareOptions {
    std::string baseline_path;
    std::string candidate_path;
    std::string output = "text";
};

Result<void> execute_compare(const CompareOptions& options);

}  // namespace queryforge
