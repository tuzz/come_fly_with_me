# An OpenGL animation created in C++ based on the theme of flight

By [Christopher Patuzzo](http://chris.patuzzo.co.uk/)


*** Camera Controls

Up: Accelerate
Down: Decelerate
Left: Turn left
Right: Turn right
Page Up: Increase elevation
Page down: Decrease elevation
0: Stop moving

*** Other Controls

Space: Pause animation - you may still move the camera.
H: Display help
P: Set position 1
U: Set position 2
Y: Set position 3
R: Reset
Q: Quit

*** Files

gl.cpp - The main C++ file.
screenshot.jpg - The screenshot of position 1.
airplane.obj - A waveform object file of an airplane.
airplane.mtl - The material file for airplane.obj.
eagle.obj - A waveform object file of an eagle.
eagle.mtl - The material file for eagle.obj.
include/north.jpg, east.jpg, south.jpg, west.jpg, bottom.jpg, top.jpg - The texture images for the skybox.
include/cripeelv.jpg, Cripfus.jpg, criperud.jpg, criptai.jpg, cripewng.jpg - The texture images for the airplane.
include/cloud.jpg - The 'cloud' texture image.
include/glm3-0.1.3/ - It was requested by jnc that I include this with my program.

*** Build Instructions

The program uses the glcc compilation program, as well as glm3-0.1.3, available: http://devernay.free.fr/hacks/glm/
glcc is available on all linux ECS machines and it was specified that we may use glm for the coursework.
To install glm3-0.1.3, cd to the directory, then run ./configure, followed by sudo make install.

To compile gl.cpp, use glcc with the additional dependencies, i.e.
	glcc -o gl gl.cpp -lglm -ljpeg

To run, use ./gl - or whatever you named the executable.

--------------------
Note: The jpeg library is the exact same dependency that SDL_image requires. I am not using any additional libraries to those specified in the coursework description. The program uses the following jpeg loading process: [image.jpg -> -ljpeg -> glm3-0.1.3] instead of: [image.jpg -> -ljpeg -> SDL_image -> glm3-0.1.3] As you can see, the SDL_image library is obselete when using glm.
--------------------

*** How does it work?

I have used something known as a skybox for my program. This places the camera at the centre of a cube. All faces of the cube have image textures. This allows the camera to browse the static landscape seamlessly.

I added four 'cloud planes' to the skybox which are made up of triangles which are calculated using the circle equation. The cloud planes do not appear to move with the camera, but in fact do. They are rounded to integers to give the impression that the skybox is moving, when in fact it is translated to its neighbour tessellation. This means that the number of 'clouds' can be set by the user and does not have to cover the entire plane, making the calculation and drawing routine much more efficient. The cloud plane is an artistic attempt to show wind currents and clouds moving along as the camera moves through the sky.

I have used glm to load an object of an airplane and one of an eagle. The eagle object did not come with a material file. Therefore, I created one myself using the standard format. To load these objects in, I wrote the drawing code myself which works in a similar manner to that of glm by using the two-phase approach, mine however is optimised for my animation as I know which modes and texture types I am using.

I set lighting in the scene. These are set at the approximate locations of the sun and the reflection of the sun on the water. This is an attempt to add realism and consistency to the scene. I set the components of the reflection's brightness to half of that of the sun. This means that you can see silhouettes and interesting specular lighting throughout the scene.

I wanted the motion to look gradual and fluid. To achieve this, I based a lot of the transformations on the sine and cosine graphs. I also applied different sets of translations before and after the rotation of each object. This meant I could program more flexible pathing. I wanted to make the animation last about 45 seconds then return to the start and begin again. One of the harder aspects of the animation was getting the eagle to land and take off from the wing of the airplane, which took a lot of careful calculation.

I wrapped up all of the motion into an 'event' kind of structure which (like everything else) is clearly commented in the code. The animation is designed such that leaving the camera in its default orientation and momentum should give the best viewing. Most parameters are configurable in the #define's at the top of the file. This can assist in running the animation on very old PCs.


*** Inspiration Sources

To create the skybox, I followed the guide that can be found here: http://sidvind.com/wiki/Skybox_tutorial
The textures that are used for the skybox can be found here: http://www.hazelwhorley.com/textures.html
The airplane.obj, airplane.mtl and eagle.obj files are contained in the sample data here: http://devernay.free.fr/hacks/glm/
To create the cloud planes, I used coursework 1 as inspiration, by creating circles with few sections and translating them.
