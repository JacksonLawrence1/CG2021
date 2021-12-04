#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <Colour.h>
#include <TextureMap.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>

#include <camera.h>
#include <interpolate.h>
#include <lighting.h>
#include <rasterize.h>
#include <raytrace.h>
#include <readFile.h>
#include <wireframe.h>

using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

TextureMap texture = TextureMap("texture.ppm");
vec3 cameraPos(0.0, -0.25, 4.0);
float focalLength = 2;
int renderMode = 2;
int lightingMode = 5;
float scaleFactor = 900;
glm::mat3 cameraOrientation(1.0, 0.0, 0.0,
							0.0, 1.0, 0.0,
							0.0, 0.0, 1.0);
bool orbit = false;
vec3 light(0.0, 0.2, 1.0);
//vec3 light(0.0, 0.4, 0.2);

vector<Colour> c = unloadMaterialFile("cornell-box.mtl");
vector<ModelTriangle> triangles = unloadobjFile("cornell-box.obj", 0.17, c);
vector<ModelTriangle> sphere = unloadSphere("sphere.obj", 0.17, Colour(255, 0, 0));


// draws relevant items on screen
void draw(DrawingWindow& window) {
	window.clearPixels();

	if (orbit) {
		cameraPos = cameraPos * rotateMatrixY(0.05);
		cameraOrientation = lookat(cameraPos);
	}

	if (renderMode == 0) renderWireFrame(window, sphere, cameraPos, focalLength, scaleFactor, cameraOrientation);
	if (renderMode == 1) renderRasterizedScene(window, sphere, cameraPos, focalLength, scaleFactor, cameraOrientation);
	//if (renderMode == 2) renderRayTracedScene(window, triangles, cameraPos, cameraOrientation, light, lightingMode, focalLength, scaleFactor);

	// temp sphere render
	if (renderMode == 2) renderRayTracedScene(window, sphere, cameraPos, cameraOrientation, light, lightingMode, focalLength, scaleFactor);

}

// handles keypresses to do certain events
void handleEvent(SDL_Event event, DrawingWindow& window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) cameraOrientation = cameraOrientation * rotateMatrixY(0.05);
		else if (event.key.keysym.sym == SDLK_RIGHT) cameraOrientation = cameraOrientation * rotateMatrixY(-0.05);
		else if (event.key.keysym.sym == SDLK_UP) cameraOrientation = cameraOrientation * rotateMatrixX(0.05);
		else if (event.key.keysym.sym == SDLK_DOWN) cameraOrientation = cameraOrientation * rotateMatrixX(-0.05);

		else if (event.key.keysym.sym == SDLK_PAGEUP) light.y = light.y + 0.05;
		else if (event.key.keysym.sym == SDLK_PAGEDOWN) light.y = light.y - 0.05;

		else if (event.key.keysym.sym == SDLK_1) renderMode = 0; // wireframe model
		else if (event.key.keysym.sym == SDLK_2) renderMode = 1; // rasterized model
		else if (event.key.keysym.sym == SDLK_3) renderMode = 2; // ray traced model

		else if (event.key.keysym.sym == SDLK_z) lightingMode = 0; // no lighting effects
		else if (event.key.keysym.sym == SDLK_x) lightingMode = 1; // creates hard shadows on models
		else if (event.key.keysym.sym == SDLK_c) lightingMode = 2; // proximity lighting on model
		else if (event.key.keysym.sym == SDLK_v) lightingMode = 3;
		else if (event.key.keysym.sym == SDLK_b) lightingMode = 4;
		else if (event.key.keysym.sym == SDLK_n) lightingMode = 5;

		else if (event.key.keysym.sym == SDLK_w) cameraPos[1] -= 0.05; // move model up
		else if (event.key.keysym.sym == SDLK_a) cameraPos[0] += 0.05; // move model left
		else if (event.key.keysym.sym == SDLK_s) cameraPos[1] += 0.05; // move model down
		else if (event.key.keysym.sym == SDLK_d) cameraPos[0] -= 0.05; // move model right

		else if (event.key.keysym.sym == SDLK_EQUALS) cameraPos[2] -= 0.05; // zoom in 
		else if (event.key.keysym.sym == SDLK_MINUS) cameraPos[2] += 0.05; // zoom out

		else if (event.key.keysym.sym == SDLK_o) orbit = !orbit; // orbit the model

		else if (event.key.keysym.sym == SDLK_l) cameraOrientation = lookat(cameraPos); // if model out of view camera looks at model

		else if (event.key.keysym.sym == SDLK_u) randomStrokedTriangle(window); // draws a random unfilled triangle on screen

		else if (event.key.keysym.sym == SDLK_j) randomFilledTriangle(window); // draws a random filled triangle on screen

	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char* mrgv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;


	//renderRayTracedScene(window, sphere, cameraPos, cameraOrientation, light, lightingMode, focalLength, scaleFactor);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);

		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
