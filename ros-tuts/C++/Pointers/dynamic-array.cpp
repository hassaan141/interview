#include <iostream>
using namespace std;

int main() {
    int size;
    cout << "Enter the size of the array: ";
    cin >> size;

    // Dynamically allocate memory for the array
    int* myArray = new int[size];

    // Initialize the array
    for (int i = 0; i < size; ++i) {
        myArray[i] = i + 1;
    }

    // Print the array
    cout << "Array elements: ";
    for (int i = 0; i < size; ++i) {
        cout << myArray[i] << " ";
    }
    cout << endl;

    // Deallocate the memory
    delete[] myArray;

    return 0;
}