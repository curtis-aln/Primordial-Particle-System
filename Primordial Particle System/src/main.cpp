#include "simulation.h"


int main()
{
	Random::set_seed(0);
	Simulation().run();
}

// performance testing - 4 million particles - No rendering
// benchmark         15fps
// SPSCQueue usage