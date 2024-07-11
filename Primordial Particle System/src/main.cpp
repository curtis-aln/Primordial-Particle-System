#include "simulation.h"

// Screen Statistics & Title & Key inputs
// optimize for 10fps at 100,000 particles
// track number of each particle and store in a file.
// screen translations and zooming

// test if the cells act as a wave. see how far particles travel over time

/*
	1. Learn how an actual spatial hash grid works
	2. Calculate the direction the particle needs to go by averaging all the positions.
		2.1. potentially remove the "angle" container and replace with "direction"
 */


int main()
{
	Simulation().run();
}
