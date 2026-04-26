#include <iostream>

int bubbleSort(int arr[], int size);

int main () {
	
	int arr [] = {9, 1, 8, 2, 7, 3, 6, 4, 5};
	int size = sizeof(arr) / sizeof(arr[0]); 
	
	bubbleSort(arr, size);
	
	for(int i : arr){
		std::cout<< i << " ";
	}
	
	std::cout<<std::endl;
	std::string foods [100];
	fill (foods, foods+100, "Pizza");
	
	
//	for(std::string j : foods){
//		std::cout<< j << " ";
//	};
	
	//fill 1/3 with different things
	int Ssze = 99;
	std::string diff[Ssze];
	fill(diff, diff + (Ssze/3), "Burger");
	fill(diff + (Ssze/3), diff + (Ssze/3)*2, "Pizza");
	fill(diff + (Ssze/3)*2, diff + Ssze, "Chicken");
	
	for (std::string i: diff){
		std::cout<< i << " ";
	}
	
	
}

int bubbleSort(int arr[], int size){
	
	for(int i =0; i<size - 1; i++){
		for(int j = 0; j<size - i - 1; j++){
			
			if(arr[j] > arr[j+1]){
				int temp = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = temp;
			}
		}
	}
}