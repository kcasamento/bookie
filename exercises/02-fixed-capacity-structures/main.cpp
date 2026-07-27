// Exercise 02: see README.md in this directory for requirements.
// Replace this stub with your implementation.

#include <array>
#include <cstddef>
#include <format>
#include <iostream>
#include <optional>

enum class Side {
  NotSet,
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

  Order() : id(0), price(0), quantity(0), side(Side::NotSet) {}

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

template <typename T, size_t Capacity>
struct OrderPool {
  public:

  OrderPool() {
    head_ = pool_.data();
    for (auto* p = pool_.data(); p != pool_.data() + pool_.size(); p += sizeof(T)) {

      // next is a variable that points to an address
      // i.e., a pointer to a byte (inside this array)
      // memcpy needs that value.  so we need to pass the 
      // address location of the variable next
      auto* next = p + sizeof(T);
      if (next == pool_.data() + pool_.size()) {
        next = nullptr;
      }

      // p is a pointer that points to a byte which in this case
      // is the location in pool where we want to change the value
      // so effectively memcpy says the value that p points to 
      // is going to be replaced with the value that &next points to
      // and since next points to the junk value in the array we actually 
      // need to pass another layer of indirection so &next's value is not a pointer address
      // that points to the junk value
      // &next -> next -> [_]
      std::memcpy(p, &next, sizeof(next));
    }

    // at this point, pool is an array of address's where each slot has the value
    // of the address to the next slot, and the last slot is a nullptr
    // head points to the address of slot 0 which is where our first free T will be
  }

  template <typename... Args>
  T* allocate(Args&&... args) {
    // slot points to the same value
    // head_ points to
    std::byte* slot = head_;

    if (slot == nullptr) {
      return nullptr;
    }

    std::byte* newHead;
    std::memcpy(&newHead, slot, sizeof(newHead));

    // head_ points to the value
    // that newHead points to
    head_ = newHead;
    
    return new (slot) T(std::forward<Args>(args)...);
  };

  void deallocate(T* o) {
    o->~T();
    std::byte* freed = reinterpret_cast<std::byte*>(o);

    memcpy(freed, &head_, sizeof(head_));

    head_ = freed;
    return;
  }

  private:
    std::byte* head_;
    alignas(alignof(T)) std::array<std::byte, Capacity * sizeof(T)> pool_;
};

template <typename T, size_t Capacity>
struct RingBuffer {
  public:
   RingBuffer() : count_(0), head_(0) {}

   bool push(Order o) {
    if (buf_.size() == count_) {
      return false;
    }

    size_t tail_idx = (head_ + count_) % Capacity;
    buf_[tail_idx] = std::move(o);

    ++count_;
    return true;
   }

   std::optional<T> pop() {
    if (count_ == 0) {
      return std::nullopt;
    }

    T val = std::move(buf_[head_]);
    head_ = (head_ + 1) % Capacity;
    --count_;
    return val;
   }
  

  private:
    size_t count_;
    size_t head_;
    std::array<std::optional<T>, Capacity> buf_;
};

int main() {
  // RingBuffer<Order, 2> b;
  // Order o1 = Order(1, 10, 1, Side::Buy);
  // Order o2 = Order(2, 10, 1, Side::Sell);
  // Order o3 = Order(3, 10, 1, Side::Buy);
  //
  // bool ok = b.push(o1);
  // std::cout << "push(o1)=" << ok << "\n";
  //
  // ok = b.push(o2);
  // std::cout << "push(o2)=" << ok << "\n";
  //
  // ok = b.push(o3);
  // std::cout << "push(o3)=" << ok << "\n";


  OrderPool pool = OrderPool<Order, 2>();
  Order* o1 = pool.allocate(1, 10, 1, Side::Buy);
  std::cout << "o1=" << o1->id << "\n";
  Order* o2 = pool.allocate(2, 10, 1, Side::Buy);
  std::cout << "o2=" << o2->id << "\n";
  Order* o3 = pool.allocate(3, 10, 1, Side::Buy);
  if (o3 == nullptr) {
    std::cout << "success\n";
  }
  std::cout << "o3=" << o3 << "\n";

  pool.deallocate(o1);
  o3 = pool.allocate(3, 10, 1, Side::Buy);
  if (o3 != nullptr) {
    std::cout << "success\n";
  }
  std::cout << "o3=" << o3->id << "\n";




  return 0;
}
