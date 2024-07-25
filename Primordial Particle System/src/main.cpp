#include "simulation.h"

/*
todo functionality:
- beacons
- more organic coloring
 */

// instead of using "at_border", use the distance between two particles.
// fill a new array with the subtraction of one array by a value. this is used to calculate directions

// 200k at 30fps
// 100k at 74fps
// 20k at 550fps


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
Baseline (500k):         10fps

*/