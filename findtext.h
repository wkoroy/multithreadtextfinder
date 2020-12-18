#ifndef FINDETEXT_H
#define FINDETEXT_H

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
#include <string>
#include <map>


struct pos_string
{
    size_t pos;
    std::string  s;
     int operator==(const pos_string& right)const;
};
// g++  -o ./task1    ./task1.cpp    -std=c++14 -O3 -lpthread
using namespace std;

/**
 * класс сравнения паттерна
*/
class pattern_string
{
public:
    pattern_string(const string &ptrn);

    bool compare(const string &str) const;
    size_t get_len() const;
    string get_basic_word(size_t &offset) const;

private:
    vector<size_t> compared_index; // индексы значащих ячеек  паттерна
    string pattern;
    size_t len;               // длина паттерна
    string basic_word;        // опорное слово паттерна
    size_t basic_word_offset; // смешение опорного слова
};

/**
 * сбор массивов строк для последующей пердачи на обработку
*/
bool string_processing_in_file(const pattern_string &str_pattern, ifstream &file, size_t limit, std::function<void(const pattern_string &ptrn, uint64_t line_number, const vector<string> &vstr)> proc_strings);

/**
 * поиск по массиву строк 
 */
void get_find_pos(const pattern_string &val, const size_t offset_str_number, const string *array, const size_t count, vector<pair<size_t, pos_string>> &result);

/* 
    разделение на потоки поиска по массиву строк  vstr
*/
void proc_strings(const pattern_string &str_pattern, const uint64_t line_number, const vector<string> &vstr , 
    vector<pair<size_t, pos_string>> &result);

/*
  найти паттерновые строки в файле
 */
void print_pattern_in_file(const pattern_string &ps, string filepath);
void get_pattern_in_file(const pattern_string &ps, string filepath ,  vector<pair<size_t, pos_string>> &result);
#endif
