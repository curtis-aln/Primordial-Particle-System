#include "settings.h"
#include "simulation/simulation.h"
#include "utils/random.h"
#include <iostream>

// Force discrete GPU on hybrid graphics systems
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main()
{
	if (SimulationSettings::set_Random_seed)
	{
		constexpr unsigned int seed = 0; // Fixed seed for reproducibility
		std::cout << "Setting Random seed to: " << seed << std::endl;
		Random::set_seed(seed);
	}

	Simulation simulation{};
	simulation.run();
}

