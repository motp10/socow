#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <cinttypes>
#include <cmath>

namespace ct {

template <typename T, std::size_t SMALL_SIZE>
class SocowVector {
private:
  static_assert(std::is_copy_constructible_v<T>, "T must have a copy constructor");
  static_assert(std::is_nothrow_move_constructible_v<T>, "T must have a non-throwing move constructor");
  static_assert(std::is_copy_assignable_v<T>, "T must have a copy assignment operator");
  static_assert(std::is_nothrow_move_assignable_v<T>, "T must have a non-throwing move assignment operator");
  static_assert(std::is_nothrow_swappable_v<T>, "T must have a non-throwing swap");

  struct ControllBlock {
    size_t ref_counter = 1;
    T* data;

    ~ControllBlock() {
      --ref_counter;
      if (ref_counter == 0) {
        delete[] data;
      }
    }
  };

  ControllBlock* ShareRef(const ControllBlock* cb) {
    cb->ref_counter++;
    return cb;
  }

  static_assert(SMALL_SIZE > 0, "SMALL_SIZE must be positive");

  union SmallObject {
    alignas(T) uint8_t small_data[SMALL_SIZE];
    ControllBlock* data;
  };

  SmallObject small_obj_;
  size_t size_ = 0;
  size_t capacity_ = SMALL_SIZE / sizeof(T);
  bool is_small_ = true;
public:
  SocowVector() = default;
  SocowVector(const SocowVector& other) {
    if (other.is_small_) {
      size_t copied = 0;
      for (size_t i = 0; i < other.size(); ++i) {
        void* curr_ptr = small_obj_.small_data + i * sizeof(T); 
        void* other_ptr = other.small_obj_.small_data + i * sizeof(T);
        try {
          ::new (curr_ptr) T(*reinterpret_cast<T*>(other_ptr));
        } catch (...) {
          for (size_t j = 0; j < copied; ++j) {
            void* ptr = small_obj_.small_data + j * sizeof(T);
            reinterpret_cast<T*>(ptr)->~T();
          }
          throw;
        }
        ++copied;
      }
    } else {
      small_obj_.data = ShareRef(other.small_obj_.data);
    }
    size_ = other.size_;
    capacity_ = other.capacity_;
  };
  SocowVector(SocowVector&& other) noexcept {
    if (other.is_small_) {
      if (is_small_) {
        
      }

    }
  };
  SocowVector& operator=(const SocowVector& other) {
    if (this == &other) { 
        return *this;
    }
    if (other.is_small_) {
      alignas(T) uint8_t tmp_data[SMALL_SIZE];
      for (size_t i = 0; i < other.size(); ++i) {
        void* curr_ptr = tmp_data + i * sizeof(T); 
        void* other_ptr = other.small_obj_.small_data + i * sizeof(T);
        ::new (curr_ptr) T(*reinterpret_cast<T*>(other_ptr));
      }
      for (size_t i = 0; i < std::min(other.size(), size_); ++i) {
        void* curr_ptr = tmp_data + i * sizeof(T); 
        void* other_ptr = other.small_obj_.small_data + i * sizeof(T);
        *reinterpret_cast<T*>(curr_ptr) = std::move(*reinterpret_cast<T*>(other_ptr));
      }
      size_ = other.size_;
      capacity_ = other.capacity_;
    } else {
      delete small_obj_.data;
      small_obj_.data = ShareRef(other.small_obj_.data);
    }
    return *this;
  };
  SocowVector& operator=(SocowVector&& other) noexcept = default;
  ~SocowVector() {
    if (is_small_) {
      for (size_t i = 0; i < size_; ++i) {
        void* ptr = small_obj_.small_data + i * sizeof(T);
        reinterpret_cast<T*>(ptr)->~T();
      }
    } else {
      delete small_obj_.data;
    }
  };

  size_t size() const noexcept {
    return size_;
  }
};

} // namespace ct
