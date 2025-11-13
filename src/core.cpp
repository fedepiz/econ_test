#include "core.h"
#include <algorithm>
#include <arena.h>
#include <cstring>
#include <string>
namespace arena {

using Page = Arena::Page;

static inline
bool HasFreeSpace(const Page* page, usize num_bytes) {
  return page && (page->capacity - page->size) >= num_bytes;
}

static inline
Page* MakePage(usize capacity, Page* next) {
  Page* page = new Page;
  page->capacity = capacity;
  // Prepare buffer
  page->buffer = new byte[capacity];
  memset(page->buffer, 0, page->capacity);
  page->next = next;
  return page;
}

byte* Arena::AllocateBytes(usize num_bytes) {
  if (!HasFreeSpace(this->page_chain, num_bytes)) {
    usize page_size = std::max(this->min_page_size, num_bytes);
    this->page_chain = MakePage(page_size, this->page_chain);
  }
  // Allocate
  auto* page = this->page_chain;
  byte* output = page->buffer + page->size;
  page->size += num_bytes;
  return output;
}

void Arena::Reset() {
  auto* page = this->page_chain;
  while (page) {
    auto* next = page->next;
    // Release page buffer
    if (page->buffer) {
      delete[] page->buffer;
    }
    delete page;
    page = next;
  }
  this->capacity = 0;
  this->page_chain = nullptr;
}

const char* Arena::AllocateString(const std::string& str) {
  usize size = str.length() + 1;
  char* out = (char*)this->AllocateBytes(size);
  strcpy(out, str.c_str());
  return out;
}

Arena::~Arena() {
  this->Reset();
}
}

