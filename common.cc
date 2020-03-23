#include <iostream>

#include "common.hpp"
using namespace std;

struct a{};
int main(){
   cout << thallium::format("123{} {} {}\n", a{}, "b", "c") ;
}
