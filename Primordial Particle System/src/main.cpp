#include "simulation.h"


int main()
{
	Simulation simulation;
	simulation.run();
}

/*
==== Feature TO-DO List ====

--- imgui todo ---
Ability to change particle radius
Ability to change polygon surfaces
Ability to change particle scale

- Ability to change simulation speed - implement delta time
- Add all particle update rule presets
- Set defualt color settings

Advanced Window
- ability to change Hash Grid Cells
- ability to change anti analizing mode


--- graphics ---
- smooth camera zooming
- render the particles on the gpu (finally)


--- misc ---
> Use CMake to create project



RENDERING OPTIMIZATION
      "A vertex buffer is a modern GPU feature that allows you to store 
	         vertex data directly in the GPU’s memory (VRAM)."
 - The vertex data resides in GPU memory, eliminating the need to resend data every frame. 

 // baseline 25fps
*/