add_library(orderbook STATIC order_book.hpp price_level.cpp)
target_include_directories(orderbook PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
