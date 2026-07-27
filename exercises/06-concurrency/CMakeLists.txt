add_executable(ex06_concurrency main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(ex06_concurrency PRIVATE Threads::Threads)
