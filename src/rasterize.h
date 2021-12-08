using namespace std;
using namespace glm;

#define WIDTH 640
#define HEIGHT 480

uint32_t convertColour(Colour colour);
vector<std::vector<uint32_t>> unloadTexture(TextureMap texture);
vector<uint32_t> getColourMap(vector<float> t0, vector<float> t1, int steps, vector<vector<uint32_t>> sortedTexture);
void drawTextureTriangle(DrawingWindow window, CanvasTriangle triangle, string filename);
TexturePoint scaleTexturePoint(TextureMap texture, TexturePoint point);


vector<vector<float>> depthBuffer(WIDTH, std::vector<float>(HEIGHT, 0));

// Finds equivalent vertex point on window 
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength, float scale, mat3 cameraOrientation) {
	glm::vec3 correctedVertices = vertexPosition - cameraPosition;
	correctedVertices = correctedVertices * cameraOrientation;
	float u = (focalLength * (correctedVertices[0] / correctedVertices[2]) * -scale) + (WIDTH / 2);
	float v = (focalLength * (correctedVertices[1] / correctedVertices[2]) * scale) + (HEIGHT / 2);
	return CanvasPoint(round(u), round(v), correctedVertices[2]);
}

// designates maximum and minimum x values spanning all y values in a triangle
// Inputs MUST be sorted in order of ascending y values
vector<vector<float>> getxRanges(CanvasPoint p0, CanvasPoint p1, CanvasPoint p2, CanvasPoint p3) {
	std::vector<float> xMins;
	std::vector<float> xMaxs;
	std::vector<float> xDepthMin;
	std::vector<float> xDepthMax;

	std::vector<std::vector<float>> top_to_left_edge = interpolateCoordinates(p0, p1, p2.y - p0.y);
	std::vector<std::vector<float>> top_to_right_edge = interpolateCoordinates(p0, p2, p2.y - p0.y);

	// Gets x value ranges for 'top' portion of triangle
	for (int i = p0.y; i < p2.y; i++) {
		xMins.push_back(top_to_left_edge[round(i - p0.y)][0]);
		xMaxs.push_back(top_to_right_edge[round(i - p0.y)][0]);
		xDepthMin.push_back(top_to_left_edge[round(i - p0.y)][2]);
		xDepthMax.push_back(top_to_right_edge[round(i - p0.y)][2]);
	}

	std::vector<std::vector<float>> left_to_bot = interpolateCoordinates(p1, p3, p3.y - p2.y);
	std::vector<std::vector<float>> right_to_bot = interpolateCoordinates(p2, p3, p3.y - p2.y);

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
vector<vector<vector<float>>> getInterpolatedRanges(vector<CanvasPoint> sorted) {
	std::vector<std::vector<float>> leftCoordinate;
	std::vector<std::vector<float>> rightCoordinate;

	CanvasPoint t0(sorted[0].texturePoint.x, sorted[0].texturePoint.y);
	CanvasPoint t1(sorted[1].texturePoint.x, sorted[1].texturePoint.y);
	CanvasPoint t2(sorted[2].texturePoint.x, sorted[2].texturePoint.y);
	CanvasPoint t3(sorted[3].texturePoint.x, sorted[3].texturePoint.y);

	std::vector<std::vector<float>> top_to_left_tex = interpolateCoordinates(t0, t1, sorted[1].y - sorted[0].y);
	std::vector<std::vector<float>> top_to_right_tex = interpolateCoordinates(t0, t2, sorted[1].y - sorted[0].y);

	// Gets x value ranges for 'top' portion of triangle
	for (int i = 0; i < sorted[1].y - sorted[0].y; i++) {
		leftCoordinate.push_back({ top_to_left_tex[i][0], top_to_left_tex[i][1] });
		rightCoordinate.push_back({ top_to_right_tex[i][0], top_to_right_tex[i][1] });
	}

	std::vector<std::vector<float>> left_to_bot_tex = interpolateCoordinates(t1, t3, sorted[3].y - sorted[1].y);
	std::vector<std::vector<float>> right_to_bot_tex = interpolateCoordinates(t2, t3, sorted[3].y - sorted[1].y);

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
vector<CanvasPoint> sortPoints(CanvasPoint v0, CanvasPoint v1, CanvasPoint v2) {
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
	// placeholder for texture 
	sorted[3].texturePoint = TexturePoint(-1, -1);

	std::vector<std::vector<float>> longEdge = interpolateCoordinates(sorted[0], sorted[2], sorted[2].y - sorted[0].y);

	// Gets vertex opposite of middle corner
	for (int i = 0; i < longEdge.size(); i++) {
		if (longEdge[i][1] == sorted[1].y) {
			sorted[3].x = round(longEdge[i][0]);
			sorted[3].depth = longEdge[i][2];
		}
	}

	return sorted;
}

// Draws filled triangle taking into account depth buffer
void drawFilledTriangle(DrawingWindow& window, CanvasTriangle triangle, Colour colour) {
	// Creates a template structure for storing all points which will be sorted
	std::vector<CanvasPoint> sorted = sortPoints(triangle.v0(), triangle.v1(), triangle.v2());

	// Leftmost point deemed by sorted[1], rightmost deemed by sorted[2]
	if (sorted[1].x > sorted[3].x) {
		std::swap(sorted[1], sorted[3]);
		std::swap(sorted[2], sorted[3]);
	}
	else { std::swap(sorted[2], sorted[3]); }
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

void drawTopTriangle(DrawingWindow& window, vector<CanvasPoint> points, vector<std::vector<uint32_t>> texture) {
	int rows = points[2].y - points[0].y;
	CanvasPoint texture0 = CanvasPoint(points[0].texturePoint.x, points[0].texturePoint.y);
	CanvasPoint texture1 = CanvasPoint(points[1].texturePoint.x, points[1].texturePoint.y);
	CanvasPoint texture2 = CanvasPoint(points[2].texturePoint.x, points[2].texturePoint.y);

	vector<float> topToBot = interpolateSingleFloats(points[0].x, points[1].x, rows);
	vector<vector<float>> textureTopToBot = interpolateCoordinates(texture0, texture1, rows);
	
	vector<float> topToMid = interpolateSingleFloats(points[0].x, points[2].x, rows);
	vector<vector<float>> textureTopToMid = interpolateCoordinates(texture0, texture2, rows);
	int textureSize = texture.size() - 1;

	for (int y = 0; y < rows; y++) {
		int rowPixels = topToMid[y] - topToBot[y];
		CanvasPoint t0(textureTopToBot[y][0], textureTopToBot[y][1]);
		CanvasPoint t1(textureTopToMid[y][0], textureTopToMid[y][1]);


		if (rowPixels < 0) {
			rowPixels = abs(rowPixels);
			vector<vector<float>> textureScaled = interpolateCoordinates(t0, t1, rowPixels);

			for (int x = 0; x < rowPixels; x++) {
				int a = round(textureScaled[x][0]);
				int b = round(textureScaled[x][1]);


				a = clamp(a, 0, textureSize);
				b = clamp(b, 0, textureSize);

				uint32_t colour = texture[b][a];
				int xValue = topToBot[y] - x;
				int yValue = points[0].y + y;
				if (xValue < WIDTH && xValue > 0 && yValue < HEIGHT && yValue > 0) window.setPixelColour(xValue, yValue, colour);
			}
		}
		else {
			vector<vector<float>> textureScaled = interpolateCoordinates(t0, t1, rowPixels);

			for (int x = 0; x < rowPixels; x++) {
				int a = round(textureScaled[x][0]);
				int b = round(textureScaled[x][1]);

				a = clamp(a, 0, textureSize);
				b = clamp(b, 0, textureSize);

				uint32_t colour = texture[b][a];
				int xValue = topToBot[y] + x;
				int yValue = points[0].y + y;
				if (xValue < WIDTH && xValue > 0 && yValue < HEIGHT && yValue > 0) window.setPixelColour(xValue, yValue, colour);
			}
		}
	}
}

void drawBotTriangle(DrawingWindow& window, vector<CanvasPoint> points, vector<vector<uint32_t>> texture) {
	int rows = points[2].y - points[0].y;

	CanvasPoint texture0 = CanvasPoint(points[0].texturePoint.x, points[0].texturePoint.y);
	CanvasPoint texture1 = CanvasPoint(points[1].texturePoint.x, points[1].texturePoint.y);
	CanvasPoint texture2 = CanvasPoint(points[2].texturePoint.x, points[2].texturePoint.y);

	vector<float> topToBot = interpolateSingleFloats(points[0].x, points[2].x, rows);
	vector<vector<float>> textureTopToBot = interpolateCoordinates(texture0, texture2, rows);

	vector<float> topToMid = interpolateSingleFloats(points[1].x, points[2].x, rows);
	vector<vector<float>> textureTopToMid = interpolateCoordinates(texture1, texture2, rows);
	int textureSize = texture.size() - 1;

	for (int y = 0; y < rows; y++) {
		int rowPixels = topToMid[y] - topToBot[y];

		CanvasPoint t0(textureTopToBot[y][0], textureTopToBot[y][1]);
		CanvasPoint t1(textureTopToMid[y][0], textureTopToMid[y][1]);

		if (rowPixels < 0) {
			rowPixels = abs(rowPixels);
			vector<vector<float>> textureScaled = interpolateCoordinates(t0, t1, rowPixels);
			for (int x = 0; x < rowPixels; x++) {
				int a = round(textureScaled[x][0]);
				int b = round(textureScaled[x][1]);

				a = clamp(a, 0, textureSize);
				b = clamp(b, 0, textureSize);

				uint32_t colour = texture[b][a];
				int xValue = topToBot[y] - x;
				int yValue = points[0].y + y;
				if (xValue < WIDTH && xValue > 0 && yValue < HEIGHT && yValue > 0) window.setPixelColour(xValue, yValue, colour);
			}
		}
		else {
			vector<vector<float>> textureScaled = interpolateCoordinates(t0, t1, rowPixels);
			for (int x = 0; x < rowPixels; x++) {
				int a = round(textureScaled[x][0]);
				int b = round(textureScaled[x][1]);

				a = clamp(a, 0, textureSize);
				b = clamp(b, 0, textureSize);

				uint32_t colour = texture[b][a];
				int xValue = topToBot[y] + x;
				int yValue = points[0].y + y;
				if (xValue < WIDTH && xValue > 0 && yValue < HEIGHT && yValue > 0) window.setPixelColour(xValue, yValue, colour);
			}
		}
	}
}


void drawTexturedTriangle(DrawingWindow& window, CanvasTriangle triangle, TextureMap texture) {
	vector<std::vector<uint32_t>> sortedTexture = unloadTexture(texture);

	// sorted[0-2] of ascending y values, sorted[3] is always the midpoint
	vector<CanvasPoint> sorted = sortPoints(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]);
	float a0 = sorted[0].x - sorted[2].x; 
	float b0 = sorted[0].y - sorted[2].y;
	float a1 = sorted[0].x - sorted[3].x; 
	float b1 = sorted[0].y - sorted[3].y;

	// get proportion to mid point to calculate midpoint on texture
	float topToBot = sqrt(a0*a0 + b0*b0);
	float topToMid = sqrt(a1*a1 + b1*b1);
	float midProportion = topToMid / topToBot;

	// calculate equivalent midpoint on texture
	sorted[3].texturePoint.x = round(((sorted[2].texturePoint.x - sorted[0].texturePoint.x) * midProportion) + sorted[0].texturePoint.x);
	sorted[3].texturePoint.y = round(((sorted[2].texturePoint.y - sorted[0].texturePoint.y) * midProportion) + sorted[0].texturePoint.y);

	vector<CanvasPoint> topTriangle = { sorted[0], sorted[1], sorted[3] };
	vector<CanvasPoint> botTriangle = { sorted[1], sorted[3], sorted[2] };

	drawTopTriangle(window, topTriangle, sortedTexture);
	drawBotTriangle(window, botTriangle, sortedTexture);
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
void renderRasterizedScene(DrawingWindow& window, vector<ModelTriangle> triangles, vec3 cameraPos, float focalLength, float scaleFactor, mat3 cameraOrientation) {
	window.clearPixels();
	
	for (int i = 0; i < triangles.size(); i++) {
		CanvasPoint pos0 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[0], focalLength, scaleFactor, cameraOrientation);
		CanvasPoint pos1 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[1], focalLength, scaleFactor, cameraOrientation);
		CanvasPoint pos2 = getCanvasIntersectionPoint(cameraPos, triangles[i].vertices[2], focalLength, scaleFactor, cameraOrientation);
		if (triangles[i].colour.texture == false) {
			drawFilledTriangle(window, CanvasTriangle(pos0, pos1, pos2), triangles[i].colour);
		}
		//else if (triangles[i].colour.texture == true) {
		else if (triangles[i].colour.texture == true) {
			TextureMap texture = TextureMap("logo_texture.ppm");
			pos0.texturePoint = scaleTexturePoint(texture, triangles[i].texturePoints[0]);
			pos1.texturePoint = scaleTexturePoint(texture, triangles[i].texturePoints[1]);
			pos2.texturePoint = scaleTexturePoint(texture, triangles[i].texturePoints[2]);
			drawTexturedTriangle(window, CanvasTriangle(pos0, pos1, pos2), texture);
		}
	}
	
	depthBuffer = vector<vector<float>>(WIDTH, std::vector<float>(HEIGHT, 0));
}
