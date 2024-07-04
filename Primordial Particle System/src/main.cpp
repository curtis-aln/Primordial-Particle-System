#include "simulation.h"

// Screen Statistics & Title & Key inputs
// optimize for 10fps at 100,000 particles
// track number of each particle and store in a file.
// screen translations and zooming


/*
Implement time stepping with adaptive step sizes to balance accuracy and performance.
Use circular buffers for efficient handling of the toroidal space wrapping.


Completity idea:
- turning depends on the directions of the other particles. those facing towards the particle have a higher weight
*/


int main()
{
	Simulation().run();
}

// baseline 100k - 7fps