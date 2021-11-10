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

using namespace std;

#define WIDTH 320
#define HEIGHT 240

TextureMap texture = TextureMap("texture.ppm");
glm::vec3 cameraPos(0.0, 0.0, 4.0);
float focalLength = 2;
glm::mat3 cameraOrientation(1.0, 0.0, 0.0,
							0.0, 1.0, 0.0,
							0.0, 0.0, 1.0);
bool orbit = false;
vector<vector<float>> depthBuffer(WIDTH, std::vector<float>(HEIGHT, 0));

// Generates a triangle with random vertices in the form CanvasTriangle
CanvasTriangle generateRandomTriangle() {
	std::vector<float> widths;  // avoids casting when initializing structures
	std::vector<float> heights;
	for (int i = 0; i < 3; i++) {
		widths.push_back(rand() % WIDTH);
		heights.push_back(rand() % HEIGHT);
	}
	CanvasTriangle triangle{ CanvasPoint{widths[0], heights[0]},
							 CanvasPoint{widths[1], heights[1]},
							 CanvasPoint{widths[2], heights[2]} };

	return triangle;
}

glm::mat3 rotateMatrixX(float angle) {
	glm::mat3 rotation(1.0, 0.0, 0.0,
		0.0, cos(angle), sin(angle),
		0.0, -sin(angle), cos(angle));
	return rotation;
}

glm::mat3 rotateMatrixY(float angle) {
	glm::mat3 rotation(cos(angle), 0.0, sin(angle),
		0.0, 1.0, 0,
		-sin(angle), 0.0, cos(angle));
	return rotation;
}

// Convert a colour to uint32_t
uint32_t convertColour(Colour colour) {
	uint32_t c = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
	return c;
}

// (1, 1) and (3, 3) step 3 = (1,1),(2,2),(3,3)
std::vector<float> interpolateSingleFloats(float from, float to, int numberofValues) {
	std::vector<float> v;
	float difference = (to - from) / (numberofValues);
	for (int i = 0; i < numberofValues; i++) {
		v.push_back(from + (i * difference));
	}
	return v;
}

// same as above function but evaluates 3 values instead of 2, e.g. (1,1,1) to (3,3,3) 
std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberofValues) {
	std::vector<std::vector<float>> v;
	for (int i = 0; i < 3; i++) {
		v.push_back(interpolateSingleFloats(from[i], to[i], numberofValues));
	}
	std::vector<glm::vec3> finalVec;
	for (int j = 0; j < numberofValues; j++) {
		glm::vec3 newVec;
		newVec[0] = v[0][j];
		newVec[1] = v[1][j];
		newVec[2] = v[2][j];
		finalVec.push_back(newVec);
	}
	return finalVec;
}

// gets appropriate window coords for the line specified but will divide into specified chunks
std::vector<std::vector<float>> interpolateLineSteps(CanvasPoint from, CanvasPoint to, int steps) {
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	std::vector<float> xList = interpolateSingleFloats(from.x, to.x, steps);
	std::vector<float> yList = interpolateSingleFloats(from.y, to.y, steps);
	std::vector<float> depthList = interpolateSingleFloats(from.depth, to.depth, steps);
	std::vector<std::vector<float>> v;

	int xRound = 0;
	int yRound = 0;
	int depthRound = 0;
	for (int i = 0; i < steps; i++) {
		v.push_back({ xList[i], yList[i], depthList[i] });
	}

	return v;
}

// Finds equivalent vertex point on window 
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength, float scale) {
	glm::vec3 correctedVertices = vertexPosition - cameraPosition;
	correctedVertices = correctedVertices * cameraOrientation;
	float u = (focalLength * (correctedVertices[0]*-scale / correctedVertices[2]*scale)) + (WIDTH / 2);
	float v = (focalLength * (correctedVertices[1]*scale / correctedVertices[2]*scale)) + (HEIGHT / 2);
	return CanvasPoint(round(u), round(v), correctedVertices[2]);
}

// designates maximum and minimum x values spanning all y values in a triangle
// Inputs MUST be sorted in order of ascending y values
std::vector<std::vector<float>> getxRanges(CanvasPoint p0, CanvasPoint p1, CanvasPoint p2, CanvasPoint p3) {
	std::vector<float> xMins;
	std::vector<float> xMaxs;
	std::vector<float> xDepthMin;
	std::vector<float> xDepthMax;

	std::vector<std::vector<float>> top_to_left_edge = interpolateLineSteps(p0, p1, p2.y - p0.y);
	std::vector<std::vector<float>> top_to_right_edge = interpolateLineSteps(p0, p2, p2.y - p0.y);

	// Gets x value ranges for 'top' portion of triangle
	for (int i = p0.y; i < p2.y; i++) {
		xMins.push_back(top_to_left_edge[round(i - p0.y)][0]);
		xMaxs.push_back(top_to_right_edge[round(i - p0.y)][0]);
		xDepthMin.push_back(top_to_left_edge[round(i - p0.y)][2]);
		xDepthMax.push_back(top_to_right_edge[round(i - p0.y)][2]);
	}

	std::vector<std::vector<float>> left_to_bot = interpolateLineSteps(p1, p3, p3.y - p2.y);
	std::vector<std::vector<float>> right_to_bot = interpolateLineSteps(p2, p3, p3.y - p2.y);

	// Gets x value ranges for 'bottom' portion of triangle
	for (int i = p2.y; i < p3.y; i++) {
		xMins.push_back(left_to_bot[round(i - p2.y)][0]);
		xMaxs.push_back(right_to_bot[round(i - p2.y)][0]);
		xDepthMin.push_back(left_to_bot[round(i - p2.y)][2]);
		xDepthMax.push_back(right_to_bot[round(i - p2.y)][2]);
	}
	std::vector<std::vector<float>> allXs;

	for (int i = 0; i < xMaxs.size(); i++) {
		allXs.push_back({ xMins[i], xMaxs[i], xDepthMin[i], xDepthMax[i] });
	}

	return allXs;
}

// Gets min and max coordinates of texture, for equivalent y coordinates in triangle
std::vector<std::vector<std::vector<float>>> getInterpolatedRanges(std::vector<CanvasPoint> sorted) {
	std::vector<std::vector<float>> leftCoordinate;
	std::vector<std::vector<float>> rightCoordinate;

	CanvasPoint t0(sorted[0].texturePoint.x, sorted[0].texturePoint.y);
	CanvasPoint t1(sorted[1].texturePoint.x, sorted[1].texturePoint.y);
	CanvasPoint t2(sorted[2].texturePoint.x, sorted[2].texturePoint.y);
	CanvasPoint t3(sorted[3].texturePoint.x, sorted[3].texturePoint.y);

	std::vector<std::vector<float>> top_to_left_tex = interpolateLineSteps(t0, t1, sorted[1].y - sorted[0].y);
	std::vector<std::vector<float>> top_to_right_tex = interpolateLineSteps(t0, t2, sorted[1].y - sorted[0].y);

	// Gets x value ranges for 'top' portion of triangle
	for (int i = 0; i < sorted[1].y - sorted[0].y; i++) {
		leftCoordinate.push_back({ top_to_left_tex[i][0], top_to_left_tex[i][1] });
		rightCoordinate.push_back({ top_to_right_tex[i][0], top_to_right_tex[i][1] });
	}

	std::vector<std::vector<float>> left_to_bot_tex = interpolateLineSteps(t1, t3, sorted[3].y - sorted[1].y);
	std::vector<std::vector<float>> right_to_bot_tex = interpolateLineSteps(t2, t3, sorted[3].y - sorted[1].y);

	// Gets x value ranges for 'bottom' portion of triangle
	for (int i = 0; i < sorted[3].y - sorted[1].y; i++) {
		leftCoordinate.push_back({ left_to_bot_tex[i][0], left_to_bot_tex[i][1] });
		rightCoordinate.push_back({ right_to_bot_tex[i][0], right_to_bot_tex[i][1] });
	}
	std::vector<std::vector<std::vector<float>>> allXs;

	for (int i = 0; i < rightCoordinate.size(); i++) {
		allXs.push_back({ leftCoordinate[i], rightCoordinate[i] });
	}

	return allXs;
}

// Sorts points according to y value (smallest to largest)
// also creates 4th point which is opposite the 'middle' point
std::vector<CanvasPoint> sortPoints(CanvasPoint v0, CanvasPoint v1, CanvasPoint v2) {
	std::vector<CanvasPoint> sorted = { {v0.x, v0.y, v0.depth}, // top point
										{v1.x, v1.y, v1.depth}, // middle point
										{v2.x, v2.y, v2.depth}, // bottom point
										{0, 0} };	  // opposite of middle point
	sorted[0].texturePoint = v0.texturePoint;
	sorted[1].texturePoint = v1.texturePoint;
	sorted[2].texturePoint = v2.texturePoint;
	// order by y coordinates
	if (sorted[1].y < sorted[0].y) { std::swap(sorted[0], sorted[1]); }
	if (sorted[2].y < sorted[1].y) { std::swap(sorted[1], sorted[2]); }
	if (sorted[1].y < sorted[0].y) { std::swap(sorted[0], sorted[1]); }

	// sets middle point's opposite y value to be the same
	sorted[3].y = sorted[1].y;
	sorted[3].x = sorted[2].x;
	sorted[3].depth = sorted[1].depth;
	sorted[3].texturePoint = TexturePoint(-1, -1);

	std::vector<std::vector<float>> longEdge = interpolateLineSteps(sorted[0], sorted[2], sorted[2].y - sorted[0].y);

	// Gets vertex opposite of middle corner
	for (int i = 0; i < longEdge.size(); i++) {
		if (longEdge[i][1] == sorted[1].y) { 
			sorted[3].x = longEdge[i][0];
			sorted[3].depth = longEdge[i][2];
		}
	}

	// Leftmost point deemed by sorted[1], rightmost deemed by sorted[2]
	if (sorted[1].x > sorted[3].x) {
		std::swap(sorted[1], sorted[3]);
		std::swap(sorted[2], sorted[3]);
	}
	else { std::swap(sorted[2], sorted[3]); }

	return sorted;
}

// Converts a texture point purely into a canvas point just by exchanging x and y values
CanvasPoint convertTexturePoint(TexturePoint texturePoint) {
	CanvasPoint canvas{ texturePoint.x, texturePoint.y };
	return canvas;
}

// finds equivalent texture midpoint triangle on texture
std::vector<CanvasPoint> findTexturemid(std::vector<CanvasPoint> sorted) {
	bool swap = false;
	if ((sorted[1].texturePoint.x == -1) && (sorted[1].texturePoint.y == -1)) {
		std::swap(sorted[1], sorted[2]);
		swap = true;
	}
	std::vector<CanvasPoint> sorted_textures = sortPoints(convertTexturePoint(sorted[0].texturePoint), 
		convertTexturePoint(sorted[1].texturePoint), convertTexturePoint(sorted[3].texturePoint));
	if ((sorted_textures[2].texturePoint.x == -1) && (sorted_textures[2].texturePoint.y == -1)) {
		std::swap(sorted_textures[1], sorted_textures[2]);
	}

	float height_tex = abs(sorted_textures[3].y - sorted_textures[0].y);
	float height = abs(sorted[3].y - sorted[0].y);
	float width_tex = abs(sorted_textures[0].x - sorted_textures[3].x);
	float width = abs(sorted[3].x - sorted[0].x);
	float yfactor = height_tex / height;
	float xfactor = width_tex / width;

	sorted[2].texturePoint.x = round(sorted[2].x * xfactor);
	sorted[2].texturePoint.y = round(sorted[2].y * yfactor);
	if (swap) { std::swap(sorted[1], sorted[2]); }

	return sorted;
}

// Given texture map, puts into format where each row corresponds to y value pixels
std::vector<std::vector<uint32_t>> unloadTexture(TextureMap texture) {
	std::vector<std::vector<uint32_t>> sortedTexture;
	std::vector<uint32_t> currentRow;
	for (int i = 0; i < texture.height; i++) {
		for (int j = 0; j < texture.width; j++) {
			currentRow.push_back(texture.pixels[j + (texture.width * i)]);
		}
		sortedTexture.push_back(currentRow);
		currentRow.clear();
	}
	return sortedTexture;
}

std::vector<uint32_t> getColourMap(std::vector<float> t0, std::vector<float> t1, int steps, std::vector<std::vector<uint32_t>> sortedTexture) {
	CanvasPoint tc0(t0[0], t0[1]);
	CanvasPoint tc1(t1[0], t1[1]);

	std::vector<std::vector<float>> point_to_point = interpolateLineSteps(tc0, tc1, steps);
	std::vector<uint32_t> colourMap;
	for (int i = 0; i < point_to_point.size(); i++) {
		colourMap.push_back(sortedTexture[round(point_to_point[i][1])][round(point_to_point[i][0])]);
	}
	return colourMap;
}

// Given a colour name will return the equivalent Colour class, white (with name Unknown) if not found
Colour getColour(string colour, vector<Colour> colourVec) {
	for (int i = 0; i < colourVec.size(); i++) {
		if (colourVec[i].name == colour) { return colourVec[i]; }
	}
	return Colour("Unknown", 0, 0, 0);
}

// Unloads a .obj file and stores the models in a vector
vector<ModelTriangle> unloadobjFile(string fileName, float scalingFactor, vector<Colour> colourVec) {
	ifstream file(fileName);
	string line;
	vector<ModelTriangle> v;
	vector<glm::vec3> currentVectors;
	Colour colour;
	while (!file.eof()) {
		getline(file, line);
		vector<string> currentLine = split(line, ' ');
		if (line[0] == 'u') {
			colour = getColour(currentLine[1], colourVec);
		}
		else if (line[0] == 'v') {
			glm::vec3 lineVector(stof(currentLine[1]) * scalingFactor, stof(currentLine[2]) * scalingFactor, stof(currentLine[3]) * scalingFactor);
			currentVectors.push_back(lineVector);
		}
		else if (line[0] == 'f') {
			glm::vec3 lineVector(stof(currentLine[1]), stof(currentLine[2]), stof(currentLine[3]));
			v.push_back(ModelTriangle(currentVectors[lineVector[0] - 1], currentVectors[lineVector[1] - 1], currentVectors[lineVector[2] - 1], colour));
		}
	}
	return v;
}

// Unloads a .obj file and stores the models in a vector
vector<glm::vec3> unloadobjVertices(string fileName, float scalingFactor) {
	ifstream file(fileName);
	string line;
	vector<glm::vec3> currentVectors;
	while (!file.eof()) {
		getline(file, line);
		vector<string> currentLine = split(line, ' ');
		if (line[0] == 'v') {
			glm::vec3 lineVector(stof(currentLine[1]) * scalingFactor, stof(currentLine[2]) * scalingFactor, stof(currentLine[3]) * scalingFactor);
			currentVectors.push_back(lineVector);
		}
	}
	return currentVectors;
}

// Unloads a .mtl file and stores the relevant colours in a vector
vector<Colour> unloadMaterialFile(string fileName) {
	ifstream file(fileName);
	string line;
	vector<Colour> c;
	string name;
	while (!file.eof()) {
		getline(file, line);
		if (line[0] == 'n') {
			vector<string> currentLine = split(line, ' ');
			name = currentLine[1];
		}
		else if (line[0] == 'K') {
			vector<string> currentLine = split(line, ' ');
			c.push_back(Colour(name, round(stof(currentLine[1]) * 255), round(stof(currentLine[2]) * 255), round(stof(currentLine[3]) * 255)));
		}
	}
	return c;
}

// Draws a line from point a to b
void drawLine(DrawingWindow& window, CanvasPoint to, CanvasPoint from, uint32_t colour) {
	float steps = std::max(abs(to.x - from.x), abs(to.x - from.x));
	float xSteps = to.x - from.x / steps;
	float ySteps = to.x - from.x / steps;
	for (float i = 0.0; i < steps; i++) {
		float x = from.x + (xSteps * i);
		float y = from.y + (ySteps * i);
		window.setPixelColour(round(x), round(y), colour);
	}
}

// Draws colored line given a pixel for each step of the line
void drawLineColoured(DrawingWindow& window, CanvasPoint from, CanvasPoint to, std::vector<uint32_t> colours, int steps) {
	std::vector<std::vector<float>> coords = interpolateLineSteps(from, to, steps);
	std::vector<float> colourStep = interpolateSingleFloats(0, colours.size(), coords.size());
	for (int i = 0; i < coords.size(); i++) {
		int index = round(colourStep[i]);
		if (index < colours.size()) { window.setPixelColour(round(coords[i][0]), round(coords[i][1]), colours[index]); }
	}
}

// Draws a filled triangle using rasterization
void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	// Creates a template structure for storing all points which will be sorted
	std::vector<CanvasPoint> sorted = sortPoints(triangle.v0(), triangle.v1(), triangle.v2());
	std::vector<std::vector<float>> xRange = getxRanges(sorted[0], sorted[1], sorted[2], sorted[3]);
	uint32_t newColour = convertColour(colour);

	// Draws triangle by iterating through y and then only drawing if x values are in particular range given the y (marked)
	for (int y = sorted[0].y; y < sorted[3].y; y++) {
		for (int x = round(xRange[y - sorted[0].y][0]); x <= round(xRange[y - sorted[0].y][1]); x++) {
			window.setPixelColour(x, y, newColour);
		}
	}

	//Colour white{ 255, 255, 255 };
	//drawStrokedTriangle(window, triangle, white);
}

void drawFilledTriangle2(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	// Creates a template structure for storing all points which will be sorted
	std::vector<CanvasPoint> sorted = sortPoints(triangle.v0(), triangle.v1(), triangle.v2());
	std::vector<std::vector<float>> xRange = getxRanges(sorted[0], sorted[1], sorted[2], sorted[3]);

	// Draws triangle by iterating through y and then only drawing if x values are in particular range given the y (marked)
	for (int y = sorted[0].y; y < sorted[3].y; y++) {
		float first = xRange[y - sorted[0].y][0]; // beginning of x range
		float second = xRange[y - sorted[0].y][1]; // end of x range
		std::vector<float> depthPoints = interpolateSingleFloats(xRange[y - sorted[0].y][2], xRange[y - sorted[0].y][3], second - first+1);
		for (int x = first; x <= second; x++) {
			float s = 1 / abs(depthPoints[x - first]);
			if ((x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) && s > depthBuffer[x][y]) {
				window.setPixelColour(x, y, convertColour(colour));
				depthBuffer[x][y] = s;
			}
		}
	}
}

// Draws a triangle with only an outline
void drawStrokedTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	uint32_t newColour = convertColour(colour);
	drawLine(window, triangle.v0(), triangle.v1(), newColour);
	drawLine(window, triangle.v0(), triangle.v2(), newColour);
	drawLine(window, triangle.v1(), triangle.v2(), newColour);
}

// Draws a triangle filled in with a texture
void drawTexturedTriangle(DrawingWindow& window, CanvasTriangle triangle, TextureMap texture) {
	std::vector<std::vector<uint32_t>> sortedTexture = unloadTexture(texture);

	std::vector<CanvasPoint> sorted = sortPoints(triangle.v0(), triangle.v1(), triangle.v2());
	std::vector<std::vector<float>> xRange = getxRanges(sorted[0], sorted[1], sorted[2], sorted[3]);
	sorted = findTexturemid(sorted);
	std::vector<std::vector<std::vector<float>>> range = getInterpolatedRanges(sorted);

	// Draws triangle by iterating through y and then only drawing if x values are in particular range given the y (marked)
	for (int y = sorted[0].y; y < sorted[3].y; y++) {
		CanvasPoint p0(range[y - sorted[0].y][0][0], range[y - sorted[0].y][0][1]);
		CanvasPoint p1(range[y - sorted[0].y][1][0], range[y - sorted[0].y][1][1]);
		auto test0 = range[y - sorted[0].y][0];
		auto test1 = range[y - sorted[0].y][1];
		std::vector<uint32_t> currentLineColour = getColourMap(test0, test1, xRange[y - sorted[0].y][1] - xRange[y - sorted[0].y][0] +10, sortedTexture);
		for (int x = round(xRange[y - sorted[0].y][0]); x < round(xRange[y - sorted[0].y][1]); x++) {
			auto test3 = x - xRange[y - sorted[0].y][0];
			if (currentLineColour.size() != 0) { window.setPixelColour(x, y, currentLineColour[test3]); }
		}
	}
	Colour white{ 255, 255, 255 };
	drawStrokedTriangle(window, triangle, white);
}

// Generates a random triangle with just an outline
void randomStrokedTriangle(DrawingWindow& window) {
	drawStrokedTriangle(window, generateRandomTriangle(), Colour{ rand() % 256, rand() % 256, rand() % 256 });
}

void randomFilledTriangle(DrawingWindow& window) {
	drawFilledTriangle(window, generateRandomTriangle(), Colour{ rand() % 256, rand() % 256, rand() % 256 });
}

vector<Colour> c = unloadMaterialFile("cornell-box.mtl");
vector<ModelTriangle> triangles = unloadobjFile("cornell-box.obj", 0.17, c);

void render(DrawingWindow& window, vector<ModelTriangle> triangles, glm::vec3 cameraPosition, float focalLength) {
	for (int i = 0; i < triangles.size(); i++) {
		CanvasPoint pos0 = getCanvasIntersectionPoint(cameraPosition, triangles[i].vertices[0], focalLength, 20);
		CanvasPoint pos1 = getCanvasIntersectionPoint(cameraPosition, triangles[i].vertices[1], focalLength, 20);
		CanvasPoint pos2 = getCanvasIntersectionPoint(cameraPosition, triangles[i].vertices[2], focalLength, 20);
		drawFilledTriangle2(window, CanvasTriangle(pos0, pos1, pos2), triangles[i].colour);
	}
	depthBuffer = vector<vector<float>>(WIDTH, std::vector<float>(HEIGHT, 0));
}

glm::mat3 lookat(glm::vec3 cameraPos, glm::vec3 target = glm::vec3(0,0,0)) {
	glm::vec3 forward = glm::normalize(cameraPos - target);
	glm::vec3 right = (glm::cross(glm::vec3(0, 1, 0), forward));
	glm::vec3 up = (glm::cross(forward, right));
	glm::mat3 newOrientation(right, up, forward);
	return newOrientation;
}

void draw(DrawingWindow& window) {
	window.clearPixels();
	//vector<glm::vec3> vectors = unloadobjVertices("cornell-box.obj", 0.17);
	render(window, triangles, cameraPos, focalLength);
	if (orbit){
		cameraPos = cameraPos * rotateMatrixY(0.05);
		cameraOrientation = lookat(cameraPos);
	}
}

void handleEvent(SDL_Event event, DrawingWindow& window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) cameraOrientation = cameraOrientation * rotateMatrixY(0.05);
		else if (event.key.keysym.sym == SDLK_RIGHT) cameraOrientation = cameraOrientation * rotateMatrixY(-0.05);
		else if (event.key.keysym.sym == SDLK_UP) cameraOrientation = cameraOrientation * rotateMatrixX(0.05);
		else if (event.key.keysym.sym == SDLK_DOWN) cameraOrientation = cameraOrientation * rotateMatrixX(-0.05);

		else if (event.key.keysym.sym == SDLK_w) cameraPos[1] -= 0.05;
		else if (event.key.keysym.sym == SDLK_a) cameraPos[0] += 0.05;
		else if (event.key.keysym.sym == SDLK_s) cameraPos[1] += 0.05;
		else if (event.key.keysym.sym == SDLK_d) cameraPos[0] -= 0.05;

		else if (event.key.keysym.sym == SDLK_EQUALS) cameraPos[2] -= 0.05;
		else if (event.key.keysym.sym == SDLK_MINUS) cameraPos[2] += 0.05;

		else if (event.key.keysym.sym == SDLK_o) orbit = !orbit;

		else if (event.key.keysym.sym == SDLK_l) cameraOrientation = lookat(cameraPos);

		else if (event.key.keysym.sym == SDLK_u) randomStrokedTriangle(window);

		else if (event.key.keysym.sym == SDLK_j) randomFilledTriangle(window);

	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char* mrgv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
