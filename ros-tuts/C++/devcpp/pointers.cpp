//Pointers
#include <iostream>

//& is address-of pointers
// * dereference operator 

int main (){
	
	std::string name = "Bro";
	
	std::string *pName = &name;
	
	std::cout << *pName<<'\n';
	
	//Array is already an address, so we dont need to use the address of operator
	std::string freePizzas[5] = {"pizza1", "pizza2", "pizza3", "pizza4", "pizza5"};
	
	std::cout << *freePizzas<<'\n';

	//so when referencing an array, you can just do this,
	//reduntent to create a pointer but can if you want cuz an array is already a pointer
	
	std::string * pPizza = freePizzas;
	
	std::cout<<*pPizza;
	
	
	//Null pointer - a special value value which means that the it is not pointing to anything
	
	//nullptr is a keyboard, useful for first asigning pointers
	
}