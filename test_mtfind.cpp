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

const string file_path  = "./strfile.txt";
const int line_count = 100500;
vector<size_t> str_words{500, 900, 4800,4800*4 ,4800*5 , 4800*12,4800/17 , line_count - 2};
string fnded_tword = "??sde main??";  



/**
 * создать случайную строку с  фиксированным алфавитом и заданной длиной
*/
std::string random_string(std::size_t length,  const std::string CHARACTERS = "UEFQEFJWOEIFHWUADFNWEITRTPRYOEIERQFNQKENFZXCMMVBNASLDKFKJG")
{
    static std::random_device random_device;
    static std::mt19937 generator(random_device());
    static std::uniform_int_distribution<> distribution(4, CHARACTERS.size() - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

void randomise_test_params()
{
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> short_distr(3, 10);
    fnded_tword = random_string(short_distr(generator), "?abcde ????? hgfdjs kfgh????djs kad??????hfjska*&^%$fghi ?? sfdgs??123456   ???");

     std::uniform_int_distribution<> line_distr(3, line_count-1);
    std::uniform_int_distribution<> count_distr(5, 45);

    size_t cnt = count_distr(generator);
    str_words.clear();
    for(size_t i=0;i<cnt;++i)
    {
        str_words.push_back( line_distr(generator)  );
    }

}
/**
 * сгенерировать файл для поиска
*/
void gen_file( map <pair<size_t , size_t>, pos_string> &d_sourse)
{
    ofstream strfile;
    strfile.open( file_path );
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(100, 500);

    if (strfile.is_open())
    {
       
        for (size_t i = 0; i < line_count; i++)
        {
            string ins_words = fnded_tword;
            string rnds = random_string(distribution(generator));
            if (count(str_words.begin(), str_words.end(), i) == 0)
                ins_words = "";
            else
            {
                d_sourse[make_pair(i, rnds.length()) ]  =  (pos_string){rnds.length() , fnded_tword } ;
            }
            strfile <<rnds  << ins_words << random_string(distribution(generator)) << endl;
        }
    }

    cout << " gen ok \n";
    strfile.close();
}



int main(int argc, char **argv)
{
    const size_t test_count = 5;
    size_t count_test_ok = 0;
   
    for (size_t i = 0; i < test_count; ++i)
    {
        cout << " TEST #" <<i<<endl;;
        map< pair<size_t , size_t> , pos_string> d_sourse;
        gen_file(d_sourse);

        cout << " GENERATED DATA: \n";
        for (auto it : d_sourse)
        {
            cout << it.first.first << " " << it.second.pos << " " << it.second.s << endl;
        }
        cout << " ------------- \n";

        auto t_start = std::chrono::high_resolution_clock::now();
        pattern_string str_pattern(fnded_tword);
        vector<pair<size_t, pos_string>> result;
        get_pattern_in_file(str_pattern, file_path, result);
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        long count_ok = 0;
        for (auto &r : result)
        {
            if (d_sourse[make_pair(r.first ,r.second.pos )] == r.second)
            {
                cout << "OK " << r.first << " " << r.second.pos << " " << r.second.s << endl;
                count_ok++;
            }
            else
            {
                cout << "FAIL " << r.first << " " << r.second.pos << " " << r.second.s << endl;
            }
        }

        if (result.size() == count_ok)
        {
            cout << "\n\n TEST PASSED\n";
            count_test_ok++;
        }
        cout << "  elapsed time , ms: " << elapsed_time_ms << endl<< endl<< endl;
        randomise_test_params();
    }

    if(count_test_ok == test_count)
    {
        cout<<" ALL TESTS OK\n";
    }
    else
    {
        cout<<count_test_ok <<"   TESTS OK\n";
        cout<<test_count - count_test_ok <<"   TESTS FAIL\n";
    }
    

        return 0;
   
}


