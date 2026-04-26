#include <iostream>

int main(){
	
//	1
//	1 2
//	1 2 3
//	1 2 3 4
//	1 2 3 4 5
	for(int i = 1; i <= 5; i++ ){
		for(int j = 1; j<=i; j++){
			std::cout<<j<< " ";
		}
		std::cout<<std::endl;
	}
	
//	1
//	2 2
//	3 3 3
//	4 4 4 4
//	5 5 5 5 5
	
	std::cout<<std::endl;
	for(int i = 1; i <= 5; i++ ){
		for(int j = 1; j<=i; j++){
			std::cout<<i<< " ";
		}
		std::cout<<std::endl;
	}
	
//	1 2 3 4 5
//	1 2 3 4
//	1 2 3
//	1 2
//	1
	
	std::cout<<std::endl;
	for(int i = 5; i >0; i-- ){
		for(int j = 1; j<=i; j++){
			std::cout<<j<< " ";
		}
		std::cout<<std::endl;
	}
	
//	1 2 3 4 5
//	2 3 4 5
//	3 4 5
//	4 5
//	5

	std::cout<<std::endl;
	for(int i = 5; i >0; i-- ){
		for(int j = i+1; j<=i; j++){
			std::cout<<j<< " ";
		}
		std::cout<<std::endl;
	}

}