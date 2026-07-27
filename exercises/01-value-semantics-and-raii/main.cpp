#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Exercise 01: see README.md in this directory for requirements.
// Replace this stub with your implementation.


enum class Side {
  Buy,
  Sell,
};

struct Order {
  int id;
  int price;
  int quantity;
  Side side;

  explicit Order(int id_, int price_, int quantity_, Side side_) 
    : id(id_), price(price_), quantity(quantity_), side(side_) {
    std::cout << "ctor(" << id << ")\n"; 
  }

  // Order& is a reference to an already exiting
  // object, not a reference to memory
  // Here id is copied directly..it's an int so it copyies the int
  // If it were a int* it would copy the address 
  Order(const Order& other)
    : id(other.id), price(other.price), quantity(other.quantity), side(other.side) {
    std::cout << "copy-ctor(" << id << ")\n";
  }

  // Order&& is effectivaly a temp object that can get
  // destroyed after the move
  Order(Order&& other) noexcept
    : id(other.id), price(other.price), quantity(other.quantity), side(other.side) {
    std::cout << "move-ctor(" << id << ")\n";
  }

  Order& operator=(const Order& other) {
    id = other.id;
    price = other.price;
    quantity = other.quantity;
    side = other.side;
    std::cout << "copy-assign(" << id << ")\n";
    return *this;
  }

  Order& operator=(Order&& other) {
    id = other.id;
    price = other.price;
    quantity = other.quantity;
    side = other.side;
    std::cout << "move-assign(" << id << ")\n";
    return *this;
  }

  // Destructor - free memory
  ~Order() {
    std::cout << "dtor(" << id << ")\n";
  }
};

class ScopedTimer {
public:
  explicit ScopedTimer(std::string label)
    : label_(std::move(label)), start_(std::chrono::steady_clock::now()) {}

  ~ScopedTimer() {
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    std::cout << label_ << ": " << elapsed.count() << " us\n";
  }

  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;
  ScopedTimer(ScopedTimer&&) = delete;
  ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
  std::string label_;
  std::chrono::steady_clock::time_point start_;
};

int main() {
  Order buyAppl = Order(1, 100, 10, Side::Buy);
  Order sellMsft = Order(2, 50, 30, Side::Sell);
  Order buyAi = Order(3, 50, 30, Side::Buy);

  // organic grow
  std::cout << "organtic vector\n";
  std::vector<Order> o;
  {
    ScopedTimer timer("organic fill");
    o.push_back(buyAppl);
    o.push_back(sellMsft);
    o.push_back(buyAi);
  }

  // reserve grow
  std::cout << "reserve vector\n";
  std::vector<Order> o2;
  o2.reserve(10);
  {
    ScopedTimer timer("reserve fill");
    o2.push_back(buyAppl);
    o2.push_back(sellMsft);
    o2.push_back(buyAi);
  }
  return 0;
}

