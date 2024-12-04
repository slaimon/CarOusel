# CarOusel

A project for the UniPi Computer Graphics course.

It consists of a freely explorable scene with cars running around a track.

### Controls

- Use **WASD** to move around the scene, the mouse to look and **Spacebar** / **Left Shift** to move up or down, respectively

- Press **E** to toggle between fast and slow movement (useful to observe the details)

- Press **Q** to toggle shadows and **K** to toggle sunlight.

- Press **L** to toggle the lamps. During the night they turn on automatically.

- Press **T** to stop the sun and the cars from moving. When you press T again, they will skip ahead to where they would've been.

- Press **C** to cycle between the available cameras.

- Press **V** to toggle the debug view: this will show the sun's shadowmap and its projection frustum.

### Features

- Phong shading with textures for the terrain, lamps and trees

- POV-switching

- Shadow mapping to cast shadows from the sun, with tight frustum fitting to avoid precision loss at shallow angles 

- The lamps turn on automatically at night, casting real-time shadows

### Dependencies

The project needs **glm**, **tinygltf**, **GLFW** and **GLEW** in order to successfully build.
