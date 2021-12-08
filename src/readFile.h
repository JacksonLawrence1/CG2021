using namespace std;
using namespace glm;

// Convert a colour to uint32_t
uint32_t convertColour(Colour colour) {
	if (colour.red > 255) colour.red = 255;
	if (colour.green > 255) colour.green = 255;
	if (colour.blue > 255) colour.blue = 255;

	uint32_t c = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
	return c;
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
			ModelTriangle currentTriangle(currentVectors[lineVector[0] - 1], currentVectors[lineVector[1] - 1], currentVectors[lineVector[2] - 1], colour);
			// calculates normal of triangle from 2 edges
			currentTriangle.normal = glm::cross(currentTriangle.vertices[1] - currentTriangle.vertices[0], currentTriangle.vertices[2] - currentTriangle.vertices[0]);
			v.push_back(currentTriangle);
		}
	}
	return v;
}

// Unloads a .obj file and stores the models in a vector
vector<ModelTriangle> unloadTextureFile(string fileName, float scalingFactor, vector<Colour> colourVec) {
	ifstream file(fileName);
	string line;
	vector<ModelTriangle> v;
	vector<glm::vec3> currentVectors;
	vector<TexturePoint> textureVectors;
	Colour colour;
	while (!file.eof()) {
		getline(file, line);
		vector<string> currentLine = split(line, ' ');
		if (line[0] == 'u') {
			colour = getColour(currentLine[1], colourVec);
		}
		else if (line[0] == 'v' && (line[1] != 't' && line[1] != 'n')) {
			glm::vec3 lineVector(stof(currentLine[1]) * scalingFactor, stof(currentLine[2]) * scalingFactor, stof(currentLine[3]) * scalingFactor);
			currentVectors.push_back(lineVector);
		}
		else if (line[0] == 'v' && line[1] == 't') {
			TexturePoint point(stof(currentLine[1]), stof(currentLine[2]));
			textureVectors.push_back(point);
		}
		else if (line[0] == 'f') {
			string currentLineString = line;
			replace(currentLineString.begin(), currentLineString.end(), '/', ' ');
			vector<string> lineVector = split(currentLineString, ' '); 

			lineVector.erase(lineVector.begin()); // remove f at beginning
			lineVector.erase(remove(lineVector.begin(), lineVector.end(), ""), lineVector.end()); // remove all empty spaces


			if (lineVector.size() == 3) {
				ModelTriangle currentTriangle(currentVectors[stof(lineVector[0])-1], currentVectors[stof(lineVector[1])-1], currentVectors[stof(lineVector[2])-1], colour);
				// calculates normal of triangle from 2 edges
				currentTriangle.normal = glm::cross(currentTriangle.vertices[1] - currentTriangle.vertices[0], currentTriangle.vertices[2] - currentTriangle.vertices[0]);
				v.push_back(currentTriangle);
			}
			else if (lineVector.size() == 6) {
				ModelTriangle currentTriangle(currentVectors[stof(lineVector[0])-1], currentVectors[stof(lineVector[2])-1], currentVectors[stof(lineVector[4])-1], colour);
				// calculates normal of triangle from 2 edges
				currentTriangle.normal = glm::cross(currentTriangle.vertices[1] - currentTriangle.vertices[0], currentTriangle.vertices[2] - currentTriangle.vertices[0]);
				currentTriangle.colour = Colour(255, 255, 255);
				currentTriangle.colour.texture = true;
				currentTriangle.texturePoints = { textureVectors[stof(lineVector[1])-1], textureVectors[stof(lineVector[3])-1], textureVectors[stof(lineVector[5])-1]};
				v.push_back(currentTriangle);
			}
		}
	}
	return v;
}

vector<ModelTriangle> getVertexNormals(vector<ModelTriangle> triangles) {
	for (int i = 0; i < triangles.size(); i++) {
		vec3 v0 = triangles[i].vertices[0];
		vec3 sum(0, 0, 0);
		// get 1st vertex normal
		for (int j = 0; j < triangles.size(); j++) {
			if (v0 == triangles[j].vertices[0] || v0 == triangles[j].vertices[1] || v0 == triangles[j].vertices[2]) {
				sum = sum + triangles[j].normal;
			}
		}
		triangles[i].vertex_normals[0] = normalize(sum);
		// get 2nd vertex normal
		vec3 v1 = triangles[i].vertices[1];
		sum = vec3(0, 0, 0);
		for (int j = 0; j < triangles.size(); j++) {
			if (v1 == triangles[j].vertices[0] || v1 == triangles[j].vertices[1] || v1 == triangles[j].vertices[2]) {
				sum = sum + triangles[j].normal;
			}
		}
		triangles[i].vertex_normals[1] = normalize(sum);
		// get 3rd vertex normal
		vec3 v2 = triangles[i].vertices[2];
		sum = vec3(0, 0, 0);
		for (int j = 0; j < triangles.size(); j++) {
			if (v2 == triangles[j].vertices[0] || v2 == triangles[j].vertices[1] || v2 == triangles[j].vertices[2]) {
				sum = sum + triangles[j].normal;
			}
		}
		triangles[i].vertex_normals[2] = normalize(sum);
	}
	return triangles;
}


vector<ModelTriangle> unloadNewFile(string fileName, float scalingFactor, vector<Colour> colourVec) {
	ifstream file(fileName);
	string line;
	vector<ModelTriangle> v;
	vector<glm::vec3> currentVectors;
	vector<glm::vec3> currentNormals;
	Colour colour;
	while (!file.eof()) {
		getline(file, line);
		vector<string> currentLine = split(line, ' ');
		if (line[0] == 'v' && (line[1] != 't' && line[1] != 'n')) {
			glm::vec3 lineVector(stof(currentLine[1]) * scalingFactor, stof(currentLine[2]) * scalingFactor, stof(currentLine[3]) * scalingFactor);
			currentVectors.push_back(lineVector);
		} 	if (line[0] == 'u') {
			colour = getColour(currentLine[1], colourVec);
		}
		else if (line[0] == 'v' && line[1] == 'n') {
			glm::vec3 lineVector(stof(currentLine[1]) * scalingFactor, stof(currentLine[2]) * scalingFactor, stof(currentLine[3]) * scalingFactor);
			currentNormals.push_back(lineVector);
		}
		else if (line[0] == 'f') {
			glm::vec3 lineVector(stof(currentLine[1]), stof(currentLine[2]), stof(currentLine[3]));
			
			ModelTriangle currentTriangle(currentVectors[lineVector[0] - 1], currentVectors[lineVector[1] - 1], currentVectors[lineVector[2] - 1], colour);
			// calculates normal of triangle from 2 edges
			currentTriangle.normal = glm::cross(currentTriangle.vertices[1] - currentTriangle.vertices[0], currentTriangle.vertices[2] - currentTriangle.vertices[0]);
			currentTriangle.colour = colour;
			if (colour.name == "Grey") {
				currentTriangle.material = 2;
			}
			if (colour.name == "Orange") {
				currentTriangle.material = 3;
			}

			if (currentNormals.size() > 0) currentTriangle.vertex_normals = {currentNormals[lineVector[0] - 1], currentNormals[lineVector[1] - 1], currentNormals[lineVector[2] - 1]};
			v.push_back(currentTriangle);
		}
	}
	return getVertexNormals(v);
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
		else if (line[0] == 'm') {
			if (c.size() == 0) {
				Colour newColour(255, 255, 255);
				newColour.texture = true;
				c.push_back(newColour);
			}
			else c.back().texture = true;

		}
	}
	return c;
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

	std::vector<std::vector<float>> point_to_point = interpolateCoordinates(tc0, tc1, steps);
	std::vector<uint32_t> colourMap;
	for (int i = 0; i < point_to_point.size(); i++) {
		colourMap.push_back(sortedTexture[round(point_to_point[i][1])][round(point_to_point[i][0])]);
	}
	return colourMap;
}

// scales texture point based on height and width of texture
TexturePoint scaleTexturePoint(TextureMap texture, TexturePoint point) {
	int height = texture.height;
	int width = texture.width;
	return TexturePoint(point.x * width, point.y * height);
}