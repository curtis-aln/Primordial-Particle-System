#include "simulation.h"

// Screen Statistics & Title & Key inputs
// track number of each particle and store in a file.
// screen translations and zooming

// test if the cells act as a wave. see how far particles travel over time
// many threads for updating. one thread for rendering


// FEATURES for next run
// Iterations in bottom left corner.
// particle angle shown in debug mode
// debug mode optimizations
// cell indexes rendered using an sf::Texture

// In ParticlePopulation::update_angles(), the angle wrapping isn't handled, which could lead to numerical instability over time.
// The use of std::fmod for position wrapping in add_particles_to_grid() might produce negative values, potentially causing issues.

// The particle update loop could potentially be parallelized for better performance.
// In PPS_Renderer::updateParticles(), consider using a vertex buffer object for better rendering performance.
// The shader in PPS_Renderer could be optimized to handle more of the particle rendering logic.
// SpatialHashGrid could benefit from using a more cache-friendly data structure for the grid.

// The color mapping in get_color() could be optimized using a lookup table or a more efficient algorithm.


int main()
{
	//Random::set_seed(0);
	Simulation().run();
}
