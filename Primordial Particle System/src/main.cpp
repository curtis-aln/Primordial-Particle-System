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
Optimize memory access patterns by storing particle data in contiguous arrays rather than separate objects.
Consider using fixed-point arithmetic instead of floating-point for faster calculations on some hardware.
Implement time stepping with adaptive step sizes to balance accuracy and performance.
Use circular buffers for efficient handling of the toroidal space wrapping.
*/


int main()
{
	Simulation().run();
}

// baseline          53fps
// object of arrays  103fps
// not rendering     166fps
// running exe file  200fps


// 20k particles
// baseline          75fps