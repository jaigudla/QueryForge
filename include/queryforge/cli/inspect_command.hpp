#pragma once

#include "queryforge/core/result.hpp"

#include <cstddef>
#include <string>

namespace queryforge {

struct InspectOptions {
    std::string input_path;
    std::string schema;
    std::string schema_file;
    std::string delimiter = ",";
    bool infer_schema = true;
    std::size_t preview_rows = 5;
    std::string output = "text";
};

Result<void> execute_inspect(const InspectOptions& options);

}  // namespace queryforge
