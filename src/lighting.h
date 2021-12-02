using namespace std;
using namespace glm;

RayTriangleIntersection getClosestIntersection(glm::vec3 source, glm::vec3 rayDirection, vector<ModelTriangle> triangles, int triangleIndex);


// returns black if surface cannot see light
float hardShadowLighting(RayTriangleIntersection surface, vector<ModelTriangle> triangles, vec3 light) {
	// calculate direction of surface to light source
	glm::vec3 rayShadowDirection = light - surface.intersectionPoint;
	rayShadowDirection = glm::normalize(rayShadowDirection);
	glm::vec3 normalizedIntersection = glm::normalize(surface.intersectionPoint);

	// find closest intersection between light source
	RayTriangleIntersection shadowRay = getClosestIntersection(surface.intersectionPoint, rayShadowDirection, triangles, surface.triangleIndex);
	float distanceToIntersection = glm::distance(glm::normalize(shadowRay.intersectionPoint), normalizedIntersection);
	float distanceToLight = glm::distance(light, normalizedIntersection);

	// if distance to intersection is closer to surface then colour set triangle colour to black
	if (distanceToIntersection < distanceToLight) return 0;
	else return 1;
}

// darkens pixels based on distance from light
float proximityLighting(RayTriangleIntersection surface, vec3 light) {
	float lightDistance = glm::distance(surface.intersectionPoint, light);
	float brightness = 1 / (3 * glm::pow(lightDistance, 2));

	if (brightness > 1) return 1;
	else return brightness;
}

// darkens pixels based on angle from light
float diffuseLighting(RayTriangleIntersection surface, vec3 light) {
	float brightness = proximityLighting(surface, light);

	// find angle of incidence in accordance to light
	glm::vec3 toLight = light - surface.intersectionPoint;
	float angleOfIncidence = glm::dot(glm::normalize(surface.intersectedTriangle.normal), toLight);
	if (angleOfIncidence < 0) angleOfIncidence = 0;
	return brightness * (angleOfIncidence * 1.7);
}

// specularly illuminated surface
float specularLighting(RayTriangleIntersection surface, vec3 cameraPos, vec3 light, int spread = 256) {
	glm::vec3 lightVector = light - surface.intersectionPoint;
	glm::vec3 normal = surface.intersectedTriangle.normal;
	glm::vec3 reflectionVector = glm::normalize(lightVector - (2.0f * normal * glm::dot(lightVector, normal)));
	glm::vec3 direction = normalize(cameraPos - surface.intersectionPoint);
	float brightness = pow(glm::dot(direction, reflectionVector), spread);

	return brightness;
}

float allLighting(RayTriangleIntersection surface, vec3 cameraPos, vector<ModelTriangle> triangles, vec3 light) {
	float shadow = hardShadowLighting(surface, triangles, light);
	float diffuse = diffuseLighting(surface, light);
	float spec = specularLighting(surface, cameraPos, light, 16);

	if (shadow < diffuse) return shadow;
	//if (spec > diffuse) return spec;
	else return diffuse;
}

Colour calculateBrightness(RayTriangleIntersection surface, float brightness, bool ambiance = true) {
	Colour colour = surface.intersectedTriangle.colour;
	// ambiance and defaulted to true
	if (brightness < 0.2 && ambiance) brightness = 0.2;
	return Colour(colour.red * brightness, colour.green * brightness, colour.blue * brightness);
}

// darkens pixels based on angle from light
float diffuseLightingGouraud(RayTriangleIntersection surface, vec3 point, vec3 normal, vec3 light) {
	float brightness = proximityLighting(surface, light);

	// find angle of incidence in accordance to light
	glm::vec3 toLight = light - point;
	float angleOfIncidence = glm::dot(glm::normalize(normal), toLight);
	if (angleOfIncidence < 0) angleOfIncidence = 0;
	return brightness * (angleOfIncidence + 0.1);
}

float gouraudLighting(RayTriangleIntersection surface, int x, int y, vec3 light) {
	float a = diffuseLightingGouraud(surface, surface.intersectedTriangle.vertices[0], surface.intersectedTriangle.vertex_normals[0], light);
	float b = diffuseLightingGouraud(surface, surface.intersectedTriangle.vertices[1], surface.intersectedTriangle.vertex_normals[1], light);
	float c = diffuseLightingGouraud(surface, surface.intersectedTriangle.vertices[2], surface.intersectedTriangle.vertex_normals[2], light);

	float w1 = (a + b) / 2;
	float w2 = (a + c) / 2;
	float p = a + w1 * (b - a) + w2 * (c - a);
	if (p > 1) return 1;
	else return p;
}
