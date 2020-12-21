#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <iterator>
#include <random>


double averageArrival = 5;
double lamda = 1 / averageArrival;
std::mt19937 rng (0);
std::exponential_distribution<double> poi (lamda);
std::uniform_int_distribution<> dis(0, 9);
std::uniform_real_distribution<double> unif(0, 1);

double sumArrivalTimes=0;
double newArrivalTime;

double poisson()
{
  newArrivalTime = poi.operator() (rng);// generates the next random number in the distribution
  sumArrivalTimes = sumArrivalTimes + newArrivalTime;
  std::cout << "newArrivalTime:  " << newArrivalTime  << "    ,sumArrivalTimes:  " << sumArrivalTimes << std::endl;
  if (sumArrivalTimes<3.6)
  {
    sumArrivalTimes=3.6;
  }
  return sumArrivalTimes;
}
