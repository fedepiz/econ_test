#ifndef POOL_H
#define POOL_H

#include <cassert>
#include <string>
#include <vector>
#include <core.h>
#include <cassert>
#include <iostream>

template <typename T>
class Pool {
private:
  std::vector<T> entries;
  std::vector<bool> check;
  std::vector<T*> free_list;
  usize frontier{0};
  usize num_allocated{0};
  std::string name{"UNNAMED_POOL"};

  struct InRange {
    bool contained{false};
    usize idx{0};
  };

  InRange CheckRange(const T& item) const {
    usize ptr = (usize)&item;
    usize start = (usize)this->entries.data();
    usize end = (usize)(this->entries.data() + this->entries.size());

    bool contained = ptr >= start && ptr < end;
    usize idx = 0;
    if (contained) {
      idx = (ptr - start)/sizeof(T);
    }
    return { contained , idx };
  }

public:
  Pool() = default;
  Pool(std::string_view name, usize capacity) {
    this->name = name;
    this->entries.resize(capacity);
    this->check.resize(capacity, false);
  }

  Pool(const Pool& other) = delete;
  Pool(Pool&& other) = delete;

  Pool& operator=(const Pool& other) = default;
  Pool& operator=(Pool&& other) = default;

  T& Allocate() {
    T* out;
    if (this->free_list.empty()) {
      bool out_of_space = this->frontier >= entries.size();
      if (out_of_space) {
        std::cout << "Pool " << this->name << " out of space!" << std::endl;
        assert(false);
      }
      out = &this->entries[this->frontier++];
    } else {
      out = *this->free_list.rbegin();
      this->free_list.pop_back();
    }
    *out = {};

    auto in_range = this->CheckRange(*out);
    assert(in_range.contained);
    assert(!this->check[in_range.idx]);
    this->check[in_range.idx] = true;

    this->num_allocated++;

    return *out;
  }

  void Deallocate(T& item) {
    auto in_range = this->CheckRange(item);
    assert(in_range.contained);

    assert(this->check[in_range.idx]);
    this->check[in_range.idx] = false;

    this->free_list.push_back(&item);

    assert(this->num_allocated > 0);
    this->num_allocated--;
  }

  usize NumAllocated() const {
    return this->num_allocated;
  }

  auto begin() {
    return this->entries.begin();
  }

  auto end() {
    return this->entries.end();
  }

  auto begin() const {
    return this->entries.begin();
  }

  auto end() const {
    return this->entries.end();
  }
};

#endif
