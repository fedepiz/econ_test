#include <algorithm>
#include <array>
#include <core.h>
#include <initializer_list>
#include <iostream>
#include <pool.h>
#include <string_view>
#include <utility>
#include <vector>

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

struct LookupResult {
  bool found{false};
  usize idx{0};
};

template <typename T>
LookupResult Lookup(const std::vector<T>& definitions, std::string_view tag) {
  for (usize idx = 0; idx < definitions.size(); ++idx) {
    const auto& entry = definitions[idx];
    if (entry.tag == tag) {
      return {.found = true, .idx = idx};
    }
  }
  return {};
}

template <typename T>
void SetVectorValues(NumVector<T>& vector, const std::vector<T>& definitions,
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

struct Pop {
  PopType* type{nullptr};
  f64 size{0.0};

  bool IsValid() const { return this->type; }
};

using Pops = Pool<Pop>;

struct Building {
  BuildingType* type{nullptr};
  f64 size{0.0};

  bool IsValid() const { return this->type; }
};

using Buildings = Pool<Building>;

struct MarketGood {
  f64 price{0.0};
  f64 stock{0.0};
  f64 supply{0.0};
  f64 demand{0.0};
};

struct Market {
  Vector<GoodType, MarketGood> goods;
};

Market MarketInit(const GoodTypes& good_types) {
  Market market;
  market.goods.Init(good_types);
  return market;
}

struct Simulation {
  GoodTypes good_types;
  PopTypes pop_types;
  BuildingTypes building_types;
  // Pools
  Pops pops;
  Buildings buildings;

  Market market;
};

static inline Pop& PopInit(
    Simulation& sim, std::string_view type_tag, f64 size) {
  auto lookup = Lookup(sim.pop_types, type_tag);
  if (!lookup.found) {
    std::cout << "Invalid tag for pop_type '" << type_tag << "'" << std::endl;
  }
  auto& pop = sim.pops.Allocate();
  pop.type = &sim.pop_types[lookup.idx];
  pop.size = size;
  return pop;
}

namespace init_sim {

template <typename T> void SetSequentialIds(std::vector<T>& definitions) {
  // Set good ids
  for (usize id = 0; id < definitions.size(); ++id) {
    definitions[id].id = {id};
  }
}

static inline void InitGoodTypes(Simulation& sim) {
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

static inline void InitPopTypes(Simulation& sim) {
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

static inline void InitBuildingTypes(Simulation& sim) {
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

void SimulationInit(Simulation& sim) {
  using namespace init_sim;
  InitGoodTypes(sim);
  InitPopTypes(sim);

  sim.pops = std::move(Pops("Pops", 2048));
  sim.buildings = std::move(Buildings("Buildings", 2048));

  sim.market.goods.Init(sim.good_types);
}

void TickMarket(const Simulation& sim, Market& market) {

  Vector<GoodType, MarketGood> next_market_goods = market.goods;
  for (const auto& good_type : sim.good_types) {
    auto& entry = next_market_goods[good_type.id];
    entry.supply = 0.0;
    entry.demand = 0.0;
  }

  NumVector<GoodType> empty_vec;
  empty_vec.Init(sim.good_types);

  struct Agent {
    Vector<GoodType, f64>* demand{&empty_vec};
    f64 demand_multiple{0.0};
    Vector<GoodType, f64>* supply{&empty_vec};
    f64 supply_multiple{0.0};
    f64 priority{0.0};
  };

  std::vector<Agent> agents;
  agents.reserve(sim.pops.NumAllocated() + sim.buildings.NumAllocated());

  for (const auto& pop : sim.pops) {
    if (!pop.IsValid()) {
      continue;
    }
    Agent agent{
        .demand = &pop.type->demand,
        .demand_multiple = pop.size,
        .supply = &empty_vec,
        .supply_multiple = pop.size,
    };
    agents.push_back(agent);
  }

  for (const auto& building : sim.buildings) {
    if (!building.IsValid()) {
      continue;
    }
    Agent agent{
        .demand = &building.type->inputs,
        .demand_multiple = building.size,
        .supply = &building.type->output,
        .supply_multiple = building.size,
    };
    agents.push_back(agent);
  }

  std::sort(agents.begin(), agents.end(),
      [](const auto& a1, const auto& a2) { return a1.priority > a2.priority; });

  for (const auto& agent : agents) {
    for (const auto& good_type : sim.good_types) {
      auto& entry = next_market_goods[good_type.id];
      entry.supply += (*agent.supply)[good_type.id] * agent.supply_multiple;
      entry.demand += (*agent.demand)[good_type.id] * agent.demand_multiple;
    }
  }

  // Calculate prices
  for (const auto& good_type : sim.good_types) {
    auto& entry = next_market_goods[good_type.id];

    f64 raw_scale = 0.0;
    if (entry.demand != 0.0 || entry.supply != 0.0) {
      raw_scale =
          (entry.demand - entry.supply) / std::max(entry.demand, entry.supply);
    }

    f64 scale = 1.0 + std::min(std::max(raw_scale, -0.75), 0.75);
    entry.price = good_type.price * scale;
  }

  for (const auto& good_type : sim.good_types) {
    const auto& entry = next_market_goods[good_type.id];
    std::cout << good_type.name << " S:" << entry.supply
              << ", D:" << entry.demand << " P:" << entry.price << "$"
              << std::endl;
  }
}

void SimulationTick(Simulation& sim) { TickMarket(sim, sim.market); }

int main() {
  Simulation sim;
  SimulationInit(sim);

  {
    PopInit(sim, "peasants", 1.0);
    PopInit(sim, "burghers", 1.0);
  }

  for (const auto& pop_type : sim.pop_types) {
    std::cout << pop_type.name << std::endl;
    for (const auto& good : sim.good_types) {
      auto amount = pop_type.demand[good.id];
      std::cout << "\t" << good.name << " = " << amount << std::endl;
    }
  }

  for (const auto& pop : sim.pops) {
    if (!pop.IsValid()) {
      continue;
    }
    std::cout << pop.type->name << " " << pop.size << std::endl;
  }

  SimulationTick(sim);

  return 0;
}
