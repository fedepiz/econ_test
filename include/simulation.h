#ifndef SIMULATION_H
#define SIMULATION_H
#include <arena.h>

namespace simulation{

class SimulationModule;

SimulationModule* Init();
void DeInit(SimulationModule* module);

struct TickRequest {
  bool advance_time{false};
};

void Tick(SimulationModule& module, const TickRequest& req);

namespace view {
struct ViewSimulation {};

struct ViewRequest {};

void View(arena::Arena& arena, const SimulationModule& module, ViewRequest& req);
}

}
#endif
