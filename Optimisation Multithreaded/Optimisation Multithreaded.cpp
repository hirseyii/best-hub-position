//A program to optimise the placement of a hub to serve cities and towns in the UK
//reads data from GBplaces.csv
//uses hill climb to find the minimum average distance between the hub and the cities

//IMRPOVEMENTS (mostly contained in total_distance function)
//repeats this hill climb for different random starting points within a box defining the edges of the UK
//distance is population weighted - for population greater than 500,000 2 visits are needed etc.
//for distances > 150 miles, distance is multiplied by 1.1 times for a better approximation
//for distances > 250 miles, distance is multiplied by 1.4 times


//NOTES
//variables 'step' and 'NRand_evals' can be changed to make the program run faster but give a less precise position/less accurate position
//each hill climb is parallelised to improve performance

//PREAMBLE
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <time.h>
#include <future>

using namespace std;

//define structure to hold info about each place
struct place {
	double lat;
	double lon;
	double pop;
	double AvDist;
};

//DECLARE FUNCTIONS
//function to read a matrix from a comma delimited file and store in 2D vector
vector <place> readData(string readfilename);

//function to calculate the Great Circle distance between two positions
double great_circle(place A, place B);

//function to convert degrees to radians
double rad(double angle);

//function to return a random number between two limits, lower and upper
double random_coords(double lower, double upper, int n);

//function to calculate total great circle distance between a point and a vector of points
double total_distance(place point, vector<place> places, double latMin, double latMax);

//hillclimb including multiple random starts
place hillclimb(double latMin, double latMax, double longMin, double longMax, vector<place> places, double step, int NRand_evals);

/////////////////////////////////////////////////////////////////////////////////////

//Count number of times the distance is calculated
int f_evals = 0;

int main() {
	vector<place> GBplacesData;										//vector to hold GBplaces
	place hubN;														//'place' to hold position of Northern hub
	place hubS;														//'place' to hold position of Southern hub
	place singlehub;												//'place' to hold position of a single hub

	//variables to store result of hillclimb function
	place N1;
	place N2;
	place S1;
	place S2;
	place single1;
	place single2;
	//read GBplaces and store data in vector
	GBplacesData = readData("GBplaces.csv");
	//rng limits
	double latMin = 49.949228;										//min latitude for rng and hill climb limit
	double latMax = 58.631250;										//max latitude for rng and hill climb limit
	double longMin = -5.786260;										//min longitude for rng and hill climb limit
	double longMax = 1.871858;										//max longitude for rng and hill climb limit
	
	//These parameters can be altered to fit requirements. Changing position of dividing line will change pay back time 

	double step = 0.005;											//resolution of hill climb
	int NRand_evals = 250;											//number of random starting points to try
	double dividingLine = 52.5;										//North-South dividing line to isolate the two hubs
	double ppm = 5;													//price per mile
	double hub_cost = 12000000;										//price per hub
	double running_cost = 10000;									//daily running cost

	//seed rng
	srand(time(NULL));

	//call hillclimb function
	//North
	auto hillN1 = async(hillclimb, dividingLine, latMax, longMin, longMax, GBplacesData, step, NRand_evals);
	auto hillN2 = async(hillclimb, dividingLine, latMax, longMin, longMax, GBplacesData, step, NRand_evals);
	//South
	auto hillS1 = async(hillclimb, latMin, dividingLine, longMin, longMax, GBplacesData, step, NRand_evals);
	auto hillS2 = async(hillclimb, latMin, dividingLine, longMin, longMax, GBplacesData, step, NRand_evals);
	//Single hub
	auto hillSingle1 = async(hillclimb, latMin, latMax, longMin, longMax, GBplacesData, step, NRand_evals);
	auto hillSingle2 = async(hillclimb, latMin, latMax, longMin, longMax, GBplacesData, step, NRand_evals);

	//get results from async streams
	//North
	N1 = hillN1.get();
	N2 = hillN2.get();
	//South
	S1 = hillS1.get();
	S2 = hillS2.get();
	//Single hub
	single1 = hillSingle1.get();
	single2 = hillSingle2.get();
	//find minimum between the two hillclimbs
	//North
	if (N1.AvDist >= N2.AvDist) {
		hubN = N1;
	}
	else {
		hubN = N2;
	}
	//South
	if (S1.AvDist >= S2.AvDist) {
		hubS = S1;
	}
	else {
		hubS = S2;
	}
	//Single hub
	if (single1.AvDist >= single2.AvDist) {
		singlehub = single1;
	}
	else {
		singlehub = single2;
	}
	//print out position and distance
	cout << "Hubs     :" << "Latitude Longitude :" << "Total Distance \n";
	cout << "Northern :" << hubN.lat << " " << hubN.lon << " : " << hubN.AvDist << "\n";
	cout << "Southern :" << hubS.lat << " " << hubS.lon << " : " << hubS.AvDist << "\n";
	cout << "One hub  :" << singlehub.lat << " " << singlehub.lon << " : " << singlehub.AvDist << "\n";
	//print total number of function evaluations
	cout << "Function evaluations: " << f_evals << "\n";

	//calculate total price for each scenario assuming all places are visited in a day
	//2 hubs
	//assume running and set up ofthe smaller hubs are each 2/3 the cost of the larger hub
	double factor = 2 * 0.66666666666;
	double ppDay2 = ppm * (hubN.AvDist + hubS.AvDist) + (2 * factor * running_cost);
	double start_up2;
	start_up2 = hub_cost * factor;

	//1 hub
	double ppDay1 = ppm * singlehub.AvDist + running_cost;
	double start_up1 = hub_cost;
	double payback;

	if (ppDay1 > ppDay2) {
		cout << "2 hubs is cheaper to run by " << ppDay1 - ppDay2 << " GBP/day \n";
		payback = (start_up2 - start_up1) / (ppDay1 - ppDay2);
		cout << "number of days to pay off the difference is " << payback << "days. \n";
		exit(1);
	}
	else if (ppDay1 < ppDay2) {
		cout << "1 hub is cheaper to run by " << ppDay2 - ppDay1 << " GBP/day \n";
		payback = (start_up2 - start_up1) / (ppDay2 - ppDay1);
		cout << "number of days to pay off the difference is " << payback << " days. \n";
		exit(1);
	}
	else {
		cout << "Both have the same running cost. 1 hub is cheaper. \n";
		exit(1);
	}
	return 0;
}

//FUNCTIONS

//function to read a matrix from a comma delimited file and store in a vector of 'place' structures
vector <place> readData(string readfilename) {
	//declare vars
	string line;										//string to store lines
	vector <place> storematrix;							//vector to store data
														//open file and check if open
	ifstream dataFile(readfilename);

	if (dataFile.is_open()) {
		cout << "The file opened successfully.\n";
		//skip line of headers
		string firstline;
		getline(dataFile, firstline);

		//loop over file lines and store line as string
		while (!dataFile.eof()) {
			getline(dataFile, line);

			vector <int> commaLocations;					//vector to store location of each comma in each line

															//check line is not empty
			if (line.length() > 0) {

				//parse line for commas and store position in commaLocations
				int comma = 0;								//temp to store comma most recent location
				while (line.find(',', comma + 1) != -1) {
					comma = (int)line.find(',', comma + 1);
					commaLocations.push_back(comma);
				}
				place data;									//structure to hold data for each place

				string lineSegment;							//string to store each element before being converted to double
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
		cout << "An error occurred, the file failed to open.\n";
		exit(1);
	}

	return storematrix;
}

//function to return a random number between two limits, lower and upper
double random_coords(double lower, double upper, int n) {
	// n is the amount of bits to split the range into
	double r;
	r = lower + (rand() % (n + 1) * (1. / n) * (upper - lower));
	return r;
}

//function to convert degrees to radians
double rad(double angle) {
	const double pi = 4 * atan(1);
	double radians = angle * (pi / 180);
	return radians;
}

//function to calculate the Great Circle distance between two positions
double great_circle(place A, place B) {
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
double total_distance(place point, vector<place> places, double latMin, double latMax) {
	//loop over places and find total distance
	double distance = 0;
	for (int n = 0; n < (int)places.size(); n++) {
		if (places[n].lat > latMax || places[n].lat < latMin) {
			//don't look at places outside set boundary
		}
		else {
			//calculate distance between point and place
			double d = great_circle(places[n], point);

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
			f_evals++;
		}
	}
	return distance;
}

//hillclimb including multiple random starts
place hillclimb(double latMin, double latMax, double longMin, double longMax, vector<place> places, double step, int NRand_evals) {

	//'place' to store global minimum
	place bestPlace;
	bestPlace.AvDist = 999999999;									//global minimum start point
	bestPlace.lat = 0;
	bestPlace.lon = 0;

	double dist, oldDist, newDist, minDist;							//declare vars to hold total distances
	int dLat, dLong;												//integers to store successful moves
	int iteration = 1;												//count number of steps taken in the hillclimb

	//loop over random points to find a global minimum
	for (int c = 0; c < NRand_evals; c++) {
		//find random number and store in place type
		place startPos;												//place type to hold random x y start position
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
					place testPos;
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