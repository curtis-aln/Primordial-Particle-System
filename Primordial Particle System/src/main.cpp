#include "simulation.h"

/*
todo functionality:
- beacons
- more organic coloring

average frames per cell: 5.01142
max frames in cell: 8236
min frames in cell: 8236
 */

// instead of using "at_border", use the distance between two particles.
// fill a new array with the subtraction of one array by a value. this is used to calculate directions

// Test how often particles change what cell they are in to see how frequently we need to preform grid update


int main()
{
	Random::set_seed(0);
	Simulation().run();
}


/* Performance Testing
Baseline (200k):         11fps
SHG 2d arr -> 1d arr     14fps
cells update particles   28fps
storing local positions  34fps
multithreading           50fps
cos, sin optimization    60fps
*/

/* Performance Testing
Baseline (500k):               10fps
pre-made neighbour arr         13fps
add particles improvements     23fps
add particles improvements #2  40fps
*/