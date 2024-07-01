#include "simulation.h"

// V2 Tasks
// Format and Clean-up program preparing for the new code changes
// Fix the constantly moving right bug
// Screen Statistics & Title & Key inputs

// Optimize for 20,000 particles at 144FPS
// optimization: only preform toroidal calculations if the particle's spatial hash box index is on the borders

// V2 main
// Graph of amount of each particle

/*
Implement time stepping with adaptive step sizes to balance accuracy and performance.
Use circular buffers for efficient handling of the toroidal space wrapping.
*/


int main()
{
	Simulation().run();
}

// baseline 30k             - 20fps
// project settings changes - 29fps
// not rendering            - 50fps
// SHG optimization         - 65fps
// pre-computing borders    - 70fps