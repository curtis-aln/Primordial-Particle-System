#include "simulation.h"

/*
todo functionality:
- beacons
- loading fonts a better way so they can be seen with an executable
- iterations in bottom corner
- Glow
- Live settings updating with Mingw?

average frames per cell: 5.01142
 */

int main()
{
	//Random::set_seed(0);
	Simulation().run();
}
