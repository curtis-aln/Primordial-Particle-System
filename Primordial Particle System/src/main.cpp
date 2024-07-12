#include "simulation.h"

// Screen Statistics & Title & Key inputs
// optimize for 10fps at 100,000 particles
// track number of each particle and store in a file.
// screen translations and zooming

// test if the cells act as a wave. see how far particles travel over time

// optimize for 1000fps at 20,000 particles
// many threads for updating. one thread for rendering


// FEATURES for next run
// video maker from inside MMSVS. take a photo of the simulation every N seconds to make a video
// Iterations in bottom left corner.
// particle angle shown in debug mode
// debug mode optimizations
// cell indexes rendered using an sf::Texture


// instead of getting all of the particles. we just get the program to pass in operation(s) we want to preform on each particle. such as is in range

int main()
{
	//Random::set_seed(0);
	Simulation().run();
}

// 70fps
//