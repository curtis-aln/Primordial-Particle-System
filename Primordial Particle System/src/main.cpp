#include "simulation.h"


int main()
{
	Random::set_seed(0);
	Simulation simulation;
	simulation.run();
}

// performance testing - 4 million particles - No rendering - 950 sf
// benchmark         12fps
// SPSCQueue usage


// use CMAKE (finally)
// recording sfml directly
// on-screen statistics
// smooth zooming
// option for circular particles


// display mode:
// organisms are circular
// the movement is smoothed by haiving every nth frame a turning frame and the others moving
// bloom

// optimizing rendering
// find the in world positions of the topleft and bottom right corners of the window. find the gridcell id's for each
// for each particle in those gridcells render.

// without rendering optimization: 130fps