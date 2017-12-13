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
#include"Optimisation Head.h"

//define structure to hold info about each place



/////////////////////////////////////////////////////////////////////////////////////

//Count number of times the distance is calculated
int f_evals = 0;

int main() {
	std::vector<optim_hub::place> GBplacesData;										//vector to hold GBplaces
	optim_hub::place hubN;														//'place' to hold position of Northern hub
	optim_hub::place hubS;														//'place' to hold position of Southern hub
	optim_hub::place singlehub;												//'place' to hold position of a single hub

	//variables to store result of hillclimb function
	optim_hub::place N1;
	optim_hub::place N2;
	optim_hub::place S1;
	optim_hub::place S2;
	optim_hub::place single1;
	optim_hub::place single2;
	//read GBplaces and store data in vector
	GBplacesData = optim_hub::readData("GBplaces.csv");
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
	auto hillN1 = async(optim_hub::hillclimb, dividingLine, latMax, longMin, longMax, GBplacesData, step, NRand_evals);
	auto hillN2 = async(optim_hub::hillclimb, dividingLine, latMax, longMin, longMax, GBplacesData, step, NRand_evals);
	//South
	auto hillS1 = async(optim_hub::hillclimb, latMin, dividingLine, longMin, longMax, GBplacesData, step, NRand_evals);
	auto hillS2 = async(optim_hub::hillclimb, latMin, dividingLine, longMin, longMax, GBplacesData, step, NRand_evals);
	//Single hub
	auto hillSingle1 = async(optim_hub::hillclimb, latMin, latMax, longMin, longMax, GBplacesData, step, NRand_evals);
	auto hillSingle2 = async(optim_hub::hillclimb, latMin, latMax, longMin, longMax, GBplacesData, step, NRand_evals);

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
	std::cout << "Hubs     :" << "Latitude Longitude :" << "Total Distance \n";
	std::cout << "Northern :" << hubN.lat << " " << hubN.lon << " : " << hubN.AvDist << "\n";
	std::cout << "Southern :" << hubS.lat << " " << hubS.lon << " : " << hubS.AvDist << "\n";
	std::cout << "One hub  :" << singlehub.lat << " " << singlehub.lon << " : " << singlehub.AvDist << "\n";
	//print total number of function evaluations
	//------------------------------------------------std::cout << "Function evaluations: " << f_evals << "\n";

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
		std::cout << "2 hubs is cheaper to run by " << ppDay1 - ppDay2 << " GBP/day \n";
		payback = (start_up2 - start_up1) / (ppDay1 - ppDay2);
		std::cout << "1 hub is cheaper to run by " << ppDay2 - ppDay1 << " GBP/day \n";
		std::cout << "number of days to pay off the difference is " << payback << "days. \n";
		exit(1);
	}
	else if (ppDay1 < ppDay2) {
		std::cout << "1 hub is cheaper to run by " << ppDay2 - ppDay1 << " GBP/day \n";
		payback = (start_up2 - start_up1) / (ppDay2 - ppDay1);
		std::cout << "number of days to pay off the difference is " << payback << " days. \n";
		exit(1);
	}
	else {
		std::cout << "Both have the same running cost. 1 hub is cheaper. \n";
		exit(1);
	}
	return 0;
}
