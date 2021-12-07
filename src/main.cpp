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

#include <glm/gtx/string_cast.hpp>

using namespace std;
using namespace glm;

#define WIDTH 640
#define HEIGHT 480

//vec3 cameraPos(0.0, 0.0, 4.0);

vec3 cameraPos(0, 0, 4.0);

float focalLength = 2;
int renderMode = 2;
int lightingMode = 3;
float scaleFactor = 500;
mat3 cameraOrientation(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

bool orbit = true;
vec3 light(0.0, 0.0, 1.0);
//vec3 light(0.0, 0.4, 0.2);

vector<Colour> c = unloadMaterialFile("new-cornell-box.mtl");
vector<ModelTriangle> triangles = unloadNewFile("new-cornell-box.obj", 0.17, c);
//vector<ModelTriangle> sphere = unloadNewFile("sphere.obj", 0.17, c);

// draws relevant items on screen
void draw(DrawingWindow& window) {
	window.clearPixels();

	if (orbit) {
		cameraPos = cameraPos * rotateMatrixY(0.05);
		cameraOrientation = lookat(cameraPos);
		cout << "CameraPos: " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << endl;
		cout << "CameraOrientation" << glm::to_string(cameraOrientation) << endl;
	}

	if (renderMode == 0) renderWireFrame(window, triangles, cameraPos, focalLength, scaleFactor, cameraOrientation);
	if (renderMode == 1) renderRasterizedScene(window, triangles, cameraPos, focalLength, scaleFactor, cameraOrientation);
	if (renderMode == 2) renderRayTracedScene(window, triangles, cameraPos, cameraOrientation, light, lightingMode, focalLength, scaleFactor);

}

// handles keypresses to do certain events
void handleEvent(SDL_Event event, DrawingWindow& window) {
	if (event.type == SDL_KEYDOWN) {
		cout << "CameraPos: " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << endl;
		cout << "CameraOrientation" << glm::to_string(cameraOrientation) << endl;


		if (event.key.keysym.sym == SDLK_LEFT) cameraOrientation = cameraOrientation * rotateMatrixY(0.05);
		else if (event.key.keysym.sym == SDLK_RIGHT) cameraOrientation = cameraOrientation * rotateMatrixY(-0.05);
		else if (event.key.keysym.sym == SDLK_UP) cameraOrientation = cameraOrientation * rotateMatrixX(-0.05);
		else if (event.key.keysym.sym == SDLK_DOWN) cameraOrientation = cameraOrientation * rotateMatrixX(0.05);

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

	// raytraced render
	cameraOrientation = mat3(0.581683, 0.0, -0.813416, -0.0, 1.0, 0.0, 0.813416, 0.0, 0.581683);
	cameraPos = vec3(-3.25366, 0, 2.32673);
	int counter = 0;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		while (cameraPos.x < 3.26) {
			draw(window);
			string name = string("images/i") + to_string(counter) + ".bmp";
			window.saveBMP(name);
			counter += 1;
		}

		window.renderFrame();
	}
}
