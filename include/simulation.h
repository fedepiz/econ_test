#ifndef SIMULATION_H
#define SIMULATION_H
#include <arena.h>
#include <pool.h>

#include <sstream>
#include <vector>
#include <deque>
#include <string>
#include <vector>

namespace simulation {
using namespace arena;

template <typename T> using unique_vector = std::unique_ptr<std::vector<T>>;

struct Date {
  u64 epoch{0};
};

template <typename T, typename V> class Vector {
  std::vector<V> entries;

public:
  Vector() = default;

  void Init(const std::vector<T>& definition, V value) {
    this->entries.resize(definition.size(), value);
  }

  void Init(const std::vector<T>& definition) {
    this->entries.resize(definition.size(), {});
  }

  V& operator[](T::Id id) { return (*this)[id.idx]; }

  const V& operator[](T::Id id) const { return (*this)[id.idx]; }

  V& operator[](usize idx) {
    assert(idx < this->entries.size());
    return this->entries[idx];
  }

  const V& operator[](usize idx) const {
    assert(idx < this->entries.size());
    return this->entries[idx];
  }
};

template <typename K> using NumVector = Vector<K, f64>;

struct Id {
  usize idx{0};
};

struct GoodType {
  using Id = Id;
  Id id;
  std::string tag;
  std::string name;
  f64 price;
};

using GoodTypes = std::vector<GoodType>;

struct PopType {
  Id id;
  std::string tag;
  std::string name;
  NumVector<GoodType> demand;
};

using PopTypes = std::vector<PopType>;

struct BuildingType {
  Id id;
  std::string tag;
  std::string name;
  NumVector<GoodType> inputs;
  NumVector<GoodType> output;
};


using BuildingTypes = std::vector<BuildingType>;

struct Pop;
struct Building;
struct Location;
struct Country;

struct Pop {
  u64 generation{0};
  const PopType* type{nullptr};
  i64 size{0};

  // Location of pop
  Location* location{nullptr};

  // Pop at location linked list
  Pop* location_chain_next{nullptr};
};

using Pops = Pool<Pop>;

struct Building {
  u64 generation{0};
  const BuildingType* type{nullptr};
  i64 size{0};

  // Location of pop
  Location* location{nullptr};
};

using Buildings = Pool<Building>;

static const char* DEFAULT_STRING = "UNSET";

struct V2 {
  f32 x{0.0};
  f32 y{0.0};
};

struct Location {
  u64 generation{0};
  const char* tag{DEFAULT_STRING};
  const char* name{DEFAULT_STRING};
  V2 coords;
  unique_vector<Pop*> pops_at_location{nullptr};
  unique_vector<Building*> buildings_at_location{nullptr};
  Country* owner_country{nullptr};
};

using Locations = Pool<Location>;

struct RGB {
  u8 r{0};
  u8 g{0};
  u8 b{0};
};

struct Country {
  u64 generation{0};
  const char* tag{DEFAULT_STRING};
  const char* name{DEFAULT_STRING};
  RGB color;
  unique_vector<Location*> owned_locations{nullptr};
};

using Countries = Pool<Country>;

using Strings = std::deque<std::string>;

struct Player {
  Country* country{nullptr};
};

struct Sim {
  Date date;
  // Common semi-static data
  GoodTypes good_types;
  PopTypes pop_types;
  BuildingTypes building_types;
  Strings strings;
  // Entity Pools
  Pops pops;
  Buildings buildings;
  Locations locations;
  Countries countries;
  // Player information
  Player player;
};

struct TickRequest {
  bool advance_time{false};
};

void Init(Sim& sim);

void Tick(Sim& sim, const TickRequest& req);

enum class EntityIdKind {
  Location,
  Building,
  INVALID,
};

struct EntityId {
  EntityIdKind kind{EntityIdKind::INVALID};
  const void* handle{nullptr};
  u64 generation{0};

  static EntityId Null() {
    return {};
  }

  private:
    template <typename T>
    auto GenerationOf() const {
      return ((const T*)this->handle)->generation;
    }
  public:

  bool IsValid() const {
    if (this->kind == EntityIdKind::INVALID || !this->handle) {
      return false;
    }

    // Check generation matches
    u64 generation = 0;
    switch (this->kind) {
      case EntityIdKind::Location:
        generation = GenerationOf<Location>();
        break;
      case simulation::EntityIdKind::Building:
        generation = GenerationOf<Building>();
      case EntityIdKind::INVALID:
        break;
    }
    return this->generation == generation;
  }

  bool operator==(const EntityId& other) const = default;
};

struct MapItem {
  EntityId id;
  const char* name{""};
  V2 coords;
  f32 size{0.0};
  RGB color;
};

arena::List<MapItem> ViewMapItems(const Sim& sim, arena::Arena& arena);

enum class Field {
  INVALID,
  Name,
  Size,
  Pops,
  Buildings,
  Country,
};

template <typename T>
class Fields {
private:
  struct Entry {
    Field field{Field::INVALID};
    T payload;
  };

  List<Entry> fields;

  Entry* FindEntry(Field field) const {
    auto it = fields.Iterate();
    while (auto* entry = it.Next()) {
      if (entry->field == field) {
        return entry;
      }
    }
    return nullptr;
  }

  static const usize CAPACITY = 32;

public:
  Fields() = default;
  Fields(Arena* arena) : fields(arena, CAPACITY) {}


  bool Has(Field field) const {
    return this->FindEntry(field);
  }

  T Get(Field field) const {
    const auto* entry = this->FindEntry(field);
    if (!entry) {
      return {};
    }
    return entry->payload;
  }

  const T* TryGet(Field field) const {
    const auto* entry = this->FindEntry(field);
    if (!entry) {
      return nullptr;
    }
    return &entry->payload;
  }

  void Set(Field field, T value) {
    if (auto* entry = this->FindEntry(field)) {
      entry->payload = std::move(value);
    } else {
      this->fields.Push(Entry { .field = field, .payload = value });
    }
  }
};

struct ExtractCtx {
  const Sim& sim;
  Arena& arena;
  std::stringstream ss;
};

struct Object {
  EntityId id;
  Fields<const char*> strings;
  Fields<List<Object*>> lists;

  Object(Arena* arena): strings(arena), lists(arena) {}
};

Object* Extract(ExtractCtx& ctx, EntityId id);
} // namespace simulation
#endif
