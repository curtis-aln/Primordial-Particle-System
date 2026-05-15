#include "simulation/simulation.h"

// Force discrete GPU on hybrid graphics systems
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main()
{
	Simulation simulation{};
	simulation.run();
}

