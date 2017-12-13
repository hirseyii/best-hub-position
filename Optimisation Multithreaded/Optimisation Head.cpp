#include"Optimisation Head.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <time.h>
#include <future>

//function to read a matrix from a comma delimited file and store in a vector of 'place' structures
std::vector <optim_hub::place> optim_hub::readData(std::string readfilename) {
	//declare vars
	std::string line;										//string to store lines
	std::vector <place> storematrix;							//vector to store data
														//open file and check if open
	std::ifstream dataFile(readfilename);

	if (dataFile.is_open()) {
		std::cout << "The file opened successfully.\n";
		//skip line of headers
		std::string firstline;
		std::getline(dataFile, firstline);

		//loop over file lines and store line as string
		while (!dataFile.eof()) {
			getline(dataFile, line);

			std::vector <int> commaLocations;					//vector to store location of each comma in each line

															//check line is not empty
			if (line.length() > 0) {

				//parse line for commas and store position in commaLocations
				int comma = 0;								//temp to store comma most recent location
				while (line.find(',', comma + 1) != -1) {
					comma = (int)line.find(',', comma + 1);
					commaLocations.push_back(comma);
				}
				place data;									//structure to hold data for each place

				std::string lineSegment;							//string to store each element before being converted to double
				double lat;									//double to store lat
				double lon;									//double to store longitude
				double pop;									//double to store population

															//seperate each element
				lineSegment = line.substr(commaLocations[1] + 1, (commaLocations[2] - 1 - (commaLocations[1])));
				pop = atof(lineSegment.c_str());
				data.pop = pop;

				lineSegment = line.substr(commaLocations[2] + 1, (commaLocations[3] - 1 - (commaLocations[2])));
				lat = atof(lineSegment.c_str());
				data.lat = lat;

				lineSegment = line.substr(commaLocations[3] + 1, (line.length() - 1 - (commaLocations[3])));
				lon = atof(lineSegment.c_str());
				data.lon = lon;

				storematrix.push_back(data);
			}
		}
		dataFile.close();
	}
	else {
		std::cout << "An error occurred, the file failed to open.\n";
		exit(1);
	}

	return storematrix;
}

//function to return a random number between two limits, lower and upper
double optim_hub::random_coords(double lower, double upper, int n) {
	// n is the amount of bits to split the range into
	double r;
	r = lower + (rand() % (n + 1) * (1. / n) * (upper - lower));
	return r;
}

//function to convert degrees to radians
double optim_hub::rad(double angle) {
	const double pi = 4 * atan(1);
	double radians = angle * (pi / 180);
	return radians;
}

//function to calculate the Great Circle distance between two positions
double optim_hub::great_circle(optim_hub::place A, optim_hub::place B) {
	//declare vars
	const double R = 3958.75;
	double latA = rad(A.lat);
	double latB = rad(B.lat);
	double longA = rad(A.lon);
	double longB = rad(B.lon);

	double dLat = latB - latA;
	double dLong = longB - longA;
	double a = pow(sin(dLat / 2), 2) + cos(latA) * cos(latB) * pow(sin(dLong / 2), 2);
	double c = 2 * atan(sqrt(a) / sqrt(1 - a));
	double dist = R * c;
	return dist;
}

//function to calculate total great circle distance between a point and a vector of points within defined north-south box
double optim_hub::total_distance(optim_hub::place point, std::vector<optim_hub::place> places, double latMin, double latMax) {
	//loop over places and find total distance
	double distance = 0;
	for (int n = 0; n < (int)places.size(); n++) {
		if (places[n].lat > latMax || places[n].lat < latMin) {
			//don't look at places outside set boundary
		}
		else {
			//calculate distance between point and place
			double d = optim_hub::great_circle(places[n], point);

			//there and back is 2d
			d = 2 * d;
			//better distance approx
			if (d > 150) {
				d = d * 1.2;
			}
			else if (d > 250) {
				d = d * 1.4;
			}
			//multiple visits needed for higher populations
			if (places[n].pop >= 250000) {
				d = d * 2;
			}
			else if (places[n].pop >= 500000) {
				d = d * 3;
			}
			else if (places[n].pop >= 750000) {
				d = d * 4;
			}
			else if (places[n].pop >= 1000000) {
				d = d * 4;
			}
			distance += d;
			//count evaluations of function
			//--------------------------------------f_evals++;
		}
	}
	return distance;
}

//hillclimb including multiple random starts
optim_hub::place optim_hub::hillclimb(double latMin, double latMax, double longMin, double longMax, std::vector<place> places, double step, int NRand_evals) {

	//'place' to store global minimum
	optim_hub::place bestPlace;
	bestPlace.AvDist = 999999999;									//global minimum start point
	bestPlace.lat = 0;
	bestPlace.lon = 0;

	double dist, oldDist, newDist, minDist;							//declare vars to hold total distances
	int dLat, dLong;												//integers to store successful moves
	int iteration = 1;												//count number of steps taken in the hillclimb

																	//loop over random points to find a global minimum
	for (int c = 0; c < NRand_evals; c++) {
		//find random number and store in place type
		optim_hub::place startPos;												//place type to hold random x y start position
		startPos.lat = random_coords(latMin, latMax, 1000);
		startPos.lon = random_coords(longMin, longMax, 1000);

		dist = total_distance(startPos, places, latMin, latMax);

		//loop to reduce totalDist
		do {
			oldDist = dist;
			minDist = oldDist;
			//generate 8 surrounding points
			for (int a = -1; a <= 1; a++) {
				for (int b = -1; b <= 1; b++) {
					//calculate new point and store in temporary test position array
					optim_hub::place testPos;
					testPos.lat = startPos.lat + step * a;
					testPos.lon = startPos.lon + step * b;

					if (a == b == 0) {
						//don't need to look at current position
					}
					else if (testPos.lat > latMax || testPos.lat < latMin) {
						//don't look at places outside UK box
					}
					else if (testPos.lon > longMax || testPos.lon < longMin) {
						//don't look at places outside UK box
					}
					else {
						//calculate dist at new point
						newDist = total_distance(testPos, places, latMin, latMax);
						//check against minimum distance
						if (newDist < minDist) {
							//store the movement
							dLat = a;
							dLong = b;
							minDist = newDist;
						}
					}
				}
			}
			//update start position to new minimum
			startPos.lat = startPos.lat + step * dLat;
			startPos.lon = startPos.lon + step * dLong;
			//update value
			dist = minDist;
			//print new movement
			iteration++;

		} while (dist < oldDist);										//continue iterating until the distance cant get smaller			

		if (dist < bestPlace.AvDist) {
			bestPlace.AvDist = dist;
			bestPlace.lat = startPos.lat;
			bestPlace.lon = startPos.lon;
		}
	}
	return bestPlace;
}