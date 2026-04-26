#include <iostream>
#include<vector>


using std::cout;
using std::cin;
using std::string;
using std::vector;
using std::pair;
using std::endl;

//how to use namespace
namespace first{
  int x = 1;
}

//typedef is easy for long data types, ex
typedef std::vector<pair<string, int>> pairlist_t;

//using is for short data types, ex
using str = string;
using pairlist_tu = vector<pair<string, int>>;

typedef std::string str;

int main() {

    pairlist_tu pairlist;

    pairlist = {
      {"Jown", 1},
      {"Doe", 2}
      };

    //type is casting either implicit or explicit

    int x1 = 3.14; //implicit
    double x2 = (int) 3.14; //explicit
    char x3 = 100; //implicit, will show the aski value of 100
    // int x4 = (char) 100; //explicit, will show 100

    cout << x2 << endl;
    cout << x1 << endl;
    cout << x3 << endl;
    cout << (char) 100 << endl;
    
    cout << pairlist[0].first << " " << pairlist[0].second << "\n";
    cout << "Hello, World!" << first::x;

    //what is the username
    string x_username;
    cout << "What is your name? ";
    cin >> x_username;
    cout
    return 0;
}