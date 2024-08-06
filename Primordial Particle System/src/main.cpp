#include "simulation.h"


int main()
{
	Random::set_seed(0);
	Simulation().run();
}

// performance testing - 4 million particles - No rendering - 950 sf
// benchmark         12fps
// SPSCQueue usage