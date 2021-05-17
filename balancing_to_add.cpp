//
// Created by andrey on 16.05.2021.
//

#include <vector>
#include <cstdlib>

int start_balancing(std::vector<int> upstream) {  // upstream[0] = 0
    for (int i = 1; i < upstream.size(); ++i) {
        upstream[i] = upstream[i] + upstream[i-1];
    }

    int sum = upstream[upstream.size() - 1];

    int random_variable = std::rand() % sum;
    int number_left_element = 0;
    int number_right_element = upstream.size();
    int middle;

    while(number_right_element >= number_left_element) {
        middle = (number_left_element + number_right_element) / 2;
        if (upstream[middle] > random_variable) {
            number_right_element--;
        } else if (upstream[middle] < random_variable) {
            number_left_element++;
        } else return number_left_element = middle;
    }

    return number_left_element--;
}

//TODO: Или сразу добавлять вектор сумм, или это делать внутри функции как сейчас