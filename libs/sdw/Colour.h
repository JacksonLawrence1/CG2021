#pragma once

#include <iostream>

struct Colour {
	std::string name;
	int red{};
	int green{};
	int blue{};
	bool texture=false;
	Colour();
	Colour(int r, int g, int b);
	Colour(std::string n, int r, int g, int b);
};

std::ostream &operator<<(std::ostream &os, const Colour &colour);
