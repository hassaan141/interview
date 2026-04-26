#include <iostream>
#include <vector>
using namespace std;

int main(){

    vector<int> v1 = {1, 2, 3, 4};
    cout << v1[2]<<endl;
    cout << v1.front()<<endl;
    cout << v1.back()<<endl;
    cout << v1.size()<<endl;

    cout<<endl;
    cout<<v1.capacity()<<endl;
    v1.push_back(9);
    //capacity is always doubled if overflowed

    cout<<v1.capacity()<<endl;

    //poping in a vector
    v1.pop_back();
    cout << v1.back()<<endl;

    //shrik capacity is done by shrik to fit
    v1.shrink_to_fit();

    vector<int> v2 = {1, 2, 3, 4, 5, 6};


    //IMPORTANT v1.begin() is a pointer. 
    //To add in a specific index
    v2.insert(v2.begin(), 5);
    cout<<v2[0]<<endl;

    v2.insert(v2.begin()+1, 23);
    cout<<v2[1]<<endl;

    v2.erase(v1.begin());




    return 0;
}