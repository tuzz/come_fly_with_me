//Camera start location and rotation, boundaries are -0.5 -> +0.5 in each axis.
#define CAMERA_START_X -0.6
#define CAMERA_START_Y -0.2
#define CAMERA_START_Z -1.8
#define CAMERA_START_R 330
#define CAMERA_START_MOMENTUM 0.15

//Camera keyboard-event motion speed per second, independent of frame rate or machine.
#define CAMERA_ROTATION 200
#define CAMERA_ELEVATION 2
#define CAMERA_MOMENTUM 0.1

//Cloud plane configuration constants.
#define CLOUDS 8
#define CLOUD_SECTIONS 3
#define CLOUD_INNER_PLANES 0.2
#define CLOUD_OUTER_PLANES 0.8

//Frame rate, lowering the frame rate may improve performance on older computers.
#define FPS 25

//Aliases for glm to ensure OpenGL's immediate mode is never used anywhere else.
#define glmTriangles() glBegin(GL_TRIANGLES)
#define glmNormal(x) glNormal3fv(x)
#define glmVertex(x) glVertex3fv(x)
#define glmPurge() glEnd()
#define MATERIAL_BY_FACE

//Other global constants.
#define PI 3.141592
#define SCALE_FACTOR 0.0001

#define SDL_main main

#include "SDL.h"
#include "SDL_opengl.h"
#include "glm.h"
#include <math.h>

//*****************************************
//           Global Variables
//*****************************************

//Use glm's texture id variable to maintain consistency.
extern GLenum _glmTextureTarget;

//Models.
GLMmodel* eagle;
GLMmodel* airplane;

//Cloud plane.
float cloudPlaneV[CLOUDS][CLOUDS][7][CLOUD_SECTIONS][3];
GLuint cloudTexture;

//Camera location and rotation.
float cameraX = CAMERA_START_X * SCALE_FACTOR;
float cameraY = CAMERA_START_Y * SCALE_FACTOR;
float cameraZ = CAMERA_START_Z * SCALE_FACTOR;
float cameraR = CAMERA_START_R;

//Key event values for camera control.
float rotationDirection = 0;
float elevationDirection = 0;
float momentumDirection = 0;

//Skybox texture indices.
GLuint skybox[6];

//Skybox vertices.
GLfloat vertices[8][3] =
  {{-1, -1, -1}, // 0: left,  bottom, back
   {-1, -1, +1}, // 1: left,  bottom, front
   {-1, +1, -1}, // 2: left,  top,    back
   {-1, +1, +1}, // 3: left,  top,    front
   {+1, -1, -1}, // 4: right, bottom, back
   {+1, -1, +1}, // 5: right, bottom, front
   {+1, +1, -1}, // 6: right, top,    back
   {+1, +1, +1}};// 7: right, top,    front

//Skybox cube faces.
GLubyte faces[6][4] =
  {{1, 0, 2, 3}, // 0: west
   {4, 5, 7, 6}, // 1: east
   {4, 0, 1, 5}, // 2: below
   {7, 3, 2, 6}, // 3: above
   {5, 1, 3, 7}, // 4: south
   {0, 4, 6, 2}};// 5: north

//Standard texture winding.
GLfloat texture[4][2] =
  {{0.0, 0.0}, // 0: left,  bottom
   {1.0, 0.0}, // 1: right, bottom
   {1.0, 1.0}, // 2: right, top
   {0.0, 1.0}};// 3: left,  top

//Animation variables.
int frame = -1;
bool paused = false;
float momentum = CAMERA_START_MOMENTUM * SCALE_FACTOR;
float momentumBeforePause;

float planePriorX = 0, planePriorY = 0, planePriorZ = 0;
float planePostX = 0, planePostY = 0, planePostZ = 0;
float planeRotX = 0, planeRotY = 0, planeRotZ = 0;

float eaglePriorX = -0.5, eaglePriorY = 0.03, eaglePriorZ = 0.1;
float eaglePostX = 0, eaglePostY = 0, eaglePostZ = 0;
float eagleRotX = 0, eagleRotY = 0, eagleRotZ = 0;

//*****************************************
//      Homogeneous Matrix Functions
//*****************************************
//Matrices are column before row.
void translate(float x, float y, float z) {
  float m[] = {1, 0, 0, 0,
           0, 1, 0, 0,
           0, 0, 1, 0,
           x, y, z, 1};

  glMultMatrixf(m);
}

void scale(float x, float y, float z) {
  float m[] = {x, 0, 0, 0,
         0, y, 0, 0,
         0, 0, z, 0,
         0, 0, 0, 1};

  glMultMatrixf(m);
}

void rotateX(float angle) {
  //Convert angle to radians.
  angle *= PI / 180;
  float m[] = {1, 0,           0,          0,
               0, cos(angle),  sin(angle), 0,
               0, -sin(angle), cos(angle), 0,
               0, 0,           0,          1};

  glMultMatrixf(m);
}

void rotateY(float angle) {
  //Convert angle to radians.
  angle *= PI / 180;
  float m[] = {cos(angle), 0, -sin(angle), 0,
               0,          1, 0,           0,
               sin(angle), 0, cos(angle),  0,
               0,          0, 0,           1};

  glMultMatrixf(m);
}

void rotateZ(float angle) {
  angle *= PI / 180;
  float m[] = {cos(angle),  sin(angle), 0, 0,
               -sin(angle), cos(angle), 0, 0,
               0,           0,          1, 0,
               0,           0,          0, 1};

  glMultMatrixf(m);
}

//Places the viewing area inside a frustum.
void perspective(float fov, float aspect, float zNear, float zFar) {
  float xyMax = zNear * tan(fov * PI / 360);
  float xMin = -xyMax;
  float yMin = -xyMax;

  float width = xyMax - xMin;
  float height = xyMax - yMin;

  float depth = zFar - zNear;
  float q = -(zFar + zNear) / depth;
  float qn = -2 * (zFar * zNear) / depth;

  float w = 2 * zNear / width;
  w /= aspect;
  float h = 2 * zNear / height;

  float m[] = {w, 0,  0,  0,
               0, h,  0,  0,
               0, 0,  q,  -1,
               0, 0, qn,  0};

  glMultMatrixf(m);
}

//*****************************************
//      Loading and Drawing Objects
//*****************************************

//Objects should be placed in ./include
void loadObjects() {
  eagle = glmReadOBJ("eagle.obj");
  glmVertexNormals(eagle, 180.0, false);
  glmUnitize(eagle);

  airplane = glmReadOBJ("airplane.obj");
  glmVertexNormals(airplane, 180.0, false);
  glmUnitize(airplane);
}

//Based on glm's two-phase approach. Majorly stripped down and optimised for this animation.
void drawObject(GLMmodel* model, bool texture) {
  //If using textures, enable them.
  if (texture) {
    glEnable(_glmTextureTarget);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }

  //Read the object file in two passes.
  int state = 0;
    for (int pass = 0; pass < 2; pass++) {

    //Set defaults.
    int filterMode = 0;
    int nextMaterial = 0;
    GLuint currentMaterial = -1;
    GLMmaterial* materialPointer = NULL;
    int nextTexture = 0;
    GLuint diffuseEnabled = -1;
    GLMgroup* group = model->groups;

    //Loop through vertices on each material.
    while (group) {
      currentMaterial = group->material;
      materialPointer = &model->materials[currentMaterial];
      filterMode = (materialPointer->diffuse[3] < 1.0);
      state = state | filterMode;
      nextMaterial = 1;

      //Set new texture if needed.
      if(materialPointer->map_diffuse != diffuseEnabled) {
        nextTexture = 1;
        diffuseEnabled = materialPointer->map_diffuse;
      }

      //Start accepting triangles in state machine.
      glmTriangles();

      //Loop through the triangles in the material group.
      for (int i = 0; i < group->numtriangles; i++) {
        GLMtriangle* triangle = &model->triangles[(group->triangles[i])];

        //Set material if needed.
        if(triangle->material && triangle->material != currentMaterial) {
        currentMaterial = triangle->material;
        materialPointer = &model->materials[currentMaterial];
        filterMode = (materialPointer->diffuse[3] < 1.0);
        state |= filterMode;
        nextMaterial = 1;
          if(materialPointer->map_diffuse != diffuseEnabled) {
            nextTexture = 1;
            diffuseEnabled = materialPointer->map_diffuse;
          }
        }

        //Only render in the second pass.
        if(filterMode == pass) {
          //Set material if needed.
          if(nextMaterial) {
            nextMaterial = 0;
            if (texture && nextTexture) {
              nextTexture = 0;
              glmPurge();
              if(diffuseEnabled == -1) glBindTexture(_glmTextureTarget, 0);
              else glBindTexture(_glmTextureTarget, model->textures[diffuseEnabled].id);
              glmTriangles();
            }

            //Material lighting, uses the material file data.
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialPointer->ambient);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialPointer->diffuse);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialPointer->specular);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materialPointer->shininess);
            }

          //Calculate the vertex normals, OpenGL uses them for lighting calculations.
          for (int j = 0; j < 3; j++) {
            if (triangle->nindices[j]!=-1) glmNormal(&model->normals[3 * triangle->nindices[j]]);

            //If using textures, apply them now.
            if (texture && (triangle->tindices[j]!=-1) && diffuseEnabled != -1)
              glTexCoord2f(model->texcoords[2 * triangle->tindices[j]] * model->textures[diffuseEnabled].width,
                     model->texcoords[2 * triangle->tindices[j] + 1] * model->textures[diffuseEnabled].height);

            //Add the points to be drawn.
            glmVertex(&model->vertices[3 * triangle->vindices[j]]);
          }
        }
      }
      glmPurge();

      //Next material group.
      group = group->next;
    }

    //Set blend function and disable the depth mask for the second pass.
    if(!state) break;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

  }

  //Reset depth buffer and blend mode.
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

//*****************************************
//               Camera
//*****************************************

void updateCamera() {
  //Reset position and rotation.
  glLoadIdentity();

  //Camera rotation.
  cameraR += rotationDirection * CAMERA_ROTATION / FPS;
  if (cameraR < 0) cameraR += 360;
  if (cameraR > 360) cameraR -= 360;

  //Camera location.
  momentum += momentumDirection * CAMERA_MOMENTUM * SCALE_FACTOR / FPS;
  if (momentum < 0) momentum = 0;
  cameraX += momentum * -sin(cameraR * PI / 180);
  cameraY += elevationDirection * CAMERA_ELEVATION * SCALE_FACTOR / FPS;
  cameraZ += momentum * cos(cameraR * PI / 180);

  //Move the world, not the camera.
  rotateY(cameraR);
  translate(cameraX, cameraY, cameraZ);
}

//*****************************************
//                Skybox
//*****************************************

//Loads the skybox textures, located in ./include
void loadSkybox() {
  GLfloat width = 600, height = 600;
  skybox[0] = glmLoadTexture("include/west.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
  skybox[1] = glmLoadTexture("include/east.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
  skybox[2] = glmLoadTexture("include/bottom.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
  skybox[3] = glmLoadTexture("include/top.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
  skybox[4] = glmLoadTexture("include/south.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
  skybox[5] = glmLoadTexture("include/north.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
}

//Draws the skybox.
void drawSkybox() {
  //Disable lighting, enable textures.
  glDisable(GL_LIGHTING);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  //Augment such that camera is at the center.
  GLfloat center[8][3];
  for (int i = 0; i < 8; i++) {
    center[i][0] = vertices[i][0] - cameraX;
    center[i][1] = vertices[i][1] - cameraY;
    center[i][2] = vertices[i][2] - cameraZ;
  }

  //Draw eace face.
  GLfloat face[4][3];
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) for (int k = 0; k < 3; k++)
      face[j][k] = center[faces[i][j]][k];
      glBindTexture(GL_TEXTURE_2D, skybox[i]);

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      glVertexPointer(3, GL_FLOAT, 0, face);
      glTexCoordPointer(2, GL_FLOAT, 0, texture);
      glDrawArrays(GL_QUADS, 0, 4);
  }

  //Enable lighting, disable textures.
  glEnable(GL_LIGHTING);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

//*****************************************
//             Cloud Plane
//*****************************************

void calculateCloudPlane() {
  //Calculate the increment.
  float incr = 2 * PI / CLOUD_SECTIONS;

  //Use cosArr and sinArr to reuse the sine and cosine calculation.
  float cosArr[CLOUD_SECTIONS + 1], sinArr[CLOUD_SECTIONS + 1];
  for (int i = 0; i < CLOUD_SECTIONS + 1; i++) {
    cosArr[i] = cos(i * incr);
    sinArr[i] = sin(i * incr);
  }

  //Calculate a circle.
  float circle[CLOUD_SECTIONS][3];
  for (int i = 0; i < CLOUD_SECTIONS; i++) {
    circle[i][0] = cosArr[i];
    circle[i][1] = 0;
    circle[i][2] = sinArr[i];
  }

  //Calculate a cloud from the circle.
  float cloud[7][CLOUD_SECTIONS][3];
  float xScale, zScale, xTrans, zTrans;
  for (int i = 0; i < 7; i++) {
    switch (i) {
      //Scale everything down quite a bit, we only want a light covering.
      case 0: xScale = 0.125; zScale = 0.125; xTrans = 0; zTrans = 0;
      break;
      case 1: xScale = 0.0625; zScale = 0.0625; xTrans = 0.125; zTrans = 0;
      break;
      case 2: xScale = 0.0625; zScale = 0.0625; xTrans = -0.125; zTrans = 0;
      break;
      case 3: xScale = 0.0625; zScale = 0.0625; xTrans = 0.125 * cos(60 * PI / 180); zTrans = 0.125 * sin(60 * PI / 180);
      break;
      case 4: xScale = 0.0625; zScale = 0.0625; xTrans = 0.125 * cos(120 * PI / 180); zTrans = 0.125 * sin(120 * PI / 180);
      break;
      case 5: xScale = 0.0625; zScale = 0.0625; xTrans = 0.125 * cos(240 * PI / 180); zTrans = 0.125 * sin(240 * PI / 180);
      break;
      case 6: xScale = 0.0625; zScale = 0.0625; xTrans = 0.125 * cos(300 * PI / 180); zTrans = 0.125 * sin(300 * PI / 180);
      break;
    }
    for (int j = 0; j < CLOUD_SECTIONS; j++) {
      cloud[i][j][0] = circle[j][0] * xScale + xTrans;
      cloud[i][j][1] = circle[j][1];
      cloud[i][j][2] = circle[j][2] * zScale + zTrans;
    }
  }

  //Calculate a cloud plane from the cloud.
  for (int i = 0; i < CLOUDS; i++)
    for (int j = 0; j < CLOUDS; j++)
      for (int k = 0; k < 7; k++)
        for (int l = 0; l < CLOUD_SECTIONS; l++) {
          cloudPlaneV[i][j][k][l][0] = cloud[k][l][0] - CLOUDS / 2 + i;
          cloudPlaneV[i][j][k][l][1] = cloud[k][l][1];
          cloudPlaneV[i][j][k][l][2] = cloud[k][l][2] - CLOUDS / 2 + j;
        }

  //Load texture.
  float width = 256, height = 256;
  cloudTexture = glmLoadTexture("include/cloud.jpg", GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, &width, &height);
}

//Draws 4 planes containing 'cloud' type objects with parameters defined as constants above.
void drawCloudPlane() {
  glPushMatrix();
  glDisable(GL_LIGHTING);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  //Move the plane with camera, utilises integer truncation.
  int xOffset = -cameraX / SCALE_FACTOR;
  float yOffset = -cameraY / SCALE_FACTOR;
  int zOffset = -cameraZ / SCALE_FACTOR;
  translate(xOffset, yOffset, zOffset);

  //Bind data pointers.
  glBindTexture(GL_TEXTURE_2D, cloudTexture);
  glVertexPointer(3, GL_FLOAT, 0, cloudPlaneV);
  glTexCoordPointer(2, GL_FLOAT, 0, texture);

  //Top plane.
  glPushMatrix();
  translate(0, CLOUD_OUTER_PLANES, 0);
  for (int i = 0; i < CLOUDS * CLOUDS * 7; i++)
    glDrawArrays(GL_POLYGON, i * CLOUD_SECTIONS, CLOUD_SECTIONS);
  glPopMatrix();

  //2nd plane.
  glPushMatrix();
  translate(0, CLOUD_INNER_PLANES, 0);
  for (int i = 0; i < CLOUDS * CLOUDS * 7; i++)
    glDrawArrays(GL_POLYGON, i * CLOUD_SECTIONS, CLOUD_SECTIONS);
  glPopMatrix();

  //3rd plane.
  glPushMatrix();
  translate(0, -CLOUD_INNER_PLANES, 0);
  for (int i = 0; i < CLOUDS * CLOUDS * 7; i++)
    glDrawArrays(GL_POLYGON, i * CLOUD_SECTIONS, CLOUD_SECTIONS);
  glPopMatrix();

  //Bottom plane.
  glPushMatrix();
  translate(0, -CLOUD_OUTER_PLANES, 0);
  for (int i = 0; i < CLOUDS * CLOUDS * 7; i++)
    glDrawArrays(GL_POLYGON, i * CLOUD_SECTIONS, CLOUD_SECTIONS);
  glPopMatrix();

  glEnable(GL_LIGHTING);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glPopMatrix();
}

//*****************************************
//                Lights
//*****************************************

//Set up lights at points on the skybox that match the textures.
void setupLights() {
  glEnable(GL_LIGHTING);glEnable(_glmTextureTarget);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glEnable(GL_RESCALE_NORMAL);

  //Set global ambience.
  GLfloat global_ambient[] = {0.5, 0.5, 0.5, 1};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

  //Set up a light source at the sun's location.
  glEnable(GL_LIGHT0);
  GLfloat light0_ambient[] = {0, 0, 0, 1};
  GLfloat light0_diffuse[] = {0.5, 0.5, 0.5, 1};
  GLfloat light0_specular[] = {0.7, 0.7, 0.7, 1};
  GLfloat light0_position[] = {-1, 1, -1, 1};

  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

  //Set up a light source at the sun's reflection.
  //The reflection is half as bright as the sun.
  glEnable(GL_LIGHT1);
  GLfloat light1_ambient[] = {0, 0, 0, 1};
  GLfloat light1_diffuse[] = {0.25, 0.25, 0.25, 1};
  GLfloat light1_specular[] = {0.35, 0.35, 0.35, 1};
  GLfloat light1_position[] = {-1, -1, -1, 1};

  glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
  glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
}

//This augments the lighting to match the skybox, required after camera rotation.
void augmentLights() {
  GLfloat light0_position[] = {-1 / SCALE_FACTOR, 1 / SCALE_FACTOR, -1 / SCALE_FACTOR, 1};
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

  GLfloat light1_position[] = {-1 / SCALE_FACTOR, -1 / SCALE_FACTOR, -1 / SCALE_FACTOR, 1};
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
}

//*****************************************
//              Main Scene
//*****************************************

void drawEagle() {
  glPushMatrix();

  //Scale the eagle down to size.
  scale(0.5, 0.5, 0.5);

  //Face forward.
  rotateY(180);

  drawObject(eagle, false);

  glPopMatrix();
}

void drawAirplane() {
  glPushMatrix();

  //Rotate the plane so that it faces forward.
  rotateX(270);
  rotateZ(90);

  drawObject(airplane, true);

  glPopMatrix();
}

//Keeps angles within bounds - prevents overflow.
void bounds(float &angle) {
  if (angle > 360) angle -= 360;
  if (angle < 0) angle += 360;
}

void drawScene() {
  glPushMatrix();

  //Set the animation control variables.
  if (!paused) frame++;
  int ticks = frame % 720;
  if (frame % 720 == 0) {
    ticks = 0;

    //Reset animation object positions and rotations.
    planePriorX = 0, planePriorY = 0, planePriorZ = 0;
    planePostX = 0, planePostY = 0, planePostZ = 0;
    planeRotX = 0, planeRotY = 0, planeRotZ = 0;

    eaglePriorX = -0.5, eaglePriorY = 0.03, eaglePriorZ = -0.1;
    eaglePostX = 0, eaglePostY = 0, eaglePostZ = 0;
    eagleRotX = 0, eagleRotY = 0, eagleRotZ = 0;
  }

  //Apply scene transformations.
  rotateY(30);
  translate(0, 0, -1500 * frame * SCALE_FACTOR);

  //Make sure angles are in bounds.
  bounds(planeRotX); bounds(planeRotY); bounds(planeRotZ);
  bounds(eagleRotX); bounds(eagleRotY); bounds(eagleRotZ);

  if (!paused) {
    //Airplane transformation events.
    //Constant swaying.
    planeRotZ -= cos(ticks * 4 * PI / 180);
    planePostY -= 0.02 * cos(ticks * 2 * PI / 180);

    //Spin upside-down.
    if (ticks >= 90 && ticks < 180) planeRotZ -= PI * sin(ticks * 2 * PI / 180);

    //Spin back round.
    if (ticks >= 180 && ticks < 270) planeRotZ += PI * sin(ticks * 2 * PI / 180);

    //Dive.
    if (ticks >= 360 && ticks < 450) {
      planeRotX -= 0.5 * PI * sin(ticks * 4 * PI / 180);
      planePriorY -= 0.01 * PI * sin(ticks * 2 * PI / 180);
      planePriorZ -= 0.02 * PI * sin(ticks * PI / 180);
    }

    //Loop-the-loop
    if (ticks >= 450 && ticks < 630) {
      planeRotX += 2;
      planePriorY += 0.03 * PI * sin((ticks - 90) * 2 * PI / 180);
      planePriorZ -= 0.03 * PI * cos((ticks - 90) * 2 * PI / 180);
    }

    //Reset position.
    if (ticks >= 630 && ticks < 720) {
      planeRotX -= 0.25 * PI * sin(ticks * 4 * PI / 180);
      planePriorY -= 0.01 * PI * sin(ticks * 2 * PI / 180);
      planePriorZ -= 0.02 * PI * sin(ticks * PI / 180);
    }

    //Eagle transformation events.
    //Set default position.
    eaglePriorX = -1;
    eaglePriorZ = 0.4;

    //Set start position over left wing.
    if (ticks >= 0 && ticks < 90) {
      eaglePriorX = -0.5;
      eaglePriorY = 0.03;
      eaglePriorZ = -0.1;
    }

    //Move a little with the wing.
    if (ticks >= 0 && ticks < 45) {
      eagleRotZ -= cos(ticks * 4 * PI / 180);
      eaglePostY -= 0.018 * sin(ticks * 4 * PI / 180);
    }

    //Move back, up and left.
    if (ticks >= 45 && ticks < 90) {
      eaglePriorX -= (float)0.5 / 45 * (ticks - 45);
      eaglePriorZ += (float)0.5 / 45 * (ticks - 45);
      eaglePostY += 0.018 * sin((ticks - 45) * 4 * PI / 180);
    }

    //Swaying.
    if (ticks >= 45 && ticks < 675) {
      eagleRotZ -= cos((ticks + 45) * 2 * PI / 180);
      eaglePostY -= 0.02 * cos((ticks + 45) * 4 * PI / 180);
    }

    //Avoid plane collision.
    if (ticks >= 180 && ticks < 270) eagleRotZ += 0.5 * PI * sin(ticks * 4 * PI / 180);

    //Glide from side to side.
    if (ticks >= 360 && ticks < 540) eaglePriorX += 0.4 * PI * sin(ticks * PI / 180);

    //Move back to start position.
    if (ticks >= 675 && ticks < 720) {
      eaglePriorX += (float)0.5 / 45 * (ticks - 675);
      eaglePriorZ -= (float)0.5 / 45 * (ticks - 675);
    }

    //Avoid plane collision.
    if (ticks >= 700 && ticks < 710) eaglePriorY += 0.01;
    if (ticks >= 710 && ticks < 720) eaglePriorY -= 0.01;
  }

  //Apply the transformations then draw the airplane.
  glPushMatrix();
  translate(planePriorX, planePriorY, planePriorZ);
  rotateX(planeRotX);
  rotateY(planeRotY);
  rotateZ(planeRotZ);
  translate(planePostX, planePostY, planePostZ);
  drawAirplane();
  glPopMatrix();

  //Apply the transformations then draw the eagle.
  glPushMatrix();
  translate(eaglePriorX, eaglePriorY, eaglePriorZ);
  rotateX(eagleRotX);
  rotateY(eagleRotY);
  rotateZ(eagleRotZ);
  translate(eaglePostX, eaglePostY, eaglePostZ);
  drawEagle();
  glPopMatrix();
  glPopMatrix();
}

//*****************************************
//                  SDL
//*****************************************

//Initialisation function.
void init() {
  //Initialise SDL.
  if(SDL_Init(SDL_INIT_VIDEO) < 0) exit(1);

  //Use double buffering (24-bit).
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  //Create window.
  if (SDL_SetVideoMode(512, 512, 32, SDL_OPENGL) == 0) exit(1); //FIND
  SDL_WM_SetCaption("cp22g08 Coursework 3", "cp22g08 Coursework 3");

  //Set up camera.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  perspective(90, 1, 0.00001, 1 / SCALE_FACTOR);
  glMatrixMode(GL_MODELVIEW);

  //Set the clear color, just in case - sky blue.
  glClearColor(0.6, 0.8, 1, 1);

  //Use z-buffer, lighting, normal scaling.
  glEnable(GL_DEPTH_TEST);

  //Set up two lights; the sun and its reflection.
  setupLights();

  //Use vertex arrays.
  glEnable(GL_VERTEX_ARRAY);
  glEnable(GL_TEXTURE);

  //Load skybox and objects.
  loadSkybox();
  loadObjects();

  //Calculate cloud plane.
  calculateCloudPlane();
}

//Main display loop.
void display() {
  //Clear buffers.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //Augment camera, draw skybox.
  updateCamera();
  drawSkybox();

  //Scale the world and reset light positions.
  scale(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR);
  augmentLights();

  //Draw the plane containing clouds and the main scene.
  drawCloudPlane();
  drawScene();

  //Swap buffers.
  SDL_GL_SwapBuffers();
}

//Event handler.
void events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
          case SDLK_q: exit(0);
          //Set camera variables.
          case SDLK_LEFT: rotationDirection--;
          break;
          case SDLK_RIGHT: rotationDirection++;
          break;
          case SDLK_PAGEUP: elevationDirection--;
          break;
          case SDLK_PAGEDOWN: elevationDirection++;
          break;
          case SDLK_UP: momentumDirection++;
          break;
          case SDLK_DOWN: momentumDirection--;
          break;
          case SDLK_SPACE:
            if (paused) {
              paused = false;
              momentum = momentumBeforePause;
            }
            else {
              paused = true;
              momentumBeforePause = momentum;
              momentum = 0;
            }
          break;
          case SDLK_0: momentum = 0;
          break;
          case SDLK_r:
            //Reset the camera.
            cameraX = CAMERA_START_X * SCALE_FACTOR;
            cameraY = CAMERA_START_Y * SCALE_FACTOR;
            cameraZ = CAMERA_START_Z * SCALE_FACTOR;
            cameraR = CAMERA_START_R;
            momentum = CAMERA_START_MOMENTUM * SCALE_FACTOR;

            //Reset animation frame, set unpaused.
            frame = -1;
            paused = false;

            //Reset animation object positions and rotations.
            planePriorX = 0, planePriorY = 0, planePriorZ = 0;
            planePostX = 0, planePostY = 0, planePostZ = 0;
            planeRotX = 0, planeRotY = 0, planeRotZ = 0;

            eaglePriorX = -0.5, eaglePriorY = 0.03, eaglePriorZ = -0.1;
            eaglePostX = 0, eaglePostY = 0, eaglePostZ = 0;
            eagleRotX = 0, eagleRotY = 0, eagleRotZ = 0;
          break;
          case SDLK_p:
            //Pause the animation.
            paused = true;
            momentumBeforePause = momentum;
            momentum = 0;

            //Set frame.
            frame = 605;

            //Move the camera.
            cameraX = 0.004524;
            cameraY = 0.000052;
            cameraZ = 0.007764;
            cameraR = 306;

            //Reset animation object positions and rotations.
            planePriorX = 0, planePriorY = -0.871533, planePriorZ = -1.577791;
            planePostX = 0, planePostY = -0.44244, planePostZ = 0;
            planeRotX = 312, planeRotY = 0, planeRotZ = 13.651090;

            eaglePriorX = -1, eaglePriorY = 0.03, eaglePriorZ = 0.4;
            eaglePostX = 0, eaglePostY = -0.293748, eaglePostZ = 0;
            eagleRotX = 0, eagleRotY = 0, eagleRotZ = 18.295479;
          break;
          case SDLK_y:
            //Pause the animation.
            paused = true;
            momentumBeforePause = momentum;
            momentum = 0;

            //Set frame.
            frame = 224;

            //Move the camera.
            cameraX = 0.001829;
            cameraY = -0.000004;
            cameraZ = 0.00298;
            cameraR = 138;

            //Reset animation object positions and rotations.
            planePriorX = 0, planePriorY = 0, planePriorZ = 0;
            planePostX = 0, planePostY = -0.5829, planePostZ = 0;
            planeRotX = 0, planeRotY = 0, planeRotZ = 267.401611;

            eaglePriorX = -1, eaglePriorY = 0.03, eaglePriorZ = 0.4;
            eaglePostX = 0, eaglePostY = 0.000001, eaglePostZ = 0;
            eagleRotX = 0, eagleRotY = 0, eagleRotZ = 43.981647;
          break;
          case SDLK_u:
            //Pause the animation.
            paused = true;
            momentumBeforePause = momentum;
            momentum = 0;

            //Set frame.
            frame = -1;

            //Move the camera.
            cameraX = 0.000094;
            cameraY = -0.000036;
            cameraZ = -0.000103;
            cameraR = 42;

            //Reset animation object positions and rotations.
            planePriorX = 0, planePriorY = 0, planePriorZ = 0;
            planePostX = 0, planePostY = 0, planePostZ = 0;
            planeRotX = 0, planeRotY = 0, planeRotZ = 0;

            eaglePriorX = -0.5, eaglePriorY = 0.03, eaglePriorZ = -0.1;
            eaglePostX = 0, eaglePostY = 0, eaglePostZ = 0;
            eagleRotX = 0, eagleRotY = 0, eagleRotZ = 0;
          break;
          case SDLK_h:
            printf("\n\n*** Camera Controls ***\nUp: Accelerate\nDown: Decelerate\nLeft: Turn left\nRight: Turn right\nPage Up: Increase elevation\nPage down: Decrease elevation\n0: Stop moving\n\n*** Other Controls ***\nSpace: Pause animation - you may still move the camera.\nH: Display help\nP: Set position 1\nU: Set position 2\nY: Set position 3\nR: Reset\nQ: Quit\n\n");
          break;
        }
      break;
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
          //Set camera variables.
          case SDLK_LEFT: rotationDirection++;
          break;
          case SDLK_RIGHT: rotationDirection--;
          break;
          case SDLK_PAGEUP: elevationDirection++;
          break;
          case SDLK_PAGEDOWN: elevationDirection--;
          break;
          case SDLK_UP: momentumDirection--;
          break;
          case SDLK_DOWN: momentumDirection++;
          break;
        }
      break;
      case SDL_QUIT: exit(0);
    }
  }
}

//Entry point.
int main() {
  //Initialise SDL and OpenGL.
  init();

  //Loop will continue until an exit event is triggered.
  int before, after, elapsed;
  while (true) {
    before = SDL_GetTicks();

    display();
    events();

    //Lock redisplay to FPS rate.
    after = SDL_GetTicks();
    elapsed = after - before;
    if (elapsed < 1000 / FPS)
      SDL_Delay(1000 / FPS - (after - before));
  }

  return 0;
}
