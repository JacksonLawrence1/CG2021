using namespace std;
using namespace glm;

#define WIDTH 320
#define HEIGHT 240

// Finds equivalent vertex point on window 
CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength, float scale, mat3 cameraOrientation) {
	glm::vec3 correctedVertices = vertexPosition - cameraPosition;
	correctedVertices = correctedVertices * cameraOrientation;
	float u = (focalLength * (correctedVertices[0] / correctedVertices[2]) * -scale) + (WIDTH / 2);
	float v = (focalLength * (correctedVertices[1] / correctedVertices[2]) * scale) + (HEIGHT / 2);
	return CanvasPoint(round(u), round(v), correctedVertices[2]);
}

// Finds closest triangle that intersects 
RayTriangleIntersection getClosestIntersection(glm::vec3 source, glm::vec3 rayDirection, vector<ModelTriangle> triangles, int triangleIndex = -1) {
	RayTriangleIntersection currentClosest;
	currentClosest.distanceFromCamera = numeric_limits<float>::max();

	for (int i = 0; i < triangles.size(); i++) {
		glm::vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
		glm::vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
		glm::vec3 SPVector = source - triangles[i].vertices[0];
		glm::mat3 DEMatrix(-rayDirection, e0, e1);
		glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

		float t = possibleSolution[0];
		float u = possibleSolution[1];
		float v = possibleSolution[2];

		if ((u >= 0.0) && (u <= 1.0) && (v >= 0.0) && (v <= 1.0) && (u + v) <= 1.0) {
			// skips checking for triangle i if triangleIndex is specified
			if (t < currentClosest.distanceFromCamera && t > 0 && triangleIndex != i) {
				currentClosest.distanceFromCamera = t;
				currentClosest.intersectedTriangle = triangles[i];
				currentClosest.triangleIndex = i;
				currentClosest.intersectionPoint = glm::vec3(triangles[i].vertices[0] + (u * e0) + (v * e1));
			}
		}
	}
	return currentClosest;
}

// renders scene using ray-tracing
void renderRayTracedScene(DrawingWindow& window, vec3 cameraPos, mat3 cameraOrientation, vector<ModelTriangle> triangles, vec3 light, int lightingMode, float focalLength, float scaleFactor) {
	window.clearPixels();
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {

			// Calculates x and y position in 3D space equivalent to the .obj file
			// scale factor ^2 ensures scaling matches with rasterized and wireframe render
			float u = ((x + cameraPos.x) - WIDTH / 2) / scaleFactor;
			float v = -((y + cameraPos.y) - HEIGHT / 2) / scaleFactor;

			// Adjusts direction in accordance to camera orientation and position
			vec3 rayDirection(u, v, cameraPos.z - focalLength);
			rayDirection = rayDirection - cameraPos;
			rayDirection = rayDirection * cameraOrientation;

			RayTriangleIntersection closestIntersection = getClosestIntersection(cameraPos, rayDirection, triangles);
			float brightness = 1;

			// Colours hard shadows black
			if (lightingMode == 1) brightness = hardShadowLighting(closestIntersection, triangles, light);

			// Colours pixels based on how far from light source (Proximity lighting)
			if (lightingMode == 2) brightness = proximityLighting(closestIntersection, light);

			// Colours pixels based on how far from light source (Proximity lighting)
			if (lightingMode == 3) brightness = diffuseLighting(closestIntersection, light);

			// specular lighting on surface
			if (lightingMode == 4) brightness = specularLighting(closestIntersection, cameraPos, light);

			// utilization of all lighting effects
			if (lightingMode == 5) brightness = allLighting(closestIntersection, cameraPos, triangles, light);

			window.setPixelColour(x, y, convertColour(calculateBrightness(closestIntersection, brightness, false)));

		}
	}
}