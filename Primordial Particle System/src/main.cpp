#include "simulation.h"

/*
todo functionality:
- beacons
- more organic coloring
 */

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
*/