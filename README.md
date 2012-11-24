## Come Fly With Me

![Screenshot](https://github.com/cpatuzzo/come_fly_with_me/blob/master/screen.jpeg?raw=true)

## Setup

The following has been tested on Mac OS X, 10.8:

```
cd vendor/glm-0.3.1/
./configure
make install
cd ../../

gcc -framework OpenGL -framework GLUT -lglm -ljpeg -lpng main.cpp

./a.out
```

## Concept

I have used something known as a skybox for my program. This places the camera at the centre of a cube. All faces of the cube have image textures. This allows the camera to browse the static landscape seamlessly.

I added four 'cloud planes' to the skybox which are made up of triangles which are calculated using the circle equation. The cloud planes do not appear to move with the camera, but in fact do. They are rounded to integers to give the impression that the skybox is moving, when in fact it is translated to its neighbour tessellation. This means that the number of 'clouds' can be set by the user and does not have to cover the entire plane, making the calculation and drawing routine much more efficient. The cloud plane is an artistic attempt to show wind currents and clouds moving along as the camera moves through the sky.

I have used glm to load an object of an airplane and one of an eagle. The eagle object did not come with a material file. Therefore, I created one myself using the standard format.

I set lighting in the scene. These are set at the approximate locations of the sun and the reflection of the sun on the water. This is an attempt to add realism and consistency to the scene. I set the components of the reflection's brightness to half of that of the sun. This means that you can see silhouettes and interesting specular lighting throughout the scene.

I wanted the motion to look gradual and fluid. To achieve this, I based a lot of the transformations on the sine and cosine graphs. I also applied different sets of translations before and after the rotation of each object. This meant I could program more flexible pathing. I wanted to make the animation last about 45 seconds then return to the start and begin again. One of the harder aspects of the animation was getting the eagle to land and take off from the wing of the airplane, which took a lot of careful calculation.

I wrapped up all of the motion into an 'event' kind of structure which (like everything else) is clearly commented in the code. The animation is designed such that leaving the camera in its default orientation and momentum should give the best viewing. Most parameters are configurable in the #define's at the top of the file. This can assist in running the animation on very old PCs.

## Controls

```
W: Accelerate
S: Decelerate
A: Turn left
D: Turn right
=: Increase elevation
-: Decrease elevation
0: Stop moving
Space: Pause animation

F: Fullscreen
P: Set viewpoint A
U: Set viewpoint B
Y: Set viewpoint C

H: Help
R: Reset
Q: Quit
```

## Contribution

Feel free to contribute. No commit is too small.

You should follow me: [@cpatuzzo](https://twitter.com/cpatuzzo)
