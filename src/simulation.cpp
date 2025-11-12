#include <array>
#include <deque>

#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <pool.h>
#include <simulation.h>

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

template <typename K>
NumVector<K> VectorInit(const std::vector<K>& definition) {
  NumVector<K> vec;
  vec.Init(definition, 0.0);
  return vec;
}

template <typename T> struct LookupResult {
  bool found{false};
  usize idx{0};
  T* ptr{nullptr};
};

static inline bool LookupStrEq(std::string_view a, std::string_view b) {
  return a == b;
}

static inline bool LookupStrEq(const std::string* a, std::string_view b) {
  return *a == b;
}

template <typename T>
LookupResult<typename T::value_type> Lookup(
    T& definitions, std::string_view tag) {
  usize idx = 0;
  for (auto& entry : definitions) {
    if (LookupStrEq(entry.tag, tag)) {
      return {.found = true, .idx = idx, .ptr = &entry};
    }
    idx++;
  }
  return {};
}

template <typename T>
void SetVectorValues(NumVector<T>& vector, std::vector<T>& definitions,
    std::initializer_list<std::pair<const char*, f64>> names) {
  for (auto [tag, amount] : names) {
    auto lookup = Lookup(definitions, tag);
    if (lookup.found) {
      vector[lookup.idx] = amount;
    } else {
      std::cout << "Invalid item with tag '" << tag << "'" << std::endl;
    }
  }
}

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

struct Location;

struct Pop {
  const PopType* type{nullptr};
  i64 size{0};

  // Location of pop
  Location* location{nullptr};

  // Pop at location linked list
  Pop* location_chain_next{nullptr};
};

using Pops = Pool<Pop>;

struct Building {
  const BuildingType* type{nullptr};
  i64 size{0};

  // Location of pop
  Location* location{nullptr};

  // Pop at location linked list
  Pop* location_chain_next{nullptr};
};

using Buildings = Pool<Building>;

std::string_view DEFAULT_STRING = "UNSET";

struct Location {
  std::string_view tag{DEFAULT_STRING};
  std::string_view name{DEFAULT_STRING};
  bool is_valid{false};
};

using Locations = Pool<Location>;

struct Country {
  std::string_view tag{DEFAULT_STRING};
  std::string_view name{DEFAULT_STRING};
  bool is_valid{false};
};

using Countries = Pool<Country>;

bool IsValid(const Pop& pop) { return pop.type != nullptr; }
bool IsValid(const Building& building) { return building.type != nullptr; }
bool IsValid(const Location& location) { return location.is_valid; }
bool IsValid(const Country& country) { return country.is_valid; }

struct Date {
  u64 epoch{0};
};

using Strings = std::deque<std::string>;

std::string_view StringAlloc(Strings& container, std::string&& data) {
  container.push_back(std::move(data));
  return container.back();
}

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
  Countries country;
};

static inline const PopType* LookupPopType(
    PopTypes& types, std::string_view tag) {
  auto lookup = Lookup(types, tag);
  if (!lookup.found) {
    std::cout << "Invalid tag for pop_type '" << tag << "'" << std::endl;
  }
  return &types[lookup.idx];
}

static inline Location* LookupLocation(
    Locations& locations, std::string_view tag) {
  auto lookup = Lookup(locations, tag);
  if (!lookup.found) {
    std::cout << "Invalid tag for pop_type '" << tag << "'" << std::endl;
  }
  return lookup.ptr;
}

static inline Pop* PopInit(Sim& sim, std::string_view type_tag,
    std::string_view location_tag, i64 size) {
  auto* pop_type = LookupPopType(sim.pop_types, type_tag);
  auto* location = LookupLocation(sim.locations, location_tag);

  if (!location) {
    return nullptr;
  }

  auto& pop = sim.pops.Allocate();
  pop.type = pop_type;
  pop.location = location;
  pop.size = size;
  return &pop;
}

struct TagAndName {
  std::string_view tag{"NULL"};
  std::string_view name{"UNNAMED"};
};

static inline Location* LocationInit(Sim& sim, TagAndName tag_name) {
  auto& location = sim.locations.Allocate();

  location.tag = StringAlloc(sim.strings, std::string(tag_name.tag));
  location.name = StringAlloc(sim.strings, std::string(tag_name.name));

  location.is_valid = true;

  return &location;
}

namespace init_sim {

template <typename T> void SetSequentialIds(std::vector<T>& definitions) {
  // Set good ids
  for (usize id = 0; id < definitions.size(); ++id) {
    definitions[id].id = {id};
  }
}

static inline void InitGoodTypes(Sim& sim) {
  struct Desc {
    const char* tag{""};
    const char* name{""};
    f64 price{1.0};
  };

  const auto descs = std::to_array({
      Desc{
          .tag = "null",
          .name = "NULL_GOOD",
          .price = 0.0,
      },
      Desc{.tag = "wheat", .name = "Wheat", .price = 10.0},
      Desc{.tag = "wool", .name = "Wool", .price = 10.0},
  });

  sim.good_types = {};
  for (const auto& desc : descs) {
    GoodType good = {
        .tag = desc.tag,
        .name = desc.name,
        .price = desc.price,
    };
    sim.good_types.push_back(std::move(good));
  }

  SetSequentialIds(sim.good_types);
}

static inline void InitPopTypes(Sim& sim) {
  auto make_type = [&sim](const char* tag, const char* name) {
    return PopType{
        .tag = tag, .name = name, .demand = VectorInit(sim.good_types)};
  };

  sim.pop_types.push_back(make_type("null_pop", "NULL_POP"));

  {
    // Peasants
    auto type = make_type("peasants", "Peasants");
    SetVectorValues(type.demand, sim.good_types, {{"wheat", 1.0}});
    sim.pop_types.push_back(type);
  }

  {
    // Burghers
    auto type = make_type("burghers", "Burghers");
    SetVectorValues(type.demand, sim.good_types, {{"wheat", 2.0}});
    sim.pop_types.push_back(type);
  }

  SetSequentialIds(sim.pop_types);
}
} // namespace init_sim

static inline void InitBuildingTypes(Sim& sim) {
  auto make_type = [&sim](const char* tag, const char* name) {
    return BuildingType{.tag = tag,
        .name = name,
        .inputs = VectorInit(sim.good_types),
        .output = VectorInit(sim.good_types)};
  };

  {
    // Farm
    auto type = make_type("farm", "Farm");
    SetVectorValues(type.output, sim.good_types, {{"wheat", 2.0}});
  }
}

template <typename T> static inline T& assume_valid(T* ptr) {
  assert(ptr);
  return *ptr;
}

static void SimulationInit(Sim& sim) {
  using namespace init_sim;
  InitGoodTypes(sim);
  InitPopTypes(sim);

  sim.pops = std::move(Pops("Pops", 2048));
  sim.buildings = std::move(Buildings("Buildings", 2048));
  sim.country = std::move(Countries("Countries", 256));
  sim.locations = std::move(Locations("Locations", 1024));

  {
    auto tag_name = TagAndName{
        .tag = "rome",
        .name = "Rome",
    };
    assume_valid(LocationInit(sim, tag_name));
  }

  {
    PopInit(sim, "peasants", "rome", 200);
    PopInit(sim, "burghers", "rome", 100);
  }
}

static inline void AdvanceDate(Date& date) {
  auto old_date = date.epoch;
  date.epoch++;
  assert(date.epoch > old_date);
}

static inline void SimulationTick(
    Sim& sim, const simulation::TickRequest& request) {
  if (request.advance_time) {
    AdvanceDate(sim.date);
  }

  for (const auto& pop : sim.pops) {
    if (!IsValid(pop))
      continue;
    std::cout << pop.type->name << "(" << pop.size << ")" << " in "
              << pop.location->name << std::endl;
  }
}

namespace simulation {
struct SimulationModule {
  Sim sim;
};

SimulationModule* Init() {
  auto module = new SimulationModule;
  SimulationInit(module->sim);
  return module;
}

void DeInit(SimulationModule* module) {
  if (!module) {
    return;
  }
  module->sim = {};
}

void Tick(SimulationModule& module, const TickRequest& request) {
  std::string test;
  SimulationTick(module.sim, request);
}
} // namespace simulation

namespace simulation {
namespace view {
using namespace arena;

void View(Arena& arena, const SimulationModule& module, ViewRequest& req) {
  
}

}
} // namespace simulation
