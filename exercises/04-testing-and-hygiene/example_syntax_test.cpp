// Worked example of Catch2 syntax only — nothing here tests OrderBook.
// Delete this file once you've written real tests against your own code;
// it exists purely so you have a compiling reference for the macros below.
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

namespace {
int doubleIt(int x) { return x * 2; }
}

TEST_CASE("doubleIt doubles a positive number", "[example]") {
  REQUIRE(doubleIt(21) == 42);
}

TEST_CASE("doubleIt behaves across several inputs", "[example]") {
  SECTION("zero") {
    CHECK(doubleIt(0) == 0);
  }
  SECTION("negative") {
    CHECK(doubleIt(-5) == -10);
  }
}

TEST_CASE("doubleIt over a generated range of inputs", "[example][generator]") {
  int x = GENERATE(1, 2, 3, 100, -7);
  CHECK(doubleIt(x) == x * 2);
}
