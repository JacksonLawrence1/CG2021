using namespace std;
using namespace glm;

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

glm::mat3 lookat(glm::vec3 cameraPos, glm::vec3 target = glm::vec3(0.25, 0.25, 0)) {
	glm::vec3 forward = normalize(cameraPos - target);
	glm::vec3 right = normalize((glm::cross(glm::vec3(0, 1, 0), forward)));
	glm::vec3 up = cross(forward, right);
	glm::mat3 newOrientation(right, up, forward);

	// when raytracing must return transpose!
	// return transpose(newOrientation);
	return newOrientation;
}
