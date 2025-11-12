#ifndef ARENA_H
#define ARENA_H
#include <core.h>
#include <utility>

namespace arena {

class Arena {
public:
  struct Page {
    usize size{0};
    usize capacity{0};
    unsigned char* buffer{nullptr};
    Page* next{nullptr};
  };
private:

  usize capacity{0};
  Page* page_chain{};
  usize min_page_size{4096};

public:
  Arena() = default;
  Arena(const Arena& other) = delete;
  ~Arena();
  
  byte* AllocateBytes(usize num_bytes);
  void Reset();

  template <typename T> T* Allocate() {
    return (T*)this->AllocateBytes(sizeof(T));
  }
};

template <typename T>
class Stream {
public:
  struct Chunk {
    usize length{0};
    usize capacity{0};
    T* buffer{nullptr};
    Chunk* next{nullptr};
  };

  struct Head {
    Chunk chunk;
    usize count{0};
    Chunk* tail{nullptr};
  };

  class Iterator {
  private:
    Chunk* chunk{nullptr};
    usize idx{0};
    usize overall_index{0};
  public:
    Iterator(Chunk* chunk): chunk(chunk) {}
    
    T* Next() {
      if (!this->chunk) { return nullptr; }
      while (this->idx >= this->chunk->length) {
        this->chunk = this->chunk->next;
        if (!this->chunk) { return nullptr; }
        this->idx = 0;
      }
      auto* value = &this->chunk->buffer[this->idx];
      this->idx++;
      this->overall_index++;
      return value;
    }

    usize Index() const {
      return this->overall_index;
    }
  };

private:
  Arena* arena{nullptr};
  Head* head{nullptr};

  static void InitChunk(Arena& arena, Chunk& chunk) {
      const usize CAPACITY = 64;
      chunk = Chunk {
        .length = 0,
        .capacity = CAPACITY,
        .buffer = new T[CAPACITY],
        .next = nullptr,
      };
  }
public:
  Stream() = default;
  Stream(Arena* arena) { this->arena = arena; }
  
  void Push(T value) {
    if (!arena) { return; }
    if (!head) {
      // Create initial chunk
      this->head = this->arena->Allocate<Head>();
      InitChunk(*this->arena, this->head->chunk);
      // Initial chunk has a last, and the last is itself
      this->head->tail = &this->head->chunk;
    }
    // if we have a chunk, then we have a tail
    auto* last = this->head->tail;
      
    // Allocate a new last chunk, if we ran out of chunks
    if (last->length >= last->capacity) {
      auto* new_chunk = this->arena->Allocate<Chunk>();
      InitChunk(*this->arena, *new_chunk);
      last->next = new_chunk;
      this->head->tail = new_chunk;
    }
    // Push into position
    last->buffer[last->length] = std::move(value);
    last->length++;
    // Increase overall count
    this->head->count++;
  }

  Iterator Iterate() {
    return Iterator(&this->head->chunk); 
  }

  usize Length() const {
    return this->head->count;
  }
};
} // namespace arena
#endif
