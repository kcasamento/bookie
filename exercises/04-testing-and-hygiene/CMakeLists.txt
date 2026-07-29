include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
  SYSTEM   # don't apply our -Wall/-Wextra/... to a dependency's own source
)
FetchContent_MakeAvailable(Catch2)

# SYSTEM (above) only affects how OUR code sees Catch2's headers — it doesn't stop
# our own -Wall/-Wextra/... from applying while Catch2's OWN .cpp files get compiled,
# since add_compile_options() is a directory-scoped setting that CMake still applies
# to every target defined in a subdirectory it pulls in. Clear it explicitly instead.
set_target_properties(Catch2 Catch2WithMain PROPERTIES COMPILE_OPTIONS "")

# add_executable(ex04_orderbook_tests order_book_test.cpp order_book_property_test.cpp)
# target_link_libraries(ex04_orderbook_tests PRIVATE orderbook Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
# catch_discover_tests(ex04_orderbook_tests)
