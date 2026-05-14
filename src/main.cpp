#include "simulation.h"


int main()
{
	Simulation simulation{};
	simulation.run();
}

// performance testing - 4 million particles - No rendering - 950 sf
// benchmark         12fps
// SPSCQueue usage


// create a load of cpp files for the header files, organise file and folder structure
// get updating and rendering to happen on different threads
// Benchmark the updating thread - i can imagine he bottle neck will be with how we are updating
// push code to github
// add Interpolation

// graphics
// - make the particles circles

// new features
// Adding Gaussian rotation noise η(σ, t) to Equation 1:

// ------ Optimizations ------
// - threadpool creates tasks every frame dispite the indexes do no change every frame
// - rendering and updating happen on the same thread
// - test if fetching the sin and cos values are actually faster, is there such thing as approximated sin and cos
