using namespace std;
using namespace glm;

RayTriangleIntersection getClosestIntersection(vec3 source, vec3 rayDirection, vector<ModelTriangle> triangles, int triangleIndex);

// returns black if surface cannot see light
float hardShadowLighting(RayTriangleIntersection surface, vector<ModelTriangle> triangles, vec3 light) {
	// calculate direction of surface to light source
	vec3 rayShadowDirection = light - surface.intersectionPoint;
	rayShadowDirection = normalize(rayShadowDirection);
	vec3 normalizedIntersection = normalize(surface.intersectionPoint);

	// find closest intersection between light source
	RayTriangleIntersection shadowRay = getClosestIntersection(surface.intersectionPoint, rayShadowDirection, triangles, surface.triangleIndex);
	float distanceToIntersection = distance(normalize(shadowRay.intersectionPoint), normalizedIntersection);
	float distanceToLight = distance(light, normalizedIntersection);

	// if distance to intersection is closer to surface then colour set triangle colour to black
	if (distanceToIntersection < distanceToLight) return 0;
	else return 1;
}

// darkens pixels based on distance from light
float proximityLighting(RayTriangleIntersection surface, vec3 light) {
	float lightDistance = distance(surface.intersectionPoint, light);
	float brightness = 1 / (3 * pow(lightDistance, 2));

	if (brightness > 1) return 1;
	else return brightness;
}

// darkens pixels based on angle from light
float diffuseLighting(RayTriangleIntersection surface, vec3 light) {
	float brightness = proximityLighting(surface, light);

	// find angle of incidence in accordance to light
	vec3 toLight = light - surface.intersectionPoint;
	float angleOfIncidence = dot(normalize(surface.intersectedTriangle.normal), toLight);
	if (angleOfIncidence < 0) angleOfIncidence = 0;
	return brightness * (angleOfIncidence * 1.7);
}

// specularly illuminated surface
float specularLighting(RayTriangleIntersection surface, vec3 cameraPos, vec3 light, int spread = 256) {
	vec3 lightVector = light - surface.intersectionPoint;
	vec3 normal = surface.intersectedTriangle.normal;
	vec3 reflectionVector = normalize(lightVector - (2.0f * normal * dot(lightVector, normal)));
	vec3 direction = normalize(cameraPos - surface.intersectionPoint);
	float brightness = pow(dot(direction, reflectionVector), spread);

	return brightness;
}

float allLighting(RayTriangleIntersection surface, vec3 cameraPos, vector<ModelTriangle> triangles, vec3 light) {
	//float shadow = hardShadowLighting(surface, triangles, light);
	float diffuse = diffuseLighting(surface, light);
	float spec = specularLighting(surface, cameraPos, light, 16);

	//if (shadow < diffuse) return shadow;
	if (spec > diffuse) return spec;
	else return diffuse;
}

Colour calculateBrightness(RayTriangleIntersection surface, float brightness, bool ambiance = true) {
	Colour colour = surface.intersectedTriangle.colour;
	// ambiance and defaulted to true
	if (brightness < 0.2 && ambiance) brightness = 0.2;
	return Colour(colour.red * brightness, colour.green * brightness, colour.blue * brightness);
}

float shadingHelper(RayTriangleIntersection surface, vec3 point, vec3 normal, vec3 cameraPos, vec3 light) {
	// combine brightness functions 
	float brightness = proximityLighting(surface, light);
	vec3 toLight = light - point;
	float angleOfIncidence = dot(normalize(normal), toLight);
	float spec = specularLighting(surface, cameraPos, light, 256);

	if (angleOfIncidence < 0) angleOfIncidence = 0;
	// ensures no overflow
	return std::min((brightness * angleOfIncidence) + spec, 1.0f);
}

float gouraudShade(RayTriangleIntersection surface, vec3 cameraPos, vec3 light) {
	// get brightness value for each vertex so can be interpolated
	float b0 = shadingHelper(surface, surface.intersectionPoint, surface.intersectedTriangle.vertex_normals[0], cameraPos, light);
	float b1 = shadingHelper(surface, surface.intersectionPoint, surface.intersectedTriangle.vertex_normals[1], cameraPos, light);
	float b2 = shadingHelper(surface, surface.intersectionPoint, surface.intersectedTriangle.vertex_normals[2], cameraPos, light);

	float u = surface.u;
	float v = surface.v;

	// interpolate brightness between vertices b0, b1, b2
	float brightness = b0 + u*(b1-b0) + v*(b2-b0);

	return brightness;
}

float phongShade(RayTriangleIntersection surface, vec3 cameraPos, vec3 light) {
	// get brightness value for each vertex so can be interpolated
	vec3 n0 = surface.intersectedTriangle.vertex_normals[0];
	vec3 n1 = surface.intersectedTriangle.vertex_normals[1];
	vec3 n2 = surface.intersectedTriangle.vertex_normals[2];
	float u = surface.u;
	float v = surface.v;

	// interpolate normal between normals n0, n1, n2
	vec3 newNormal = n0 + u * (n1 - n0) + v * (n2 - n0);

	float brightness = shadingHelper(surface, surface.intersectionPoint, newNormal, cameraPos, light);

	return brightness;
}
