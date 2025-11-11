#ifndef ARENA_H
#define ARENA_H
#include <core.h>

namespace arena {

struct Page {
  usize size{0};
  usize capacity{0};
  unsigned char* buffer{nullptr};
  Page* next{nullptr};
};

class Arena {
private:
  usize capacity{0};
  Page* page_chain{};
  usize min_page_size{4096};

public:
  byte* AllocateBytes(usize num_bytes);
  void Reset();

  template <typename T> T* Allocate(usize num_bytes) {
    return this->AllocateBytes(sizeof(T));
  }
};
} // namespace arena
#endif
