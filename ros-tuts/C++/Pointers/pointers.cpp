#include <iostream>
#include <string>
#include <vector>
using namespace std;


int main () {

    int n = 5;
    cout<<n<<endl;
    cout<<&n<<endl;
    int* nPtr = &n;
    cout<<"The address to n is "<<nPtr<<endl;
    cout<<"To get the value of our address, we have to dereference it "<<*nPtr<<endl;
    *nPtr = 10;
    cout<<"The changed nPtr is 10 but n is also now changed "<<nPtr<<endl;



    return 0;
}


