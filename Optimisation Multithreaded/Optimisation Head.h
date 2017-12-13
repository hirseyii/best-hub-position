#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <time.h>
#include <future>

namespace optim_hub {
	//===========================================
	// STRUCTS
	//===========================================

	struct place {
		double lat;
		double lon;
		double pop;
		double AvDist;
	};


	//===========================================
	// FUNCTIONS
	//===========================================

	//function to read a matrix from a comma delimited file and store in 2D vector
	std::vector <place> readData(std::string readfilename);

	//function to calculate the Great Circle distance between two positions
	double great_circle(place A, place B);

	//function to convert degrees to radians
	double rad(double angle);

	//function to return a random number between two limits, lower and upper
	double random_coords(double lower, double upper, int n);

	//function to calculate total great circle distance between a point and a vector of points
	double total_distance(place point, std::vector<place> places, double latMin, double latMax);

	//hillclimb including multiple random starts
	place hillclimb(double latMin, double latMax, double longMin, double longMax, std::vector<place> places, double step, int NRand_evals);
}
