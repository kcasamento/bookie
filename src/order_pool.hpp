#pragma once

#include <array>
#include <cstddef>

template <typename T, size_t Capacity> struct OrderPool {
public:
  OrderPool() {
    head_ = pool_.data();
    for (auto *p = pool_.data(); p != pool_.data() + pool_.size();
         p += sizeof(T)) {

      // next is a variable that points to an address
      // i.e., a pointer to a byte (inside this array)
      // memcpy needs that value.  so we need to pass the
      // address location of the variable next
      auto *next = p + sizeof(T);
      if (next == pool_.data() + pool_.size()) {
        next = nullptr;
      }

      // p is a pointer that points to a byte which in this case
      // is the location in pool where we want to change the value
      // so effectively memcpy says the value that p points to
      // is going to be replaced with the value that &next points to
      // and since next points to the junk value in the array we actually
      // need to pass another layer of indirection so &next's value is not a
      // pointer address that points to the junk value &next -> next -> [_]
      std::memcpy(p, &next, sizeof(next));
    }

    // at this point, pool is an array of address's where each slot has the
    // value of the address to the next slot, and the last slot is a nullptr
    // head points to the address of slot 0 which is where our first free T will
    // be
  }

  template <typename... Args> T *allocate(Args &&...args) {
    // slot points to the same value
    // head_ points to
    std::byte *slot = head_;

    if (slot == nullptr) {
      return nullptr;
    }

    std::byte *newHead;
    std::memcpy(&newHead, slot, sizeof(newHead));

    // head_ points to the value
    // that newHead points to
    head_ = newHead;

    return new (slot) T(std::forward<Args>(args)...);
  };

  void deallocate(T *o) {
    o->~T();
    std::byte *freed = reinterpret_cast<std::byte *>(o);

    memcpy(freed, &head_, sizeof(head_));

    head_ = freed;
    return;
  }

private:
  std::byte *head_;
  alignas(alignof(T)) std::array<std::byte, Capacity * sizeof(T)> pool_;
};
