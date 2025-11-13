#include <array>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <pool.h>
#include <simulation.h>

namespace simulation {
using namespace arena;

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

template <typename T> unique_vector<T> MakeUniqueVec() {
  return std::make_unique<std::vector<T>>();
}

static bool IsValid(const Pop& pop) { return pop.generation % 2 == 1; }
static bool IsValid(const Building& building) {
  return building.generation % 2 == 1;
}
static bool IsValid(const Location& location) {
  return location.generation % 2 == 1;
}
static bool IsValid(const Country& country) {
  return country.generation % 2 == 1;
}

const char* StringAlloc(Strings& container, std::string&& data) {
  container.push_back(std::move(data));
  return container.back().c_str();
}

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
  pop.generation += 1;
  pop.type = pop_type;
  pop.size = size;

  // Add the pop to the list of pops at location
  pop.location = location;
  location->pops_at_location->push_back(&pop);

  return &pop;
}

static inline const BuildingType* LookupBuildingType(
    BuildingTypes& types, std::string_view tag) {
  auto lookup = Lookup(types, tag);
  if (!lookup.found) {
    std::cout << "Invalid tag for building_type'" << tag << "'" << std::endl;
  }
  const auto* result = &types[lookup.idx];
  assert(result);
  return result;
}

static inline Building* BuildingInit(Sim& sim, std::string_view type_tag, std::string_view location_tag, i64 size) {

  auto* building_type = LookupBuildingType(sim.building_types, type_tag);
  auto* location = LookupLocation(sim.locations, location_tag);

  if (!location) {
    return nullptr;
  }

  auto& building = sim.buildings.Allocate();
  building.generation += 1;
  building.type = building_type;
  building.size = size;

  // Add the building to the list of buildings at location
  building.location = location;
  location->buildings_at_location->push_back(&building);

  return &building;
}

struct TagAndName {
  std::string_view tag{"NULL"};
  std::string_view name{"UNNAMED"};
};

static inline Location* LocationInit(Sim& sim, TagAndName tag_name, V2 coords) {
  auto& location = sim.locations.Allocate();
  location.generation++;
  location.tag = StringAlloc(sim.strings, std::string(tag_name.tag));
  location.name = StringAlloc(sim.strings, std::string(tag_name.name));
  location.coords = coords;
  location.pops_at_location = MakeUniqueVec<Pop*>();
  location.buildings_at_location = MakeUniqueVec<Building*>();
  return &location;
}

static inline Country* CountryInit(Sim& sim, TagAndName tag_name, RGB color) {
  auto& country = sim.countries.Allocate();
  country.generation++;
  country.tag = StringAlloc(sim.strings, std::string(tag_name.tag));
  country.name = StringAlloc(sim.strings, std::string(tag_name.name));
  country.color = color;
  country.owned_locations = MakeUniqueVec<Location*>();
  return &country;
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
    return BuildingType{
        .tag = tag,
        .name = name,
        .inputs = VectorInit(sim.good_types),
        .output = VectorInit(sim.good_types)};
  };

  {
    // Null building
    sim.building_types.push_back(make_type("null_building", "NULL_BUILDING"));
  }
  {
    // Farm
    auto type = make_type("farm", "Farm");
    SetVectorValues(type.output, sim.good_types, {{"wheat", 2.0}});
    sim.building_types.push_back(type);
  }
}

template <typename T> static inline T& assume_valid(T* ptr) {
  assert(ptr);
  return *ptr;
}

static void ChangeLocationOwner(
    Sim& sim, Country* country, Location* location) {
  assert(!location->owner_country);
  country->owned_locations->push_back(location);
  location->owner_country = country;
}

void Init(Sim& sim) {
  using namespace init_sim;
  InitGoodTypes(sim);
  InitPopTypes(sim);
  InitBuildingTypes(sim);

  sim.pops = std::move(Pops("Pops", 2048));
  sim.buildings = std::move(Buildings("Buildings", 2048));
  sim.countries = std::move(Countries("Countries", 256));
  sim.locations = std::move(Locations("Locations", 1024));

  {
    auto tag_name = TagAndName{
        .tag = "italy",
        .name = "Italy",
    };
    auto color = RGB{40, 255, 40};
    auto* country = CountryInit(sim, tag_name, color);
    assume_valid(country);
    sim.player.country = country;
  }

  {
    auto tag_name = TagAndName{
        .tag = "rome",
        .name = "Rome",
    };
    auto* location = LocationInit(sim, tag_name, {0.0, 0.0});
    assume_valid(location);
    ChangeLocationOwner(sim, sim.player.country, location);
  }

  {
    PopInit(sim, "peasants", "rome", 200);
    PopInit(sim, "burghers", "rome", 100);
    BuildingInit(sim, "farm", "rome", 1);
  }
}

static inline void AdvanceDate(Date& date) {
  auto old_date = date.epoch;
  date.epoch++;
  assert(date.epoch > old_date);
}

void Tick(Sim& sim, const simulation::TickRequest& request) {
  if (request.advance_time) {
    AdvanceDate(sim.date);
  }
}

List<MapItem> ViewMapItems(const Sim& sim, Arena& arena) {
  auto list = arena::List<MapItem>(&arena);

  for (const auto& location : sim.locations) {
    if (!IsValid(location)) {
      continue;
    }
    MapItem item;
    if (location.owner_country) {
      item.color = location.owner_country->color;
    }
    item.id = EntityId {
        .kind = EntityIdKind::Location,
        .handle = (void*)&location,
        .generation = location.generation,
    };
    item.name = location.name;
    item.coords = location.coords;
    item.size = 2.0f;
    list.Push(item);
  }

  return list;
}

static inline const char* PopString(ExtractCtx& ctx) {
  auto string = ctx.ss.str();
  const auto* result = ctx.arena.AllocateString(string);
  // Clearing a stringstream, because C++ is a bit silly...
  ctx.ss.str("");
  ctx.ss.clear();
  return result;
}

static inline Object* NewObject(ExtractCtx& ctx) {
  return ctx.arena.Allocate<Object>(&ctx.arena);
}

template <typename T>
const char*  Write(ExtractCtx& ctx, T value) {
  ctx.ss << value;
  return PopString(ctx);
}

static inline Object* Info(ExtractCtx& ctx, const Pop& pop) {
  auto* obj = NewObject(ctx);
  obj->strings.Set(Field::Name, pop.type->name.c_str());
  obj->strings.Set(Field::Size, Write(ctx, pop.size));
  return obj;
}

static inline Object* Info(ExtractCtx& ctx, const Building& building) {
  auto* obj = NewObject(ctx);
  obj->id = EntityId {
    .kind = EntityIdKind::Building,
    .generation = building.generation,
    .handle = (&building)
  };
  obj->strings.Set(Field::Name, building.type->name.c_str());
  obj->strings.Set(Field::Size, Write(ctx, building.size));
  return obj;
}

static inline void Extract(
    ExtractCtx& ctx, Object& obj, const Location& location) {
  obj.strings.Set(Field::Name, location.name);

  {
    auto* name = "No country";
    if (auto* country = location.owner_country) {
      obj.strings.Set(Field::Country, country->name);
    }
  }

  // Pops
  {
    auto list = List<Object*>(&ctx.arena);
    for (const auto* pop : *location.pops_at_location) {
      list.Push(Info(ctx, *pop));
    }
    obj.lists.Set(Field::Pops, list);
  }

  {
    // Buildings
    auto list = List<Object*>(&ctx.arena);
    for (const auto* item : *location.buildings_at_location) {
      list.Push(Info(ctx, *item));
    }
    obj.lists.Set(Field::Buildings, list);
  }
}

static inline
void Extract(ExtractCtx& ctx, Object& obj, const Building& building) {
  obj.strings.Set(Field::Name, building.type->name.c_str());
  obj.strings.Set(Field::Size, Write(ctx, building.size));
}

Object* Extract(ExtractCtx& ctx, EntityId id) {
  Object* obj = NewObject(ctx);
  obj->id = id;
  if (!id.IsValid()) {
    obj->strings.Set(Field::Name, "INVALID");
    return obj;
  }

  switch (id.kind) {
  case EntityIdKind::Location:
    Extract(ctx, *obj, *(Location*)id.handle);
    break;
  case EntityIdKind::Building:
    Extract(ctx, *obj, *(Building*)id.handle);
    break;
  case EntityIdKind::INVALID:
    break;
  }
  return obj;
}
} // namespace simulation
