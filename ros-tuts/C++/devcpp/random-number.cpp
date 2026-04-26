#include <iostream>
#include <bits/stdc++.h>

int main(){
	
	srand(time(0));
	
	for (int i = 0; i<10; i++)
		std::cout<< rand() % 4 <<std::endl;

}