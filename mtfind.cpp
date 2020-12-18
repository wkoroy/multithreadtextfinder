#include <iostream>
#include <random>
#include <fstream>
#include <functional>
#include <cstdint>
#include <memory>
#include <thread>
#include <future>
#include <chrono>
#include <utility>
#include <algorithm>
#include <map>
#include "findtext.h"

// g++  -o ./task1    ./task1.cpp    -std=c++14 -O3 -lpthread
using namespace std;
 


int main(int argc, char **argv)
{

    //  gen_file();
    if (argc == 3)
    {
        pattern_string str_pattern(argv[2]);
        string file_path = argv[1];
        print_pattern_in_file(str_pattern, file_path);
    }
    else
    {
        cout << " using: /$mtfind filepath \"template\" \n";
        return 0;
    }
}


