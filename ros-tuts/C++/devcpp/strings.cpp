#include <iostream>

int main(){
	
	std::string name;
	
	std::cout<< "What is your name? ";
	std::getline(std::cin, name);
	//to get the length, use .length
	std::cout<<name.length()<<std::endl;
	
	std::cout<<"To check if your name is empty:  " <<name.empty()<<std::endl;
	
	//to clear the name use, .clear
	
	//To append string, use .append()
	
	std::cout<<"To append a string with @gmail.com "<< name.append("@gmail.com")<<std::endl;
	
	//To get a certain character of the string, we can do .at(), which gives us the index for the respective string
	
	std::cout<<"The first letters index is "<<name.at(0) << std::endl;
	
	//To insert at a given position, we can do .insert() method, which takes the index where we want to insert and what we want to insert
	
	std::cout<<"Inserting ! infront of my name "<<name.insert(0, "!")<<std::endl;
	
	std::cout<<"Inserting & at the end " <<name.insert((name.length()), "&") <<std::endl;
	
	//.find() gives you the index of the first instance of a character
	
	//.earse() eliminates a portion of the string
	
//	std::cout<<"Earing @gmail.com and after is " << name.erase(name.find("@"), (name.length - 1))<<std::endl;
	std::cout<<"Finding first instance of @ is "<<name.find("@")<<std::endl;
	std::cout<<"Earing @gmail.com and after is " << name.erase(0,3)<<std::endl;

}