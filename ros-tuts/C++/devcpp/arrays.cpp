#include <iostream>
#include <bits/stdc++.h>
using std::string;

int getTotalGrades(int grades[], int gradeSize);

int main(){
	
	string car[] = {"Car", "Truck"};
	car[0] = "Bike";
	std::cout<<car[0]<<std::endl;
	
	//how to set a size then declare later
	string something[4];
	something[0] = "A";
	something[1] = "A";
	something[2] = "A";
	something[3] = "A";

	//using the for each loop to iterate over a iterable dataset
	
	int grades[] = {23, 43, 54, 65 ,23 ,54, 675, 75, 23 ,13};
	
	for(int grade: grades){
		std::cout<<grade<<std::endl;
	}
	
	//passing an array in a function
	int gradeSize = sizeof(grades)/sizeof(grades[0]);
	int totalGrades = getTotalGrades(grades, gradeSize);
	std::cout<<"The total grades are: "<<totalGrades<<std::endl;
}

int getTotalGrades(int grades[], int gradeSize){
	
	int total;
	for(int i = 0; i<gradeSize; i++){
		total += grades[i];
	}
	
	return total;
}