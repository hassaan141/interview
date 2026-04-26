//pointers and arrays
#include <iostream>
using namespace std;

int main () {
  
  int lucky [5] = {1, 2, 3, 4, 5};
  
  //This prints out the address of the first index
  cout<<lucky<<endl;
  //also prints out the address of the first index
  cout<<&lucky[0]<<endl;
  
  //prints out the value in the 2nd index
  cout<<lucky[2]<<endl;
  //also prints out the value in the 2nd index, address of the first element and then add 2 more spaces to that
  cout<<*(lucky + 2)<<endl;
  
  int enterNumbers [5];
  
  for(int i = 0; i<5; i++){
  	cout<<"Please Enter A Number: ";
  	cin>>enterNumbers[i];
  }
  
  for(int i = 0; i<6; i++){
  	cout<<*(enterNumbers+i)<<" ";
  }

  
}