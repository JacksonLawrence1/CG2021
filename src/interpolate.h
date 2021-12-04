using namespace std;
using namespace glm;

// (1, 1) and (3, 3) step 3 = (1,1),(2,2),(3,3)
vector<float> interpolateSingleFloats(float from, float to, int numberofValues) {
	vector<float> v;
	float difference = (to - from) / (numberofValues);
	for (int i = 0; i < numberofValues; i++) {
		v.push_back(from + (i * difference));
	}
	return v;
}

// same as above function but evaluates 3 values instead of 2, e.g. (1,1,1) to (3,3,3) 
vector<vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberofValues) {
	vector<vector<float>> v;
	for (int i = 0; i < 3; i++) {
		v.push_back(interpolateSingleFloats(from[i], to[i], numberofValues));
	}
	vector<vec3> finalVec;
	for (int j = 0; j < numberofValues; j++) {
		vec3 newVec;
		newVec[0] = v[0][j];
		newVec[1] = v[1][j];
		newVec[2] = v[2][j];
		finalVec.push_back(newVec);
	}
	return finalVec;
}

// gets appropriate window coords for the line specified but will divide into specified chunks
vector<vector<float>> interpolateCoordinates(CanvasPoint from, CanvasPoint to, int steps) {
	vector<float> xList = interpolateSingleFloats(from.x, to.x, steps);
	vector<float> yList = interpolateSingleFloats(from.y, to.y, steps);
	vector<float> depthList = interpolateSingleFloats(from.depth, to.depth, steps);
	vector<std::vector<float>> v;

	int xRound = 0;
	int yRound = 0;
	int depthRound = 0;
	for (int i = 0; i < steps; i++) {
		v.push_back({ xList[i], yList[i], depthList[i] });
	}

	return v;
}