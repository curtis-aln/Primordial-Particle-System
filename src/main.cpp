#include "settings.h"
#include "simulation/simulation.h"
#include "utils/random.h"

// Force discrete GPU on hybrid graphics systems
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main()
{
	if (SimulationSettings::set_Random_seed)
		Random::set_seed(0);

	Simulation simulation{};
	simulation.run();
}

