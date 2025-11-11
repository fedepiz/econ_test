#ifndef SIMULATION_H
#define SIMULATION_H
namespace simulation{

class SimulationModule;

SimulationModule* Init();
void DeInit(SimulationModule* module);

struct TickRequest {
  bool advance_time{false};
};

void Tick(SimulationModule& module, const TickRequest& req);

}
#endif
