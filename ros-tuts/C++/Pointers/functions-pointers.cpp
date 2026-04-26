#include <iostream>
using namespace std;

int getMin(int numbers[], int size){
	
	int min = numbers[0];
	for(int i = 0; i<size; i++){
		cout<<numbers[i]<< " ";
		if(numbers[i]<min){
			min=numbers[i];
		}
	}
	cout<<endl;
	cout<<min;
}

void getMinAndMax(int numbers[], int size, int* min, int* max){
	
	for(int i = 0; i<size; i++){
		if(numbers[i] < *min){
			*min=numbers[i];
		}
		if(numbers[i] > *max){
			*max=numbers[i];
		}
	}
	
}

int main (){
	
	int numbers[5] = {5, 4, -3, 29, 6};
	int size = sizeof(numbers)/sizeof(numbers[0]);
	
	getMin(numbers, size);
	
	int min= numbers[0];
	int max= numbers[0];
	getMinAndMax(numbers, size, &min, &max);
	cout<<endl;
	cout<<"min is: "<<min<<" and max is: "<<max;
	
	
	
	
	return 0;
}