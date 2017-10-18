# best-hub-position
Calculates the optimum position of 1 or 2 'delivery' hubs in the UK using hillclimb methods.

* uses hill climb to find the minimum average distance between the hub and the cities

* repeats this hill climb for different random starting points within a box defining the edges of the UK

* distance is population weighted - for population greater than 500,000 2 visits are needed etc.

* for distances > 150 miles, distance is multiplied by 1.1 times for a better approximation

* for distances > 250 miles, distance is multiplied by 1.4 times

Makes use of async to run concurrent hillclimbs.

