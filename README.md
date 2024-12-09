# CarOusel

A project for the UniPi Computer Graphics course.

It consists of a freely explorable scene with cars running around a track.

### Controls

- Use **WASD** to move around the scene, the mouse to look and **Spacebar** / **Left Shift** to move up or down, respectively

- Press **E** to toggle between fast and slow movement (useful to observe the details)

- Press **Q** to toggle shadows and **K** to toggle sunlight.

- Press **L** to toggle the lamps and **J** to toggle the cars' headlights. During the night they will turn on automatically.

- Press **T** to stop time.

- Press **C** to cycle between the available cameras.

- Press **V** to toggle the debug view: this will show the sun's shadowmap and its projection frustum.

### Features

- Phong shading with textures for the terrain, lamps and trees

- POV-switching

- Shadow mapping to cast shadows from the sun, with tight frustum fitting to avoid precision loss at shallow angles 

- The street lamps and headlights turn on automatically at night, casting real-time shadows with Percentage Closer Filtering (PCF) and slope bias.

### Dependencies

The project needs **glm**, **tinygltf**, **GLFW** and **GLEW** in order to successfully build.
