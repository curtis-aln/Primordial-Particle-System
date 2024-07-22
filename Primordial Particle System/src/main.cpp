#include "simulation.h"

/*
todo optimization:
- john tampon spatial hash grid
- thread pool


todo functionality:
- beacons
- more organic coloring
 */



int main()
{
	Random::set_seed(0);
	Simulation().run();
}


/* Performance Testing
Baseline (200k):        11fps
SHG 2d arr -> 1d arr    14fps
cells update particles  28fps
*/