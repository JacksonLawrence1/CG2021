using namespace std;
using namespace glm;

#define WIDTH 640
#define HEIGHT 480

// Finds closest triangle that intersects 
RayTriangleIntersection getClosestIntersection(glm::vec3 source, glm::vec3 rayDirection, vector<ModelTriangle> triangles, int triangleIndex = -1, int material=-1) {
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
			if (t < currentClosest.distanceFromCamera && t > 0 && triangleIndex != i && triangles[i].material != material) {
				currentClosest.distanceFromCamera = t;
				currentClosest.intersectedTriangle = triangles[i];
				currentClosest.triangleIndex = i;
				currentClosest.intersectionPoint = glm::vec3(triangles[i].vertices[0] + (u * e0) + (v * e1));
				currentClosest.u = u;
				currentClosest.v = v;

			}
		}
	}
	return currentClosest;
}

// renders scene using ray-tracing
void renderRayTracedScene(DrawingWindow& window, vector<ModelTriangle> triangles, vec3 cameraPos, mat3 cameraOrientation, vec3 light, int lightingMode, float focalLength, float scaleFactor) {
	window.clearPixels();
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {

			// Calculates x and y position in 3D space equivalent to the .obj file
			// scale factor ^2 ensures scaling matches with rasterized and wireframe render
			float u = ((x + cameraPos.x) - WIDTH / 2) / scaleFactor;
			float v = -((y + cameraPos.y) - HEIGHT / 2) / scaleFactor;

			// Adjusts direction in accordance to camera orientation and position
			vec3 rayDirection(u, v, -focalLength);
			rayDirection = normalize(rayDirection * cameraOrientation);

			RayTriangleIntersection closestIntersection = getClosestIntersection(cameraPos, rayDirection, triangles);
			
			float brightness = 1;

			// Colours hard shadows black
			if (lightingMode == 1) brightness = hardShadowLighting(closestIntersection, triangles, light);

			// Colours pixels based on how far from light source (Proximity lighting)
			if (lightingMode == 2) brightness = diffuseLighting(closestIntersection, light);

			// utilization of all lighting effects
			if (lightingMode == 3) brightness = allLighting(closestIntersection, cameraPos, triangles, light);

			// OPTIONAL :: add hard shadow
			if (lightingMode == 4) brightness = gouraudShade(closestIntersection, cameraPos, light);

			// OPTIONAL :: add hard shadow
			if (closestIntersection.intersectedTriangle.colour.name == "Red" && lightingMode >= 2) brightness = phongShade(closestIntersection, cameraPos, light);

			closestIntersection = checkMirror(closestIntersection, triangles, light);

			window.setPixelColour(x, y, convertColour(calculateBrightness(closestIntersection, brightness, true)));

		}
	}
}