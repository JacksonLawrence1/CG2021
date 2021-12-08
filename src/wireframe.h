using namespace std;
using namespace glm;

#define WIDTH 640
#define HEIGHT 480

// Draws a line from point a to b
void drawLine(DrawingWindow& window, CanvasPoint to, CanvasPoint from, uint32_t colour = convertColour(Colour(255, 255, 255))) {
	float numberOfSteps = std::max(abs(to.x - from.x), abs(to.y - from.y));
	float xStep = (to.x - from.x) / numberOfSteps;
	float yStep = (to.y - from.y) / numberOfSteps;

	for (float i = 0.0; i < numberOfSteps; i++) {
		int x = round(from.x + (xStep * i));
		int y = round(from.y + (yStep * i));
		// check if pixel isn't out of bounds
		if (x < WIDTH && x > 0 && y > 0 && y < HEIGHT) window.setPixelColour(round(x), round(y), colour);
	}
}

// Draws a triangle with only an outline
void drawStrokedTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	uint32_t newColour = convertColour(colour);
	drawLine(window, triangle.v0(), triangle.v1(), newColour);
	drawLine(window, triangle.v0(), triangle.v2(), newColour);
	drawLine(window, triangle.v1(), triangle.v2(), newColour);
}

// Generates a random triangle with just an outline
void randomStrokedTriangle(DrawingWindow& window) {
	drawStrokedTriangle(window, generateRandomTriangle(), Colour{ rand() % 256, rand() % 256, rand() % 256 });
}

// renders scene using wire frames
void renderWireFrame(DrawingWindow& window, vector<ModelTriangle> triangles, vec3 cameraPos, float focalLength, float scaleFactor, mat3 cameraOrientation) {
	window.clearPixels();
	for (int i = 0; i < triangles.size(); i++) {
		CanvasPoint pos0 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[0], focalLength, scaleFactor, cameraOrientation);
		CanvasPoint pos1 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[1], focalLength, scaleFactor, cameraOrientation);
		CanvasPoint pos2 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[2], focalLength, scaleFactor, cameraOrientation);
		drawStrokedTriangle(window, CanvasTriangle(pos0, pos1, pos2), triangles[i].colour);
	}
}