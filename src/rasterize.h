using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

uint32_t convertColour(Colour colour);
vector<std::vector<uint32_t>> unloadTexture(TextureMap texture);
vector<uint32_t> getColourMap(vector<float> t0, vector<float> t1, int steps, vector<vector<uint32_t>> sortedTexture);
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength, float scale, mat3 cameraOrientation);

vector<vector<float>> depthBuffer(WIDTH, std::vector<float>(HEIGHT, 0));

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

// Draws filled triangle taking into account depth buffer
void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	// Creates a template structure for storing all points which will be sorted
	std::vector<CanvasPoint> sorted = sortPoints(triangle.v0(), triangle.v1(), triangle.v2());
	std::vector<std::vector<float>> xRange = getxRanges(sorted[0], sorted[1], sorted[2], sorted[3]);

	// Draws triangle by iterating through y and then only drawing if x values are in particular range given the y (marked)
	for (int y = sorted[0].y; y < sorted[3].y; y++) {
		float first = xRange[y - sorted[0].y][0]; // beginning of x range
		float second = xRange[y - sorted[0].y][1]; // end of x range
		std::vector<float> depthPoints = interpolateSingleFloats(xRange[y - sorted[0].y][2], xRange[y - sorted[0].y][3], second - first + 1);
		for (int x = first; x <= second; x++) {
			float s = 1 / abs(depthPoints[x - first]);
			if ((x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) && s > depthBuffer[x][y]) {
				window.setPixelColour(x, y, convertColour(colour));
				depthBuffer[x][y] = s;
			}
		}
	}
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
		std::vector<uint32_t> currentLineColour = getColourMap(test0, test1, xRange[y - sorted[0].y][1] - xRange[y - sorted[0].y][0] + 10, sortedTexture);
		for (int x = round(xRange[y - sorted[0].y][0]); x < round(xRange[y - sorted[0].y][1]); x++) {
			auto test3 = x - xRange[y - sorted[0].y][0];
			if (currentLineColour.size() != 0) { window.setPixelColour(x, y, currentLineColour[test3]); }
		}
	}
	//Colour white{ 255, 255, 255 };
	//drawStrokedTriangle(window, triangle, white);
}

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

void randomFilledTriangle(DrawingWindow& window) {
	drawFilledTriangle(window, generateRandomTriangle(), Colour{ rand() % 256, rand() % 256, rand() % 256 });
}

// renders scene using rasterization
void renderRasterizedScene(DrawingWindow& window, vec3 cameraPos, vector<ModelTriangle> triangles, float focalLength, float scaleFactor, mat3 cameraOrientation) {
	window.clearPixels();

	for (int i = 0; i < triangles.size(); i++) {
		CanvasPoint pos0 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[0], focalLength, scaleFactor, cameraOrientation);
		CanvasPoint pos1 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[1], focalLength, scaleFactor, cameraOrientation);
		CanvasPoint pos2 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[2], focalLength, scaleFactor, cameraOrientation);
		drawFilledTriangle(window, CanvasTriangle(pos0, pos1, pos2), triangles[i].colour);
	}

	depthBuffer = vector<vector<float>>(WIDTH, std::vector<float>(HEIGHT, 0));
}
