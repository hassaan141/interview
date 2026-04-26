#include <ctime>
#include <iostream>

template <typename T>
T max( T x, T y){
	return (x > y) ? x : y;
}

int main () {
	
	std::cout << max(1.0, 2.1) <<std::endl;						

}

