//
// Created by wwd on 2021/9/15.
//

#include <iostream>
#include <string>
int main(){
    std::string test = "http://127.0.0.1/?wwd=1&wsx=666";
    auto result = test.find('?');
    std::cout << test.substr(result + 1, test.size() - result);
}