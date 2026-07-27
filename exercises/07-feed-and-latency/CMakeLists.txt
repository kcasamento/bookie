add_executable(ex07_feed_and_latency main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(ex07_feed_and_latency PRIVATE Threads::Threads)
