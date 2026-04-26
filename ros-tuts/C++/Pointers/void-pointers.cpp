//void pointer can hold the address of a char, struct, int etc
//can point to a variable of any data type

#include <iostream>
using namespace std;
//this is how you print a normal pointer
void printNumber(int *number){
  std::cout<< *number << std::endl;
}

//this is how you print a normal pointer
void printLetter(char *charPtr){
  std::cout<< *charPtr << std::endl;
}

//void pointer can hold the address of a char, struct, int etc

//downside is that we can never know if an error happens like this so be careful
void print(void*ptr, char type){
  switch(type){
    case 'i': cout<< *((int*)ptr)<<endl;
    break;
    case 'c': cout<< *((char*)ptr)<<endl;
    break;
    case 'f': cout<< *((float*)ptr)<<endl;
    break;

  }
}

int main () {
  
  int number = 5;
  char letter = 'A';
  float decimal = 5.5;
  printNumber(&number);
  printLetter(&letter);
  print(&number, 'i');
  print(&letter, 'c');
  print(&decimal, 'f');

  
}