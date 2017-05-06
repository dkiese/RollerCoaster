/**
 * Author:	Andrew Robert Owens
 * Email:	arowens [at] ucalgary.ca
 * Date:	January, 2017
 * Course:	CPSC 587/687 Fundamental of Computer Animation
 * Organization: University of Calgary
 *
 * Copyright (c) 2017 - Please give credit to the author.
 *
 * File:	main.cpp
 *
 * Summary:
 *
 * This is a (very) basic program to
 * 1) load shaders from external files, and make a shader program
 * 2) make Vertex Array Object and Vertex Buffer Object for the quad
 *
 * take a look at the following sites for further readings:
 * opengl-tutorial.org -> The first triangle (New OpenGL, great start)
 * antongerdelan.net -> shaders pipeline explained
 * ogldev.atspace.co.uk -> good resource
 */

#include <iostream>
#include <cmath>
#include <chrono>
#include <limits>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>

#include "ShaderTools.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "OpenGLMatrixTools.h"
#include "Camera.h"
#include "Vec3f_FileIO.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>

//==================== GLOBAL VARIABLES ====================//
/*	Put here for simplicity. Feel free to restructure into
*	appropriate classes or abstractions.
*/

// Drawing Program
GLuint basicProgramID;

// Data needed for Quad
GLuint vaoID;
GLuint vertBufferID;
Mat4f M;
// Data needed for left Rail
GLuint vaoID2;
GLuint vertBufferID2;
Mat4f M2;

// Data needed for Line 
GLuint line_vaoID;
GLuint line_vertBufferID;
Mat4f line_M;

// Only one camera so only one veiw and perspective matrix are needed.
Mat4f V;
Mat4f P;

// Only one thing is rendered at a time, so only need one MVP
// When drawing different objects, update M and MVP = M * V * P
Mat4f MVP;

// Camera and veiwing Stuff
Camera camera;
int g_moveUpDown = 0;
int g_moveLeftRight = 0;
int g_moveBackForward = 0;
int g_rotateLeftRight = 0;
int g_rotateUpDown = 0;
int g_rotateRoll = 0;
float g_rotationSpeed = 0.015625;
float g_panningSpeed = 0.25;
bool g_cursorLocked;
float g_cursorX, g_cursorY;

bool g_play = false;

int WIN_WIDTH = 800, WIN_HEIGHT = 600;
int FB_WIDTH = 800, FB_HEIGHT = 600;
float WIN_FOV = 60;
float WIN_NEAR = 0.01;
float WIN_FAR = 1000;

std::vector<Vec3f> verts;
std::vector<Vec3f> leftverts;
std::vector<Vec3f> subVerts;
std::vector<Vec3f> drawsubVerts;
std::vector<Vec3f> ndrawsubVerts;
int currentPoint = 0;
int prevcurrentPoint = 0;
int pointMax;
float maxPoint = 0.0;

std::string file;
int amountofLines;
int amountofQuads;

//checking every single line segment
float d = 0;

//==================== FUNCTION DECLARATIONS ====================//
void displayFunc();
void resizeFunc();
void init();
void generateIDs();
void deleteIDs();
void setupVAO();
void loadQuadGeometryToGPU();
void reloadProjectionMatrix();
void loadModelViewMatrix();
void setupModelViewProjectionTransform();

void windowSetSizeFunc();
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowSetSizeFunc(GLFWwindow *window, int width, int height);
void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height);
void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods);
void windowMouseMotionFunc(GLFWwindow *window, double x, double y);
void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods);
void animateQuad(float t);
void moveCamera();
void reloadMVPUniform();
void reloadColorUniform(float r, float g, float b);
std::string GL_ERROR();
int main(int, char **);
void subdivide(std::vector<Vec3f> vecs);
std::vector<Vec3f> scale(std::vector<Vec3f> vectors, float scaler);

//==================== FUNCTION DEFINITIONS ====================//

void displayFunc() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use our shader
  glUseProgram(basicProgramID);

  // ===== DRAW QUAD ====== //
  MVP = P * V * M;
  reloadMVPUniform();
  reloadColorUniform(1, 1, 0);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(vaoID);
  // Draw Quads, start at vertex 0, draw 4 of them (for a quad)
  glDrawArrays(GL_TRIANGLE_STRIP, 0, amountofQuads); //24

  // ==== DRAW LINE ===== //
  MVP = P * V * line_M;
  reloadMVPUniform();

  reloadColorUniform(0, 1, 1);

  // Use VAO that holds buffer bindings
  // and attribute config of buffers
  glBindVertexArray(line_vaoID);
  // Draw lines
  glDrawArrays(GL_LINES, 0, amountofLines);
  
}

float distance(Vec3f p1, Vec3f p2){
	 return sqrt( pow((p1.x()-p2.x()),2) + pow((p1.y()-p2.y()),2) + pow((p1.z()-p2.z()),2) );
}


void animateQuad(float t) {
  M = RotateAboutYMatrix(1 * t); //50

  //float s = (std::sin(t) + 1.f) / 2.f;
  float s;	

  float deltaX = 0;
  float deltaY = 0;
  float deltaZ = 0;

  // get length of decel stage
  float decelstagelength;
  Vec3f h1;
  Vec3f h2;
  for(int i = 64360; i< ndrawsubVerts.size(); i+=2){
  	if(i == ndrawsubVerts.size()-2){
		h1 = ndrawsubVerts[i];
		h2 = ndrawsubVerts[0];
	}
	else{
		h1 = ndrawsubVerts[i];
		h2 = ndrawsubVerts[i+1];
	}	
	decelstagelength += distance(h1,h2); 
   }

 // get length of stage
  float stagelength;
  Vec3f z1;
  Vec3f z2;
  for(int i = 0; i< ndrawsubVerts.size(); i+=2){
  	if(i == ndrawsubVerts.size()-2){
		z1 = ndrawsubVerts[i];
		z2 = ndrawsubVerts[0];
	}
	else{
		z1 = ndrawsubVerts[i];
		z2 = ndrawsubVerts[i+1];
	}	
	stagelength += distance(h1,h2); 
   }
   //printf("Decelstagelength: %f\n", decelstagelength);
  
  /*for(int i = 0; i<ndrawsubVerts.size(); i++){
	if(ndrawsubVerts[currentPoint].y() > maxPoint){
			maxPoint = ndrawsubVerts[currentPoint].y();
			pointMax = currentPoint;
			printf("MAXPOINT: %i\n" ,pointMax);}
 }*/
  //printf("ndrawsubVerts: %i\n" , ndrawsubVerts.size());
  for(int i = currentPoint; i<ndrawsubVerts.size(); i+=2){

	//constant speed
  	if(currentPoint >= 0 && currentPoint <= 6134){
		s = 1.0;
		}
	
	//free falling stage
	if((currentPoint >= 6136 && currentPoint <= 16658)){
		//printf("max y coordinate: %f, ", ndrawsubVerts[35520]);
		//printf("current y coordinate: %f\n", ndrawsubVerts[currentPoint]);
		float h = ndrawsubVerts[6136].y() - ndrawsubVerts[currentPoint].y();
		//printf("Change in H: %f\n", h);
		float vdata = 2*9.81*h;
		//printf("vdata: %f\n", vdata);
		float velocity = sqrt(vdata);
		//printf("velocity: %f\n", velocity);
		s = velocity;

		}
	//lifting
  	if(currentPoint >= 16660 && currentPoint <= 18446){
		s = 0.5;
		}

	//free falling stage
	if((currentPoint >= 18448 && currentPoint <= 24998)){ //23178
		//printf("max y coordinate: %f, ", ndrawsubVerts[35520]);
		//printf("current y coordinate: %f\n", ndrawsubVerts[currentPoint]);
		float h = ndrawsubVerts[18448].y() - ndrawsubVerts[currentPoint].y();
		//printf("Change in H: %f\n", h);
		float vdata = 2*9.81*h;
		//printf("vdata: %f\n", vdata);
		float velocity = sqrt(vdata);
		//printf("velocity: %f\n", velocity);
		s = velocity;

		}

	 //lifting stage
	 if(currentPoint >= 25000 && currentPoint <= 36876){
		s = 0.5;
		}

	  //free falling stage
	 if(currentPoint >= 36878 && currentPoint <= 64358){
		//printf("max y coordinate: %f, ", ndrawsubVerts[35520]);
		//printf("current y coordinate: %f\n", ndrawsubVerts[currentPoint]);
		float h = ndrawsubVerts[36878].y() - ndrawsubVerts[currentPoint].y();
		//printf("Change in H: %f\n", h);
		float vdata = 2*9.81*h;
		//printf("vdata: %f\n", vdata);
		float velocity = sqrt(vdata);
		//printf("velocity: %f\n", velocity);
		s = velocity;

		}
	//deceleration stage
      float currentdistance;
	if(currentPoint >= 64360){
		Vec3f l1 = ndrawsubVerts[currentPoint];
		Vec3f l2;
		if(currentPoint >= 64360)
			l2 = ndrawsubVerts[73728];
		else
			l2 = ndrawsubVerts[29000];
		// get length of the track	
		currentdistance = distance(l1,l2);
		s = currentdistance/decelstagelength; 
	}
	
	//unload
	//if(currentPoint == 73728){
		//sleep(2000);	
	//}

	s = s/stagelength; // normalize the speeds
	s = s*4; // double the speed
	Vec3f p1;
	Vec3f p2;	
	if(i == ndrawsubVerts.size()-2){
		p1 = ndrawsubVerts[i];
		p2 = ndrawsubVerts[0];
		currentPoint = 0;
		i = 0;
	}
	else{
		p1 = ndrawsubVerts[i];
		p2 = ndrawsubVerts[i+1];
	}

	
	d += distance(p1,p2);
	// we can draw this segment
	//printf("Change in S: %f\n" , s);
	//printf("distance: %f\n" , d);
	if(d > s){
		deltaX = ndrawsubVerts[prevcurrentPoint].x();
		deltaY = ndrawsubVerts[prevcurrentPoint].y();
		deltaZ = ndrawsubVerts[prevcurrentPoint].z();
		d = 0;
		prevcurrentPoint = currentPoint;
		currentPoint = i+2;
		printf("previous point: %i\n" , prevcurrentPoint);
		printf("current point: %i\n" , currentPoint);
		break;
	}
	if(d ==s){
		deltaX = p1.x();
		deltaY = p1.y();
		deltaZ = p1.z();
		d = 0;
		prevcurrentPoint = currentPoint;
		currentPoint = i+2;
		printf("previous point: %i\n" , prevcurrentPoint);
		printf("current point: %i\n" , currentPoint);
		break;
	}
	else{
		//printf("d is less than s!");
	}
	}

  M = TranslateMatrix(deltaX, deltaY, deltaZ) * M;
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}


void loadQuadGeometryToGPU() {
  // Just basic layout of floats, for a quad
  // 3 floats per vertex, 4 vertices
  // lets draw some sorta cube
	//top face
  verts.push_back(Vec3f(1, 1, -1));
  verts.push_back(Vec3f(-1, 1, -1));
  verts.push_back(Vec3f(-1, 1, 1));
  verts.push_back(Vec3f(1, 1, 1));
	//bottom face
  verts.push_back(Vec3f(1, -1, 1));
  verts.push_back(Vec3f(-1, -1, 1));
  verts.push_back(Vec3f(-1, -1, -1));
  verts.push_back(Vec3f(1, -1, -1));
	// Front face
  verts.push_back(Vec3f( 1.0,  1.0, 1.0));
  verts.push_back(Vec3f(-1.0,  1.0, 1.0));
  verts.push_back(Vec3f(-1.0, -1.0, 1.0));
  verts.push_back(Vec3f( 1.0, -1.0, 1.0));
 
      // Back face 
  verts.push_back(Vec3f( 1.0, -1.0, -1.0));
  verts.push_back(Vec3f(-1.0, -1.0, -1.0));
  verts.push_back(Vec3f(-1.0,  1.0, -1.0));
  verts.push_back(Vec3f( 1.0,  1.0, -1.0));
 
      // Left face (x = -1.0f)
  verts.push_back(Vec3f(-1.0,  1.0,  1.0));
  verts.push_back(Vec3f(-1.0,  1.0, -1.0));
  verts.push_back(Vec3f(-1.0, -1.0, -1.0));
  verts.push_back(Vec3f(-1.0, -1.0,  1.0));
 
      // Right face (x = 1.0f)
  verts.push_back(Vec3f(1.0,  1.0, -1.0));
  verts.push_back(Vec3f(1.0,  1.0,  1.0));
  verts.push_back(Vec3f(1.0, -1.0,  1.0));
  verts.push_back(Vec3f(1.0, -1.0, -1.0));

  //subdivide(verts);
  //verts.clear();
  amountofQuads = verts.size();
  verts = scale(verts, 0.25);
  /*for(int i = 0; i< ndrawsubVerts.size(); i++){
	verts.push_back(ndrawsubVerts[i]);
	}*/

  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * verts.size(), // byte size of Vec3f, 4 of them
               verts.data(),      // pointer (Vec3f*) to contents of verts
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}

void subdivide(std::vector<Vec3f> vecs){
	subVerts.clear();
      drawsubVerts.clear();
	ndrawsubVerts.clear();
	//printf("vec size: %i\n",vecs.size());

	//inserting a new point into vecs at the midpoint calling it subVerts
	for(int i = 0; i<vecs.size(); i+=2){
		Vec3f p1;
		Vec3f p2;
		if(i == vecs.size()-1){
			p1 = vecs[i];
			p2 = vecs[0];
		}
		else{
			p1 = vecs[i];
			p2 = vecs[i+1];
		}
		
		/*printf("p1: %f\n",p1.z());
		printf("p2: %f\n",p2.z());	
		float z = (p1.z() + p2.z())/2;	
		printf("newpoint: %f\n",z);*/
	
		Vec3f newPoint{((p1.x() + p2.x())/2),((p1.y() + p2.y())/2),((p1.z() + 
		p2.z())/2)};
		//printf("newpoint x: %f\n",newPoint.x());
		//printf("newpoint y: %f\n",newPoint.y());
		subVerts.push_back(p1);		
		subVerts.push_back(newPoint);
		subVerts.push_back(newPoint);
		subVerts.push_back(p2);
	}
		//getting the midpoints of this new graph thingy calling it drawsubVerts
	for(int i = 0; i<subVerts.size(); i+=2){
		Vec3f p1;
		Vec3f p2;
		if(i == subVerts.size()-1){
			p1 = subVerts[i];
			p2 = subVerts[0];
		}
		else{
			p1 = subVerts[i];
			p2 = subVerts[i+1];
		}
	
		Vec3f newPoint{((p1.x() + p2.x())/2),((p1.y() + p2.y())/2),((p1.z() + 
		p2.z())/2)};
		//printf("newpoint x: %f ",newPoint.x());
		//printf("newpoint y: %f\n",newPoint.y());
		drawsubVerts.push_back(newPoint);
	}
	// making drawsubVerts into a bunch of lines instead of points
	for(int l =0; l<drawsubVerts.size(); l++){
		if(l == drawsubVerts.size()-1){
			ndrawsubVerts.push_back(drawsubVerts[l]);
			ndrawsubVerts.push_back(drawsubVerts[0]);
		}
		else{
			ndrawsubVerts.push_back(drawsubVerts[l]);
			ndrawsubVerts.push_back(drawsubVerts[l+1]);
		}
	}
		

}

std::vector<Vec3f> scale(std::vector<Vec3f> vectors, float scaler){
	for(int i =0; i < vectors.size(); i++){
		//printf("x before scale: %f",vectors[i].x());
		vectors[i].x() = vectors[i].x()*scaler;
		//printf("x afterscale: %f\n",vectors[i].x());
		vectors[i].y() = vectors[i].y()*scaler;
		vectors[i].z() = vectors[i].z()*scaler;
	}
	return vectors;
}

void loadLineGeometryToGPU() {
  // Just basic layout of floats, for a quad
  // 3 floats per vertex, 4 vertices

  // loading a track
  std::vector<Vec3f> vectors;
  loadVec3fFromFile(vectors, file);
  std::vector<Vec3f> verts;
  amountofLines = 1;
  printf("before calling sub size: %i\n",vectors.size());

  vectors = scale(vectors, 0.25);
  subdivide(vectors);
  
  //sub dividing n times
  for(int i =0; i<10; i++){
	  subdivide(ndrawsubVerts);
	}


  printf("after calling sub size: %i\n",ndrawsubVerts.size());
	for(int i =0; i<ndrawsubVerts.size(); i++){
		verts.push_back(ndrawsubVerts[i]);
		amountofLines++;
	}

  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(Vec3f) * amountofLines*2, // byte size of Vec3f, 4 of them
               verts.data(),      // pointer (Vec3f*) to contents of verts
               GL_STATIC_DRAW);   // Usage pattern of GPU buffer
}

void setupVAO() {
  glBindVertexArray(vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );

  glBindVertexArray(line_vaoID);

  glEnableVertexAttribArray(0); // match layout # in shader
  glBindBuffer(GL_ARRAY_BUFFER, line_vertBufferID);
  glVertexAttribPointer(0,        // attribute layout # above
                        3,        // # of components (ie XYZ )
                        GL_FLOAT, // type of components
                        GL_FALSE, // need to be normalized?
                        0,        // stride
                        (void *)0 // array buffer offset
                        );

  glBindVertexArray(0); // reset to default
}

void reloadProjectionMatrix() {
  // Perspective Only

  // field of view angle 60 degrees
  // window aspect ratio
  // near Z plane > 0
  // far Z plane

  P = PerspectiveProjection(WIN_FOV, // FOV
                            static_cast<float>(WIN_WIDTH) /
                                WIN_HEIGHT, // Aspect
                            WIN_NEAR,       // near plane
                            WIN_FAR);       // far plane depth
}

void loadModelViewMatrix() {
  M = IdentityMatrix();
  line_M = IdentityMatrix();
  // view doesn't change, but if it did you would use this
  V = camera.lookatMatrix();
}

void reloadViewMatrix() { V = camera.lookatMatrix(); }

void setupModelViewProjectionTransform() {
  MVP = P * V * M; // transforms vertices from right to left (odd huh?)
}

void reloadMVPUniform() {
  GLint id = glGetUniformLocation(basicProgramID, "MVP");

  glUseProgram(basicProgramID);
  glUniformMatrix4fv(id,        // ID
                     1,         // only 1 matrix
                     GL_TRUE,   // transpose matrix, Mat4f is row major
                     MVP.data() // pointer to data in Mat4f
                     );
}

void reloadColorUniform(float r, float g, float b) {
  GLint id = glGetUniformLocation(basicProgramID, "inputColor");

  glUseProgram(basicProgramID);
  glUniform3f(id, // ID in basic_vs.glsl
              r, g, b);
}

void generateIDs() {
  // shader ID from OpenGL
  std::string vsSource = loadShaderStringfromFile("./shaders/basic_vs.glsl");
  std::string fsSource = loadShaderStringfromFile("./shaders/basic_fs.glsl");
  basicProgramID = CreateShaderProgram(vsSource, fsSource);

  // VAO and buffer IDs given from OpenGL
  glGenVertexArrays(1, &vaoID);
  glGenBuffers(1, &vertBufferID);
  glGenVertexArrays(1, &line_vaoID);
  glGenBuffers(1, &line_vertBufferID);
}

void deleteIDs() {
  glDeleteProgram(basicProgramID);

  glDeleteVertexArrays(1, &vaoID);
  glDeleteBuffers(1, &vertBufferID);
  glDeleteVertexArrays(1, &line_vaoID);
  glDeleteBuffers(1, &line_vertBufferID);
}

void init() {
  glEnable(GL_DEPTH_TEST);
  glPointSize(50);

  camera = Camera(Vec3f{0, 0, 5}, Vec3f{0, 0, -1}, Vec3f{0, 1, 0});

  // SETUP SHADERS, BUFFERS, VAOs

  generateIDs();
  setupVAO();
  loadQuadGeometryToGPU();
  loadLineGeometryToGPU();

  loadModelViewMatrix();
  reloadProjectionMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

int main(int argc, char **argv) {
  GLFWwindow *window;

  file = argv[1];
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window =
      glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Project", NULL, NULL);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glfwSetWindowSizeCallback(window, windowSetSizeFunc);
  glfwSetFramebufferSizeCallback(window, windowSetFramebufferSizeFunc);
  glfwSetKeyCallback(window, windowKeyFunc);
  glfwSetCursorPosCallback(window, windowMouseMotionFunc);
  glfwSetMouseButtonCallback(window, windowMouseButtonFunc);

  glfwGetFramebufferSize(window, &WIN_WIDTH, &WIN_HEIGHT);

  // Initialize glad
  if (!gladLoadGL()) {
    std::cerr << "Failed to initialise GLAD" << std::endl;
    return -1;
  }

  std::cout << "GL Version: :" << glGetString(GL_VERSION) << std::endl;
  std::cout << GL_ERROR() << std::endl;

  init(); // our own initialize stuff func

  float t = 0;
  float dt = 0.01;

  while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
         !glfwWindowShouldClose(window)) {

    if (g_play) {
      t += dt;
      animateQuad(t);
    }

    displayFunc();
    moveCamera();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // clean up after loop
  deleteIDs();

  return 0;
}

//==================== CALLBACK FUNCTIONS ====================//

void windowSetSizeFunc(GLFWwindow *window, int width, int height) {
  WIN_WIDTH = width;
  WIN_HEIGHT = height;

  reloadProjectionMatrix();
  setupModelViewProjectionTransform();
  reloadMVPUniform();
}

void windowSetFramebufferSizeFunc(GLFWwindow *window, int width, int height) {
  FB_WIDTH = width;
  FB_HEIGHT = height;

  glViewport(0, 0, FB_WIDTH, FB_HEIGHT);
}

void windowMouseButtonFunc(GLFWwindow *window, int button, int action,
                           int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      g_cursorLocked = GL_TRUE;
    } else {
      g_cursorLocked = GL_FALSE;
    }
  }
}

void windowMouseMotionFunc(GLFWwindow *window, double x, double y) {
  if (g_cursorLocked) {
    float deltaX = (x - g_cursorX) * 0.01;
    float deltaY = (y - g_cursorY) * 0.01;
    camera.rotateAroundFocus(deltaX, deltaY);

    reloadViewMatrix();
    setupModelViewProjectionTransform();
    reloadMVPUniform();
  }

  g_cursorX = x;
  g_cursorY = y;
}

void windowKeyFunc(GLFWwindow *window, int key, int scancode, int action,
                   int mods) {
  bool set = action != GLFW_RELEASE && GLFW_REPEAT;
  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, GL_TRUE);
    break;
  case GLFW_KEY_W:
    g_moveBackForward = set ? 1 : 0;
    break;
  case GLFW_KEY_S:
    g_moveBackForward = set ? -1 : 0;
    break;
  case GLFW_KEY_A:
    g_moveLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_D:
    g_moveLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_Q:
    g_moveUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_E:
    g_moveUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_UP:
    g_rotateUpDown = set ? -1 : 0;
    break;
  case GLFW_KEY_DOWN:
    g_rotateUpDown = set ? 1 : 0;
    break;
  case GLFW_KEY_LEFT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? -1 : 0;
    else
      g_rotateLeftRight = set ? 1 : 0;
    break;
  case GLFW_KEY_RIGHT:
    if (mods == GLFW_MOD_SHIFT)
      g_rotateRoll = set ? 1 : 0;
    else
      g_rotateLeftRight = set ? -1 : 0;
    break;
  case GLFW_KEY_SPACE:
    g_play = set ? !g_play : g_play;
    break;
  case GLFW_KEY_LEFT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 0.5;
    } else {
      g_panningSpeed *= 0.5;
    }
    break;
  case GLFW_KEY_RIGHT_BRACKET:
    if (mods == GLFW_MOD_SHIFT) {
      g_rotationSpeed *= 1.5;
    } else {
      g_panningSpeed *= 1.5;
    }
    break;
  default:
    break;
  }
}

//==================== OPENGL HELPER FUNCTIONS ====================//

void moveCamera() {
  Vec3f dir;

  if (g_moveBackForward) {
    dir += Vec3f(0, 0, g_moveBackForward * g_panningSpeed);
  }
  if (g_moveLeftRight) {
    dir += Vec3f(g_moveLeftRight * g_panningSpeed, 0, 0);
  }
  if (g_moveUpDown) {
    dir += Vec3f(0, g_moveUpDown * g_panningSpeed, 0);
  }

  if (g_rotateUpDown) {
    camera.rotateUpDown(g_rotateUpDown * g_rotationSpeed);
  }
  if (g_rotateLeftRight) {
    camera.rotateLeftRight(g_rotateLeftRight * g_rotationSpeed);
  }
  if (g_rotateRoll) {
    camera.rotateRoll(g_rotateRoll * g_rotationSpeed);
  }

  if (g_moveUpDown || g_moveLeftRight || g_moveBackForward ||
      g_rotateLeftRight || g_rotateUpDown || g_rotateRoll) {
    camera.move(dir);
    reloadViewMatrix();
    setupModelViewProjectionTransform();
    reloadMVPUniform();
  }
}

std::string GL_ERROR() {
  GLenum code = glGetError();

  switch (code) {
  case GL_NO_ERROR:
    return "GL_NO_ERROR";
  case GL_INVALID_ENUM:
    return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:
    return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:
    return "GL_INVALID_OPERATION";
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_OUT_OF_MEMORY:
    return "GL_OUT_OF_MEMORY";
  default:
    return "Non Valid Error Code";
  }
}
