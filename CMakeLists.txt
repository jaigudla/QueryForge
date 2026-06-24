cmake_minimum_required(VERSION 3.20)

project(queryforge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(
  ${CMAKE_SOURCE_DIR}/include/queryforge/version.hpp.in
  ${CMAKE_BINARY_DIR}/generated/queryforge/version.hpp
  @ONLY
)

option(QUERYFORGE_BUILD_FUZZ "Build CSV parser fuzz target" OFF)
option(QUERYFORGE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

if(MSVC)
  add_compile_options(/W4)
  if(QUERYFORGE_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(QUERYFORGE_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
endif()

include(FetchContent)

FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.2
)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.4
)
FetchContent_MakeAvailable(CLI11 Catch2)

add_library(queryforge_lib
  src/benchmark/benchmark_stats.cpp
  src/cli/compare_command.cpp
  src/cli/inspect_command.cpp
  src/cli/run_command.cpp
  src/data/csv_line_parser.cpp
  src/data/csv_schema.cpp
  src/data/data_loader.cpp
  src/data/dataset_generator.cpp
  src/data/table.cpp
  src/data/workload.cpp
  src/query/query_filter.cpp
  src/query/linear_scan.cpp
  src/query/strategy.cpp
  src/recommend/recommendation.cpp
  src/report/report.cpp
  src/util/format.cpp
  src/util/memory.cpp
)
target_include_directories(queryforge_lib PUBLIC
  include
  ${CMAKE_BINARY_DIR}/generated
)
target_compile_features(queryforge_lib PUBLIC cxx_std_20)
if(WIN32)
  target_link_libraries(queryforge_lib PUBLIC psapi)
endif()

add_executable(queryforge src/main.cpp)
target_link_libraries(queryforge PRIVATE queryforge_lib CLI11::CLI11)

add_executable(queryforge_tests
  tests/smoke_test.cpp
  tests/benchmark/benchmark_stats_test.cpp
  tests/data/csv_schema_test.cpp
  tests/data/csv_line_parser_test.cpp
  tests/data/data_loader_test.cpp
  tests/data/dataset_generator_test.cpp
  tests/query/query_filter_test.cpp
  tests/query/linear_scan_test.cpp
  tests/query/strategy_test.cpp
  tests/query/sorted_index_test.cpp
  tests/recommend/recommendation_test.cpp
  tests/cli/regression_test.cpp
)
target_link_libraries(queryforge_tests PRIVATE queryforge_lib Catch2::Catch2WithMain)
target_compile_definitions(queryforge_tests PRIVATE QUERYFORGE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

enable_testing()
add_test(NAME unit COMMAND queryforge_tests)
add_test(NAME cli_help COMMAND queryforge --help)
add_test(NAME cli_version COMMAND queryforge --version)
add_test(NAME cli_sample_synthetic
  COMMAND queryforge run --rows 1000 --where symbol=AAPL --strategy all --runs 1 --warmup 0
)
add_test(NAME cli_users_schema_example
  COMMAND queryforge run --input ${CMAKE_SOURCE_DIR}/examples/users.csv --where country=US --where age>=30 --strategy all --runs 1 --warmup 0
)
add_test(NAME cli_game_events_explicit_schema
  COMMAND queryforge run --input ${CMAKE_SOURCE_DIR}/examples/game_events.csv --schema event_id:int64,level:string,frame_time_ms:double,entity_count:int64 --where frame_time_ms>20 --strategy all --runs 1 --warmup 0
)
add_test(NAME cli_inspect_users
  COMMAND queryforge inspect ${CMAKE_SOURCE_DIR}/examples/users.csv --rows 2
)
add_test(NAME cli_json_regression
  COMMAND queryforge run --input ${CMAKE_SOURCE_DIR}/benchmarks/fixtures/small_table.csv --where country=US --strategy linear_scan --runs 1 --warmup 0 --output json
)
add_test(NAME cli_invalid_runs
  COMMAND queryforge run --rows 1000 --where symbol=AAPL --runs 0
)
add_test(NAME cli_invalid_schema
  COMMAND queryforge run --input ${CMAKE_SOURCE_DIR}/examples/users.csv --schema user_id:uuid --where country=US
)
set_tests_properties(cli_invalid_runs PROPERTIES WILL_FAIL TRUE)
set_tests_properties(cli_invalid_schema PROPERTIES WILL_FAIL TRUE)

install(TARGETS queryforge queryforge_lib
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
install(DIRECTORY include/queryforge
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp"
)
install(FILES ${CMAKE_BINARY_DIR}/generated/queryforge/version.hpp
  DESTINATION include/queryforge
)

include(CPack)
set(CPACK_PACKAGE_NAME "queryforge")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;TGZ")

if(QUERYFORGE_BUILD_FUZZ AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_executable(fuzz_csv_parser fuzz/fuzz_csv_parser.cpp src/data/csv_line_parser.cpp)
  target_include_directories(fuzz_csv_parser PRIVATE include)
  target_compile_options(fuzz_csv_parser PRIVATE -fsanitize=fuzzer,address,undefined)
  target_link_options(fuzz_csv_parser PRIVATE -fsanitize=fuzzer,address,undefined)
endif()
