cmake_minimum_required(VERSION 3.20)

project(queryforge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
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
  src/data/data_loader.cpp
  src/data/dataset_generator.cpp
  src/query/query_filter.cpp
  src/query/linear_scan.cpp
  src/query/strategy.cpp
  src/recommend/recommendation.cpp
  src/report/report.cpp
  src/util/format.cpp
  src/cli/run_command.cpp
)
target_include_directories(queryforge_lib PUBLIC include)
target_compile_features(queryforge_lib PUBLIC cxx_std_20)

add_executable(queryforge src/main.cpp)
target_link_libraries(queryforge PRIVATE queryforge_lib CLI11::CLI11)

add_executable(queryforge_tests
  tests/smoke_test.cpp
  tests/benchmark/benchmark_stats_test.cpp
  tests/data/data_loader_test.cpp
  tests/data/dataset_generator_test.cpp
  tests/query/query_filter_test.cpp
  tests/query/linear_scan_test.cpp
  tests/query/strategy_test.cpp
  tests/recommend/recommendation_test.cpp
)
target_link_libraries(queryforge_tests PRIVATE queryforge_lib Catch2::Catch2WithMain)

enable_testing()
add_test(NAME unit COMMAND queryforge_tests)
add_test(NAME cli_help COMMAND queryforge --help)
add_test(NAME cli_sample_synthetic
  COMMAND queryforge run --rows 1000 --where symbol=AAPL --strategy all --runs 1 --warmup 0
)
add_test(NAME cli_invalid_runs
  COMMAND queryforge run --rows 1000 --where symbol=AAPL --runs 0
)
set_tests_properties(cli_invalid_runs PROPERTIES WILL_FAIL TRUE)
