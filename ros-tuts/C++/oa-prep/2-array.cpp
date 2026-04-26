#include <iostream>
#include <vector>

int main() {
    std::vector<std::vector<int>> array = {{1, 2, 3},
                                           {4, 5, 6},
                                           {7, 8, 9}};

    // Finding the number of rows
    int num_floors = array.size();
    std::cout << num_floors << std::endl;  // Outputs: 3

    // Finding the number of columns
    int num_units = array[0].size();
    std::cout << num_units << std::endl;  // Outputs: 3

    array.push_back({10, 11});

    for(auto i: array){
        for(int j: i){
            std::cout<<j<<" ";
        }
        std::cout<<std::endl;
    }

    array[1].erase(array[1].end() - 1);

    return 0;
} 