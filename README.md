# dig-path-stage

Commands are provided via a serial connection in newline-terminated strings. A baud rate of 9600 is used. The follwing commands are used:

> zero
Returns the stage to physical origin and sets relative home coordinates.

> gohome
Returns the stage to relative home.

>sethome
Set current positive to be relative home.

> distances <x distances, in steps> <y distances, in steps>
Move the stage by the specified distances.

> position <x position, in steps> <y position, in steps>
Moves the stage to the target position, which is relative to the relative home coordinates.

> line <x distance, in steps> <y distance, steps> <step size, in steps>  <step time, in ms>
Performs a line sweep along a given axis (x or y). Either "x distance" or "y distance" must be zero. The stage waits "step time" milliseconds between steps. Does not return stage upon completion.

> raster <x distance, in steps> <y distance, in steps> <step size, in steps> <step time, in ms>
Performs a raster sweep. Stage sweeps "across" the larger distance and "down" the smaller distance. As in the line sweep the stage waits "step time" milliseconds between steps. Returns the stage to the starting coordinates of the raster sweep upon completion.

> led <0, 1>
Turns the LED on or off (0 for off; 1 for on).
