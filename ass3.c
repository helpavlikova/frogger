/*
 *
 */
#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

// Program uses the Simple OpenGL Image Library for loading textures: http://www.lonesock.net/soil.html
#include <SOIL.h>

typedef struct
{
    float time;
    float value;
}Keyframe;


typedef struct
{
    float duration;
    float currTime;
    float currRotation;

    float initialVal;

    // Number of keyframes being used.
    int nKeyframes;

    // This will be restricted to 10 keyframes for your animation, feel free to change it.
    Keyframe keyframes[10];
} Interpolator;

Interpolator legInterpolators[6];

typedef struct { float x, y, z; } vec3f;

typedef struct { vec3f initialPos, position, v;
                 float lastT;
                 float radius;
                 bool collided;} state;
state projectile;
state cars[4];
state logs[4];

const float g = -9.8;
const int milli = 1000;

typedef struct {
  bool debug;
  bool go;
  float startTime;
  int frames;
  float frameRate;
  float frameRateInterval;
  float lastFrameRateT;
  float angle;
  float speed;
  float rotation;
  int circleNum;
  bool normalsOn;
  bool rightbtn;
  bool lightsOn;
  bool wireframe;
  int logColided;
  int lives;
  int score;
  bool died;
  bool won;
  bool gameOver;
} global_t;

global_t global = { true, false, 0, 0, 0, 0.2, 0, M_PI/4, 2.0, M_PI/2, 16, false, false, true, false, -1, 5, 0, 0, 0 };

typedef struct {
    float angleX;
    float angleY;
    float angleZ;
    float lastX;
    float lastY;
    float deltaX;
    float deltaY;
    float zoomScale;
    float x;
    float y;
} camera_t;

static GLuint roadTexture;
static GLuint grassTexture;
static GLuint sandTexture;
static GLuint woodTexture;
static GLuint skybox[6];

camera_t camera ={90.0, 0.0, 30.0, 0, 0, 0, 0, -0.7, 0.0, 0.0};
GLfloat green[] = {0.0, 1.0, 0.0};
GLfloat brown[] = {0.5, 0.3, 0.1};
GLfloat red[] = {1.0, 0.0, 0.0};
GLfloat white[] = {1.0, 1.0, 1.0};
GLfloat gray[] = {0.8, 0.8, 0.8};
GLfloat blue[] = {0.0, 0.0, 1.0};
GLfloat black[] = {0.0, 0.0, 0.0};
GLfloat lightBlue[] = {0.6, 0.8, 1.0};
GLfloat purple[] = {1.0, 0.0, 1.0};
GLfloat light_position[] = { 1.0, 1.0, 1.0, 1.0 };

float lerp(float t0, float v0, float t1, float v1, float t)
{
    return v0 + (t - t0) * (v1 - v0) / (t1 - t0);
}

float absFloat(float n) {
    if (n > 0)
        return n;
    else
        return -n;
}

static GLuint loadTexture(const char *filename)
{
    GLuint tex = SOIL_load_OGL_texture(filename, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
    if (!tex)
        return 0;

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

void resetGame() {
    global.lives = 5;
}

void resetInterpolators() {
    for(int i = 0; i < 5; i++) {
        legInterpolators[i].currRotation = legInterpolators[i].initialVal;
        legInterpolators[i].currTime = 0;
    }
}

void resetFrogger() {
    projectile.initialPos.x = projectile.position.x = -1.8;
    projectile.initialPos.y = projectile.position.y = 0;
    projectile.initialPos.z = projectile.position.z = 0;
}

void substractLife() {
    printf("lives: %d\n", global.lives);
    resetFrogger();
    global.died = 1;

    if (global.lives > 0) {
        global.lives--;
    } else {
        global.gameOver = true;
        resetGame();
    }
}

bool testCarCollision() {

    for(int i = 0; i < 4; i++){
        if( (projectile.position.x - cars[i].position.x) * (projectile.position.x - cars[i].position.x)
                + (projectile.position.z - cars[i].position.z) * (projectile.position.z - cars[i].position.z) <= 0.25 * 0.25) {
            cars[i].collided = true;
            return true;
        }
        else {
            cars[i].collided = false;

        }
    }
    return false;
}

int testLogCollision() {

    for(int i = 0; i < 4; i++){
        if((projectile.position.z >= logs[i].position.z - 0.5) && (projectile.position.z <= logs[i].position.z) &&
                (projectile.position.x - logs[i].position.x) * (projectile.position.x - logs[i].position.x) +
                (projectile.position.y - logs[i].position.y) * (projectile.position.y - logs[i].position.y) <= (0.05 + 0.05) * (0.05 + 0.05)){
            logs[i].collided = true;
            return i;
        }
        else{
            logs[i].collided = false;

        }
    }
    return -1;
}

void drawVector(float x, float y, float z, float a, float b, float c, float s) {
    float V = sqrt(a*a + b*b + c*c);

    a /= V;
    b /= V;
    c /= V;

    a*=s;
    b*=s;
    c*=s;

    a+=x;
    b+=y;
    c+=z;

    glPushAttrib(GL_CURRENT_BIT); //store current colour
    glBegin(GL_LINES);
    glColor3f (1.0, 0.0, 1.0);
    glVertex3f(x, y, z);
    glVertex3f(a, b, c);
    glEnd();
    glPopAttrib(); //restore previous colour
}

void drawNormals(float posX, float posZ) {
    glPushAttrib(GL_CURRENT_BIT);

    glBegin(GL_LINES);
    glColor3f(1.0,1.0,0.0);
    glVertex3f(posX, 0, posZ);
    glVertex3f(posX, 0.2, posZ);
    glEnd();

    glPopAttrib();
}

void drawAxes(float length) {
    glPushAttrib(GL_CURRENT_BIT); //store current colour

    glBegin(GL_LINES);
    glColor3f(1.0,0.0,0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(length, 0.0, 0.0);


    glColor3f(0.0,1.0,0.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, length, 0.0);


    glColor3f(0.0,0.0,1.0);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, length);

    glEnd();

    glPopAttrib(); //restore previous colour
}

void drawSquare(float gridX, float gridZ, float gridWidth, float depth, GLuint texture) {


    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBegin(GL_QUADS);

        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);
        glVertex3f(gridX, depth, gridZ);

        glNormal3f(0, 1, 0);
        glTexCoord2f(1, 0);
        glVertex3f(gridX + gridWidth, depth, gridZ);

        glNormal3f(0, 1, 0);
        glTexCoord2f(1, 1);
        glVertex3f(gridX + gridWidth, depth, gridZ - gridWidth);

        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 1);
        glVertex3f(gridX, depth, gridZ - gridWidth);
    glEnd();


    glDisable(GL_TEXTURE_2D);
}

void drawGridStrip(float a, float depth, GLuint texture) {
    float gridWidth = 0.5;
    float gridZ = -1.5; //because of the way drawSquare works

    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 2; j++){
            drawSquare(a, gridZ, gridWidth, depth, texture);
            a += gridWidth;
        }
        a -= 2* gridWidth;
        gridZ+= gridWidth;
    }

}

void drawTexturedQuad(float a, float b, float height, GLuint texture) {

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glNormal3f(0, 1, 0);
    glVertex3f(a, height, -2);
    glTexCoord2f(1, 0);
    glNormal3f(0, 1, 0);
    glVertex3f(a, height, 2);
    glTexCoord2f(1, 1);
    glNormal3f(0, 1, 0);
    glVertex3f(b, height, 2);
    glTexCoord2f(0, 1);
    glNormal3f(0, 1, 0);
    glVertex3f(b,height,-2);
    glEnd();


    glDisable(GL_TEXTURE_2D);
}

void drawSkybox() {

    float textureSize = 512.0;
    float width, height, length;
    width = height = length = 4;

    float texture0 = 1.0f / textureSize;
    float texture1 = (textureSize - 1) / textureSize;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, skybox[0]);
   glBegin(GL_QUADS);
   glNormal3f(0.0f, -1.0f, 0.0f);
   glTexCoord2d(texture0, texture0); glVertex3f( width, height, length);
   glTexCoord2d(texture0, texture1); glVertex3f(-width, height, length);
   glTexCoord2d(texture1, texture1); glVertex3f(-width, height, -length);
   glTexCoord2d(texture1, texture0); glVertex3f( width, height, -length);
   glEnd();

   glBindTexture(GL_TEXTURE_2D, skybox[1]);
   glBegin(GL_QUADS);
   glNormal3f(1.0f, 0.0f, 0.0f);
   glTexCoord2d(texture0, texture0); glVertex3f(-width, -height, length);
   glTexCoord2d(texture1, texture0); glVertex3f( width, -height, length);
   glTexCoord2d(texture1, texture1); glVertex3f( width, height, length);
   glTexCoord2d(texture0, texture1); glVertex3f(-width, height, length);
   glEnd();

   glBindTexture(GL_TEXTURE_2D, skybox[2]);
   glBegin(GL_QUADS);
   glNormal3f(-1.0f, 0.0f, 0.0f);
   glTexCoord2d(texture0, texture0); glVertex3f( width, -height, -length);
   glTexCoord2d(texture1, texture0); glVertex3f(-width, -height, -length);
   glTexCoord2d(texture1, texture1); glVertex3f(-width, height, -length);
   glTexCoord2d(texture0, texture1); glVertex3f( width, height, -length);
   glEnd();

   glBindTexture(GL_TEXTURE_2D, skybox[3]);
   glBegin(GL_QUADS);
   glNormal3f(0.0f, 0.0f, 1.0f);
   glTexCoord2d(texture0, texture0); glVertex3f(-width, -height, -length);
   glTexCoord2d(texture1, texture0); glVertex3f(-width, -height, length);
   glTexCoord2d(texture1, texture1); glVertex3f(-width, height, length);
   glTexCoord2d(texture0, texture1); glVertex3f(-width, height, -length);
   glEnd();

   glBindTexture(GL_TEXTURE_2D, skybox[4]);
   glBegin(GL_QUADS);
   glNormal3f(1.0f, 0.0f, -1.0f);
   glTexCoord2d(texture0, texture0); glVertex3f( width, -height, length);
   glTexCoord2d(texture1, texture0); glVertex3f( width, -height, -length);
   glTexCoord2d(texture1, texture1); glVertex3f( width, height, -length);
   glTexCoord2d(texture0, texture1); glVertex3f( width, height, length);
   glEnd();


    glDisable(GL_TEXTURE_2D);
}

void drawQuad(float a, float b) {
    glColor4f(0.3, 0.6, 1.0, 0.5);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(a, 0, -2);
    glNormal3f(0, 1, 0);
    glVertex3f(a, 0, 2);
    glNormal3f(0, 1, 0);
    glVertex3f(b, 0, 2);
    glNormal3f(0, 1, 0);
    glVertex3f(b,0,-2);
    glEnd();


}

void drawGrid(int n) {
    float gridWidth = 0.5;
    float gridX = -(gridWidth * n / 2.0);
    float gridZ = gridWidth * n / 2.0;
    float depth = 0;

    for(int i = 0; i < n; i++){ //for each column
       for( int j = 0; j < n; j++){ //one line
           //river bed lowering
           if (j >= 6)
               depth = -0.02;
           else
               depth = 0;

           drawSquare(gridX, gridZ, gridWidth, depth, grassTexture);

          if(global.normalsOn){
               glDisable(GL_LIGHTING);
               drawNormals(gridX, gridZ);
               drawNormals(gridX + gridWidth, gridZ);
               drawNormals(gridX, gridZ - gridWidth);
               drawNormals(gridX + gridWidth, gridZ - gridWidth);
               glEnable(GL_LIGHTING);
           }

           gridX += gridWidth;
        }
       gridZ -= gridWidth   ;
       gridX = -(gridWidth * n / 2.0);
    }
}

void drawSphere(int slices, int stacks, float r) {
    float theta, phi;
    float x1, y1, z1, x2, y2, z2;
    float step_phi = M_PI / stacks;

    //  glBegin(GL_QUAD_STRIP) - mistake to put it here! Why?
    for (int j = 0; j <= stacks; j++) {
      phi = j / (float)stacks * M_PI;
      glBegin(GL_QUAD_STRIP);
      for (int i = 0; i <= slices; i++) {
        theta = i / (float)slices * 2.0 * M_PI;
        x1 = r * sinf(phi) * cosf(theta);
        y1 = r * sinf(phi) * sinf(theta);
        z1 = r * cosf(phi);
        x2 = r * sinf(phi + step_phi) * cosf(theta);
        y2 = r * sinf(phi + step_phi) * sinf(theta);
        z2 = r * cosf(phi + step_phi);
        glNormal3f(x1, y1, z1); //has to be normalized
        glVertex3f(x1, y1, z1);
        glNormal3f(x2, y2, z2);
        glVertex3f(x2, y2, z2);
      }
      glEnd();
    }
}

void drawCircle(float r, float posZ) {
    float theta;
    float x1, y1, x2, y2;

    for (int i = 0; i <= global.circleNum; i++) {
      theta = i / (float)global.circleNum * 2.0 * M_PI;
      x1 = r * cosf(theta);
      y1 = r * sinf(theta);
      theta = (i+1) / (float)global.circleNum * 2.0 * M_PI;
      x2 = r * cosf(theta);
      y2 = r * sinf(theta);
      glBegin(GL_TRIANGLE_STRIP);
          glVertex3f(x1, y1, posZ);
          glVertex3f(x2, y2, posZ);
          glVertex3f(0, 0, posZ);
      glEnd();
    }
}

void drawCylinder( float r, float length) {
    float theta;
    float x,y;

//1st circle
    drawCircle(r, 0);

//2nd circle
    drawCircle(r, -length);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, woodTexture);

//joining circles
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= global.circleNum; i++) {
      theta = i / (float)global.circleNum * 2.0 * M_PI;
      x = r * cosf(theta);
      y = r * sinf(theta);
      glTexCoord2f(0, 0);
      glVertex3f(x, y, 0);
      glTexCoord2f(1, 0);
      glVertex3f(x, y, -length);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void drawCube(float x, float y, float z) {
//    drawAxes(0.5);

    glBegin(GL_QUADS);

    // top
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-x, y, z);
    glVertex3f(x, y, z);
    glVertex3f(x, y, -z);
    glVertex3f(-x, y, -z);

    // bottom
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-x, -y, z);
    glVertex3f(x, -y, z);
    glVertex3f(x, -y, -z);
    glVertex3f(-x, -y, -z);


    // front
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x, -y, z);
    glVertex3f(x, y, z);
    glVertex3f(-x, y, z);
    glVertex3f(-x, -y, z);


    // back
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x, -y, -z);
    glVertex3f(x, y, -z);
    glVertex3f(-x, y, -z);
    glVertex3f(-x, -y, -z);

    // right
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x, y, -z);
    glVertex3f(x, y, z);
    glVertex3f(x, -y, z);
    glVertex3f(x, -y, -z);

    // left
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-x, y, -z);
    glVertex3f(-x, y, z);
    glVertex3f(-x, -y, z);
    glVertex3f(-x, -y, -z);

    glEnd();
}

void mouseMotion(int x, int y) {
  //  printf("x: %d, y: %d\n",x,y);
    camera.deltaX = camera.lastX - x;
    camera.deltaY = camera.lastY - y;

    camera.lastX = x;
    camera.lastY = y;


    if(global.rightbtn == 1)
        camera.zoomScale -= camera.deltaY / 50.0;
    else{
        camera.angleX -= camera.deltaX;
        camera.angleY -= camera.deltaY;
        camera.angleZ -= camera.deltaY;
    }

    glutPostRedisplay();
}

void mouseMotionBtn(int button, int state,int x, int y) {


    if(button == GLUT_LEFT_BUTTON){
        global.rightbtn = 0;
       // printf("x is %d, y is %d\n",x,y);
    }

    if(button == GLUT_RIGHT_BUTTON){
        global.rightbtn = 1;
       // printf("zooming in\n");
    }
    camera.lastX = x;
    camera.lastY = y;
    glutPostRedisplay();
}

void drawHead() {

    glPushMatrix();
          glTranslatef(1, 0, 0);
          //neck rotations for animation go here

          glPushMatrix();
              glTranslatef(1.3, 0, 0); //translate to centre of head
              drawCube(0.6, 0.8, 1.0); //draw head
          glPopMatrix();

          glPushMatrix();
              glTranslatef(2.0, 0.3, 0.5); //translate to centre of eyes
              glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
              drawCube(0.4, 0.3, 0.3); //draw eye ball

              glTranslatef(0.4, 0.0, 0.0); //translate to centre of eyes
              glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black);
              drawCube(0.1, 0.2, 0.2); //draw eye
          glPopMatrix();


          glPushMatrix();
              glTranslatef(2.0, 0.3, -0.5); //translate to centre of eyes
              glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
              drawCube(0.4, 0.3, 0.3); //draw eye ball


              glTranslatef(0.4, 0.0, 0.0); //translate to centre of eyes
              glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black);
              drawCube(0.1, 0.2, 0.2); //draw eye
          glPopMatrix();

          glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, green);

          glPushMatrix();
              glTranslatef(2.0, -0.3, 0.0); //translate to centre of mouth
               glRotatef(10,0,0,1); //upper jaw interpolator
              drawCube(1.0, 0.2, 1.0); //draw upper jaw
          glPopMatrix();


          glPushMatrix();
              glTranslatef(2.0, -0.6, 0.0); //translate to centre of mouth
              glRotatef(-10,0,0,1); //lower jaw interpolator
              drawCube(1.0, 0.2, 1.0); //draw lower jaw
          glPopMatrix();


      glPopMatrix();
}

void drawFingers() {
    glPushMatrix();
       glTranslatef(0,-0.3,0.5);//  translate to centre of fingers
       glRotatef(-30, 1, 0, 0);
       drawCube(0.1, 0.4, 0.1);//  render finger
    glPopMatrix();


    glPushMatrix();
       glTranslatef(0,-0.3,-0.5);//  translate to centre of fingers
       glRotatef(30, 1, 0, 0);
       drawCube(0.1, 0.4, 0.1);//  render finger
    glPopMatrix();


    glPushMatrix();
       glTranslatef(0,-0.3,0);//  translate to centre of fingers
       drawCube(0.1, 0.4, 0.1);//  render finger
    glPopMatrix();
}

void drawFrontLeg() {

    glRotatef(legInterpolators[1].currRotation, 0, 0, 1);//shoulder rotations for animation go here

        glPushMatrix();
            glTranslatef(0,-0.8,0);//translate to centre of upper arm
            drawCube(0.3, 0.8, 0.5); //render upper arm

            glPushMatrix();
               glTranslatef(0,-0.8,0); // translate to elbow
               glRotatef(legInterpolators[2].currRotation, 0, 0, 1);// elbow rotations for animation go here

                glPushMatrix();
                  glTranslatef(0,-1.5,0);//  translate to centre of lower arm
                  drawCube(0.4, 1.5, 0.4);//  render lower arm

                  glPushMatrix();
                     glTranslatef(0,-1.5,0);// translate to knuckles
                     drawFingers();

                  glPopMatrix();


                glPopMatrix();
            glPopMatrix();
        glPopMatrix();
}

void drawRearLeg() {
    glRotatef(legInterpolators[3].currRotation, 0, 0, 1);//hip rotations for animation go here

    glPushMatrix();
        glTranslatef(0,-0.8,0);//translate to centre of upper leg
        drawCube(0.3, 0.8, 0.3); //render upper leg

        glPushMatrix();
           glTranslatef(0,-0.8,0); // translate to elbow
           glRotatef(-130, 0, 0, 1);// knee rotations for animation go here

            glPushMatrix();
              glTranslatef(0,-0.8,0);//  translate to centre of lower leg
              drawCube(0.3, 0.8, 0.3);//  render lower leg

              glPushMatrix();
                 glTranslatef(0,-0.5,0);// translate to leg joint
                 glRotatef(150, 0, 0, 1);// lower leg rotations for animation go here

                 glPushMatrix();
                    glTranslatef(0,-1.2,0);//  translate to centre of leg
                    drawCube(0.3, 1.2, 0.3);//  render leg

                    glPushMatrix();
                       glTranslatef(0,-1.2,0);// translate to knuckles
                       drawFingers();

                    glPopMatrix();


                 glPopMatrix();

              glPopMatrix();


            glPopMatrix();
        glPopMatrix();
    glPopMatrix();
}

void drawFrogger(void) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, green);
    glPushMatrix ();
        glTranslatef (projectile.position.x, projectile.position.y+0.05, projectile.position.z);
        glScalef(0.02,0.02,0.02);
       // drawSphere(global.circleNum,global.circleNum,projectile.radius);

        glRotatef(global.rotation * 180/ M_PI - 90, 0, 1, 0);
        glRotatef(legInterpolators[0].currRotation, 0, 0, 1); //rotation of main body
            drawCube(2, 1.0, 1.2); //main body
            drawHead();
            glPushMatrix();
                glTranslatef(1.5,- 0.8, 1.3);//translate to right shoulder
                glRotatef(-30, 0, 1, 0);
                drawFrontLeg();

            glPopMatrix();

            glPushMatrix();
                glTranslatef(1.5,- 0.8, -1.3);//translate to left shoulder
                glRotatef(30, 0, 1, 0);
                drawFrontLeg();
            glPopMatrix();


            glPushMatrix();
                glTranslatef(-2,- 0.2, 1.2); //translate to right hip
                glRotatef(-30, 0, 1, 0);
                drawRearLeg();
            glPopMatrix();


            glPushMatrix();
                glTranslatef(-2,- 0.2, -1.2); //translate to left hip
                glRotatef(30, 0, 1, 0);
                drawRearLeg();
            glPopMatrix();




    glPopMatrix ();

}

void drawWindows(void) {
    //left window
    glPushMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, lightBlue);
    glTranslatef (0, 0.04, 0.25);
    drawCube(0.20, 0.06, 0.001);
    glPopMatrix();

    //right window
    glPushMatrix();
    glTranslatef (0, 0.04, -0.25);
    drawCube(0.20, 0.06, 0.001);
    glPopMatrix();

    //front window
    glPushMatrix();
    glTranslatef (0.25, 0.04, 0);
    drawCube(0.001, 0.06, 0.20);
    glPopMatrix();

    //back window
    glPushMatrix();
    glTranslatef (-0.25, 0.04, 0);
    drawCube(0.001, 0.06, 0.20);
    glPopMatrix();
}

void drawWheels() {

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, black);
    glPushMatrix();
        glTranslatef (-0.35, -0.18, 0.26);
        drawCylinder(0.1,0.05);
    glPopMatrix();

    glPushMatrix();
        glTranslatef (0.35, -0.18, 0.26);
        drawCylinder(0.1,0.05);
    glPopMatrix();

    glPushMatrix();
        glTranslatef (-0.35, -0.18, -0.26);
        drawCylinder(0.1,0.05);
    glPopMatrix();

    glPushMatrix();
        glTranslatef (0.35, -0.18, -0.26);
        drawCylinder(0.1,0.05);
    glPopMatrix();
}

void drawCar() {
     glPushMatrix();
        glScalef(0.5,0.5,0.5);
        glTranslatef (0, 0.2, 0);
        glRotatef (90, 0, 1, 0);
        drawCube(0.5, 0.1, 0.25);

        glPushMatrix();
            glTranslatef (0, 0.1, 0);
            drawCube(0.25, 0.15, 0.25);
            glPushMatrix();
                drawWindows();
            glPopMatrix();

            drawWheels();

        glPopMatrix();

     glPopMatrix();
}

void updateVelocity() {
    projectile.v.x = sin(global.rotation) * cos(global.angle) * global.speed;
    projectile.v.y = sin(global.angle) * global.speed;
    projectile.v.z = cos(global.angle) * cos(global.rotation)* global.speed;
}

void updateInterpolator(float dt, int i){
    float duration =(2.0 * global.speed * sinf(global.angle)) / - g;

    if (legInterpolators[i].currTime >= duration){
        legInterpolators[i].currTime = 0;
        return;
    }

    int currKeyframe;

    for (currKeyframe = 0; currKeyframe < legInterpolators[i].nKeyframes; currKeyframe ++){
        if(legInterpolators[i].keyframes[currKeyframe].time * duration >= legInterpolators[i].currTime){
            break;
        }
    }

    legInterpolators[i].currRotation = lerp(legInterpolators[i].keyframes[currKeyframe - 1].time * duration, legInterpolators[i].keyframes[currKeyframe - 1].value,
            legInterpolators[i].keyframes[currKeyframe].time * duration, legInterpolators[i].keyframes[currKeyframe].value, legInterpolators[i].currTime);


    legInterpolators[i].currTime += dt;

   // printf("currRotation: %f, currTime %f\n", currRotation, interpolator.currTime);
}

void setPosition() {
      projectile.lastT = global.startTime;
      projectile.position.y = 0;
      projectile.initialPos.x =  projectile.position.x;
      projectile.initialPos.y = projectile.position.y;
      projectile.initialPos.z = projectile.position.z;

}

void setCars(int i) {
    cars[i].position.x = - 1.2 + 0.5 * i;
}

void updateProjectileStateNumerical(float dt)
{
  // Euler integration

  // Position
  projectile.position.x += projectile.v.x * dt;
  projectile.position.y += projectile.v.y * dt;
  projectile.position.z += projectile.v.z * dt;


  // Velocity
  projectile.v.y += g * dt;


}

void drawParametricParabola() {
    float v0 = global.speed;
    float flightTime = (2.0 * v0 * sinf(global.angle)) / -g;

    float x, y,t;

    glPointSize(2.0);

    glPushAttrib(GL_CURRENT_BIT);
    glColor3f(0.0,0.0,1.0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= global.circleNum; i++) {
      t = flightTime / global.circleNum * i;
      x = v0 * t * cosf(global.angle);
      y = v0 * t * sinf(global.angle) - (0.5 *-g *t * t);
      glVertex2f(x, y);
    }
    glEnd();

    glPopAttrib();
    //normals and tangents
    for (int i = 0; i <= global.circleNum; i++) {
      t = flightTime / global.circleNum * i;
      x = v0 * t * cosf(global.angle);
      y = v0 * t * sinf(global.angle) - (0.5 *-g *t * t);

      float derX = v0 * cosf(global.angle);
      float derY = v0 * sinf(global.angle) + t *g;

      if(global.normalsOn){ //tangents
        glDisable(GL_LIGHTING);
        glColor3f(0.0,1.0,1.0);
        drawVector(x,y,0.0,derX,derY,0.0,0.0);
        printf("drawing tangents\n");
        glEnable(GL_LIGHTING);
      }

    }

}

void checkCollisions(float dt) {
    projectile.collided = testCarCollision();

    if(projectile.collided){
        projectile.collided = false;
       substractLife();
    }


    if( global.logColided != -1 && !global.go){
        projectile.initialPos.z = projectile.position.z += dt * logs[global.logColided].v.x;
    } else {
        global.logColided = testLogCollision();

    }

    //falling into the river
    if ( global.logColided == -1 && projectile.position.x >= 1 && projectile.position.x < 2 && projectile.position.y <= 0.05){
        printf("Fell into a river!\n");
        substractLife();
    }

    //staying on log forever
    if ( projectile.position.x >= 1 && projectile.position.x < 2 && projectile.position.y <= 0.05 && projectile.position.z > 2){
        printf("Fell into a river!\n");
        global.logColided = -1;
        substractLife();
    }

}

void displayOSD() {
  char buffer[30];
  char *bufp;
  int w, h;

  glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  /* Set up orthographic coordinate system to match the
     window, i.e. (0,0)-(w,h) */
  w = glutGet(GLUT_WINDOW_WIDTH);
  h = glutGet(GLUT_WINDOW_HEIGHT);
  glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  /* Controls */
  glColor3f(1.0, 1.0, 0.0);
  glRasterPos2i(30, 130);
  snprintf(buffer, sizeof buffer, "CONTROLS:");
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  glRasterPos2i(30, 110);
  snprintf(buffer, sizeof buffer, "Jump: Space");
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  glRasterPos2i(30, 90);
  snprintf(buffer, sizeof buffer, "Jump angle: W/S/A/D");
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  glRasterPos2i(30, 70);
  snprintf(buffer, sizeof buffer, "Direction: Left/Right keys");
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  glRasterPos2i(30, 50);
  snprintf(buffer, sizeof buffer, "Camera rotation: Left mouse BTN");
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  glRasterPos2i(30, 30);
  snprintf(buffer, sizeof buffer, "Zoom: Right mouse BTN");
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);



  /* Lives */
  glColor3f(0.0, 1.0, 0.0);
  glRasterPos2i(w/2 - 20, h - 30);
  snprintf(buffer, sizeof buffer, "LIVES LEFT: %d", global.lives);
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  /* Score */
  glColor3f(0.0, 1.0, 0.0);
  glRasterPos2i(w/2 - 20, h - 50);
  snprintf(buffer, sizeof buffer, "SCORE: %d", global.score);
  for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);


  /* Died */
  if(global.gameOver) {
      glColor3f(1.0, 0.0, 0.0);
      glRasterPos2i(w/2 - 20, h / 2 - 50);
      snprintf(buffer, sizeof buffer, "GAME OVER");
      for (bufp = buffer; *bufp; bufp++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *bufp);


      glRasterPos2i(w/2 - 120, h / 2 - 80);
      snprintf(buffer, sizeof buffer, "PRESS RETURN TO TRY AGAIN");
      for (bufp = buffer; *bufp; bufp++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *bufp);
  }



  /* Pop modelview */
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);

  /* Pop projection */
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  /* Pop attributes */
  glPopAttrib();
}

void display(void) {
    GLenum err;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    if(!global.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    if(global.lightsOn){
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    }

    //camera transformations
    glTranslatef(0,0,camera.zoomScale);
    glRotatef(camera.angleY,1.0,0.0,0.0);
    glRotatef(camera.angleX,0.0,1.0,0.0); //interactive rotation
    glRotatef(camera.angleZ,0.0,0.0,1.0);
    glTranslatef(-projectile.position.x, -projectile.position.y, -projectile.position.z); //camera following the sphere


    glDisable(GL_LIGHTING);
    drawSkybox();
    displayOSD();
    glEnable(GL_DEPTH_TEST);


     //drawAxes(2.0);
     drawVector(projectile.position.x,projectile.position.y, projectile.position.z, projectile.v.x, projectile.v.y,projectile.v.z, global.speed * 0.25);


     glPushMatrix();
         glTranslatef(projectile.initialPos.x,projectile.initialPos.y, projectile.initialPos.z);
         glRotatef(global.rotation * 180/ M_PI - 90 ,0.0,1.0,0.0);
         drawParametricParabola();
     glPopMatrix();

    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position); //after the camera transformations!


    //road
    drawTexturedQuad(-1.5, 0.5, 0.01, roadTexture);


    drawFrogger();



    //grass
    drawGrid(8);
    drawGridStrip(-3, 0, grassTexture);
    drawGridStrip(2, 0, grassTexture);


    //logs
    for(int i = 0; i < 4; i++){
        glPushMatrix();
        glTranslatef(logs[i].position.x, 0, logs[i].position.z);
        if(logs[i].collided) {
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, green);
        }
        else {
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, brown);
        }
            drawCylinder(0.05,0.5);
        glPopMatrix();
    }

    //cars
    for(int i = 0; i < 4; i++){
        glPushMatrix();
            glTranslatef(cars[i].position.x, 0, cars[i].position.z);
            if(cars[i].collided) {
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, green);
            }
            else {
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, red);
            }
            drawCar();
        glPopMatrix();
    }

    //river
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
    drawGridStrip(1.0, -0.01, sandTexture);

    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(1.0, 2);
    glDisable(GL_BLEND);


    glutSwapBuffers();

    global.frames++;

    // Check for errors
    while ((err = glGetError()) != GL_NO_ERROR)
        printf("%s\n",gluErrorString(err));
}

void update(void) {
  float t, dt;

  t = glutGet(GLUT_ELAPSED_TIME) / (float)milli;


  dt = t - cars[0].lastT;

    for(int i = 0; i < 4; i++){
        cars[i].position.z += dt * cars[i].v.x;
        if(cars[i].position.z > 2){
            cars[i].position.z = -2;
            setCars(i);
            cars[i].v.x = rand()/(RAND_MAX/3.0) + 1;
        }


        logs[i].position.z += dt * logs[i].v.x;
        if(logs[i].position.z > 2){
            logs[i].position.z = -2;
            logs[i].position.x = (float)rand()/(float)(RAND_MAX/0.9) + 1.1;
            logs[i].v.x = (float)rand()/(float)(RAND_MAX/1.0) + 0.5;
        }
    }

    checkCollisions(dt);

    //check if player won
    if(projectile.position.x > 2){
        printf("Yay, you made it!\n");
        global.won = 1;
        global.score++;
        resetFrogger();
    }


  if (projectile.lastT < 0.0) {
    projectile.lastT = t;
    return;
  }


  cars[0].lastT = t;

  glutPostRedisplay();


  dt = t - projectile.lastT;
  projectile.lastT = t;

  if(projectile.position.y < 0){
      setPosition();
      updateVelocity();
      resetInterpolators();
      display();
      global.go = false;
  }

  if (!global.go) { //stopping frog animation when it reaches y=0
    return;
  }



  if (global.debug)
   // printf("%f %f\n", t, dt);

  updateProjectileStateNumerical(dt);

  //interpolation
  for(int i = 0; i < 5; i ++){
        updateInterpolator(dt, i);
  }


  /* Frame rate */
  dt = t - global.lastFrameRateT;
  if (dt > global.frameRateInterval) {
    global.frameRate = global.frames / dt;
    global.lastFrameRateT = t;
    global.frames = 0;
  }

  glutPostRedisplay();
}

void initInterpolators() {
    for(int i = 0; i < 5; i++){
        legInterpolators[i].currTime = 0;
        legInterpolators[i].duration = 10;
        legInterpolators[i].nKeyframes = 2;

        legInterpolators[i].keyframes[0].time = 0;
        legInterpolators[i].keyframes[1].time = 0.5;
        legInterpolators[i].keyframes[2].time = 1;
    }

    //torso
    legInterpolators[0].initialVal =  0;
    legInterpolators[0].keyframes[0].value =  0;
    legInterpolators[0].keyframes[1].value = 60;
    legInterpolators[0].keyframes[2].value = 0;

    //upper front leg
    legInterpolators[1].initialVal = -60;
    legInterpolators[1].keyframes[0].value = -60;
    legInterpolators[1].keyframes[1].value = 0;
    legInterpolators[1].keyframes[2].value = -60;

    //lower front leg
    legInterpolators[2].initialVal = 150;
    legInterpolators[2].keyframes[0].value = 150;
    legInterpolators[2].keyframes[1].value = -60;
    legInterpolators[2].keyframes[2].value = 150;


    //upper rear leg
    legInterpolators[3].initialVal = 70;
    legInterpolators[3].keyframes[0].value = 70;
    legInterpolators[3].keyframes[1].value = -60;
    legInterpolators[3].keyframes[2].value = 70;

    resetInterpolators();

}

void initSkybox() {
    skybox[0] = loadTexture("skybox/posy.jpg");
    skybox[1] = loadTexture("skybox/posx.jpg");
    skybox[2] = loadTexture("skybox/negx.jpg");
    skybox[3] = loadTexture("skybox/negz.jpg");
    skybox[4] = loadTexture("skybox/posz.jpg");

}

void myinit (void)
{
    initInterpolators();

    cars[0].lastT = global.startTime;
    glEnable(GL_NORMALIZE);

    roadTexture = loadTexture("road.png");
    grassTexture = loadTexture("grass.jpg");
    sandTexture = loadTexture("sand.jpg");
    woodTexture = loadTexture("wood.jpg");

    initSkybox();

    projectile.initialPos.x = projectile.position.x = -1.8;


    projectile.radius = 0.05;
    updateVelocity();
    srand(time(NULL));
    for(int i = 0; i < 4; i++){
        setCars(i);
        cars[i].v.x = (float)rand()/(float)(RAND_MAX/0.5) + 1;

        logs[i].position.x = (float)rand()/(float)(RAND_MAX/0.8) + 1;
        logs[i].v.x = (float)rand()/(float)(RAND_MAX/0.5) + 0.5;
    }
}

void handleSpecialKeypress(int key, int x, int y) {
 switch (key) {
     case GLUT_KEY_LEFT:
       global.rotation += M_PI/24;
       updateVelocity();
       display();
       break;
     case GLUT_KEY_RIGHT:
       global.rotation -= M_PI/24;
       updateVelocity();
       display();
       break;
 }
}

void keyboardCB(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
  case 'q':
    exit(EXIT_SUCCESS);
    break;
  case 'b':
    global.debug = !global.debug;
    break;
  case 'l':
    global.lightsOn = !global.lightsOn;
    break;
  case 'p':
    global.wireframe = !global.wireframe;
    break;
  case ' ':
    if (!global.go && !global.gameOver) {
        printf("-------------------JUMP-------------------\n");
      global.startTime = glutGet(GLUT_ELAPSED_TIME) / (float)milli;
      printf("start time: %f\n",global.startTime);
      updateVelocity();
      projectile.position.y = 0;
      global.go = true;
      setPosition();
      updateVelocity();
    }
    break;
  case 13: // pressing the return key
      global.gameOver = false;
    break;
  case 'a':
    global.angle += M_PI/24;
    printf("angle: %f\n", global.angle);
    updateVelocity();
    break;
  case 'd':
    global.angle -= M_PI/24;
    printf("angle: %f\n", global.angle);
    updateVelocity();
    break;
  case 'w':
    if(global.speed <= 3.0) {
        global.speed += 0.2;
    }
    updateVelocity();
    break;
  case 's':
    global.speed -= 0.2;
    updateVelocity();
    break;
  case '=': // up
    global.circleNum *= 2.0;
    break;
  case '-': // down
    global.circleNum /= 2.0;
    break;
  case 'n':
    global.normalsOn = !global.normalsOn;
    break;
  default:
    break;
  }
  glutPostRedisplay();
}

void reshape(int w, int h)
{
   // printf("width: %d height: %d\n", w,h);

  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);	// Set the matrix we want to manipulate
  glLoadIdentity();			// Overwrite the contents with the identity
  gluPerspective(75.0,(float) w / (float) h, 0.01,100);		// Multiply the current matrix with a generated perspective matrix
  glMatrixMode(GL_MODELVIEW);	// Change back to the modelview matrix
}

/*  Main Loop
 *  Open window with initial window size, title bar,
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(1000, 800);
  glutInitWindowPosition(500, 500);
  glutCreateWindow("Assignment 2");
  glutKeyboardFunc(keyboardCB);
  glutSpecialFunc(handleSpecialKeypress);

  glutMotionFunc(mouseMotion);
  glutMouseFunc(mouseMotionBtn);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutIdleFunc(update);

  myinit();

  glutMainLoop();

  return 0;
}
