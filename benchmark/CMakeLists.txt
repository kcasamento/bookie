include(FetchContent)

# Google Benchmark's own CMakeLists builds its own test suite (pulling in
# googletest as a second dependency) unless these are turned off first.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.1
  SYSTEM   # don't apply our -Wall/-Wextra/... to a dependency's own source
)
FetchContent_MakeAvailable(benchmark)

add_executable(orderbook_benchmarks order_book_benchmark.cpp)
target_link_libraries(orderbook_benchmarks PRIVATE orderbook benchmark::benchmark_main)
