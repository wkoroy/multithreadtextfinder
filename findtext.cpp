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
#include <array>
#include "findtext.h"


using proc_str_func_t = std::function<void(const pattern_string &ptrn, uint64_t line_number, const vector<string> &vstr ,  vector<pair<size_t, pos_string>> &result  )> ; 


// g++  -o ./task1    ./task1.cpp    -std=c++14 -O3 -lpthread
using namespace std;

pattern_string::pattern_string(const string &ptrn)
{
    pattern = ptrn;
    len = pattern.length();

    bool startwr = false;
    bool endwr = false;
    basic_word_offset = 0;

    pair<size_t, pair<size_t, string>> word;
    word.first = 0;

    // определение индексов для сравнения
    //  и выбор опорного слова -наибольшего по длине
    for (size_t ic = 0; ic < pattern.length(); ic++)
    {
        char c = pattern[ic];
        if (c != '?')
        {
            compared_index.push_back(ic);
            startwr = true;
            if (!basic_word_offset)
                basic_word_offset = ic;
        }
        else
        {
            if (startwr)
                endwr = true;
        }

        if (startwr && !endwr)
        {
            basic_word += c;
        }
        else
        {
            //   выбрать слово максимальной длины
            if (basic_word.length() > word.first)
            {
                word = make_pair(basic_word.length(), make_pair(basic_word_offset, basic_word));
            }

            startwr = false;
            endwr = false;

            basic_word = "";
            basic_word_offset = 0;
        }
    }

    // если срока не кончается [?] , то собрать накопившиеся символы и вычислить опорное слово
    if (startwr && !endwr)
    {
        if (basic_word.length() > word.first)
        {
            cout << basic_word.length() << "  " << basic_word_offset << " - - |" << basic_word << "|" << endl;
            word = make_pair(basic_word.length(), make_pair(basic_word_offset, basic_word));
        }
    }
    basic_word = word.second.second;
    basic_word_offset = word.second.first;
}

bool pattern_string::compare(const string &str) const
{

    if (len != str.length())
        return false;

    bool ok = true;
    for (size_t v : compared_index)
    {
        ok &= (pattern[v] == str[v]);
        if (!ok)
            break;
    }
    return ok;
}

size_t pattern_string::get_len() const
{
    return len;
}

string pattern_string::get_basic_word(size_t &offset) const
{
    offset = basic_word_offset;
    return basic_word;
}

/**
 * сбор массивов строк для последующей пердачи на обработку
*/
bool string_processing_in_file(const pattern_string &str_pattern, ifstream &file, size_t limit,

                               proc_str_func_t proc_strings,
                               vector<pair<size_t, pos_string>> &result)
{
    bool switch_vectors = false;
    uint64_t line_counter = 1;
    vector<string> strings[2]; //  два вектора для двойной буферизации
    for (auto &sv : strings)
        sv.reserve(limit);

    std::string str;

    future<void> thwaiter;
    bool left_data = false;    //  признак наличия необработанных данных
    bool is_last_data = false; //  признак  последнего необработанного вектора

    do
    {
        auto read_res = !std::getline(file, str).eof();
        if (read_res)
            strings[switch_vectors].emplace_back(str);
        else
            is_last_data = strings[switch_vectors].size() > 0;

        left_data = strings[switch_vectors].size() > 0;

        if (strings[switch_vectors].size() == limit || is_last_data)
        {
            if (thwaiter.valid())
                thwaiter.wait(); //   ожидать предыдущий поток обработки
            thwaiter = async(std::launch::async, proc_strings, str_pattern, (line_counter - strings[switch_vectors].size() - is_last_data), ref(strings[switch_vectors]) , ref(result) );
            switch_vectors = !switch_vectors; //  переключить выбираемый буфер
            strings[switch_vectors].clear();
        }
        line_counter += read_res > 0;
    } while (left_data);

    if (thwaiter.valid())
        thwaiter.wait(); //   ожидать последний поток обработки

    return line_counter > 0;
}

/**
 * поиск по массиву строк 
 */
void get_find_pos(const pattern_string &val, const size_t offset_str_number, const string *array, const size_t count, vector<pair<size_t, pos_string>> &result)
{
    for (size_t i = 0; i < count; ++i)
    {
        int sr = 0;
        size_t offs = 0;
        const string s = val.get_basic_word(offs); //
        auto found = array[i].find(s);             // определить наличие первого вхождения опрного слова
        if (found != std::string::npos && found >= offs)
        {
            sr = found - offs; //  сместить на начало найденного паттерна
            do
            {
                string forfind = array[i].substr(sr, val.get_len());
                if (val.compare(forfind))
                {
                    auto pr = make_pair(i + offset_str_number,  (pos_string) {(size_t) sr ,forfind }); //   добавить строку в выдачу
                    result.push_back(pr);
                    sr += val.get_len();
                }
                else
                    ++sr;
            } while ((sr + val.get_len()) < array[i].length());
        }
    }
}

/* 
    разделение на потоки поиска по массиву строк  vstr
*/
void proc_strings(const pattern_string &str_pattern, const uint64_t line_number, const vector<string> &vstr , 
    vector<pair<size_t, pos_string>> &result)
{
    // std::thread::id this_id = std::this_thread::get_id();
    const unsigned int n = std::thread::hardware_concurrency();

    //>> цикл расчета нагрузки по потокам
    auto number_str_for_threads = vstr.size() / n;
    size_t str_count = vstr.size();
    const string *str_ptrs = vstr.data();
    static size_t line_offset = 0;
    line_offset = line_number;

    vector<pair<size_t, pos_string>> t_results[n]; //  массив результатов для потоков
    vector<const string * > t_str_ptrs(n);      // массив указателей на начало данных для потоков
    vector<size_t> t_thr_str_cnt(n);                // массив количества обрабатываемых строк   для потоков
    vector<size_t>  t_lines_offset(n);           // массив смещений строк для потоков

    for (size_t th_distr = 0; th_distr < n; ++th_distr)
    {
        size_t thr_str_cnt;
        str_count -= number_str_for_threads;
        thr_str_cnt = number_str_for_threads + str_count * (str_count < number_str_for_threads);

        t_str_ptrs[th_distr] = str_ptrs;
        t_thr_str_cnt[th_distr] = thr_str_cnt;
        t_lines_offset[th_distr] = line_offset;

        str_ptrs += thr_str_cnt;
        line_offset += thr_str_cnt;
    }

    unique_ptr<thread> pthreads[n];
    unique_ptr<pattern_string> ptstrs[n];

    for (size_t t = 0; t < n; ++t)
    {
        ptstrs[t] = make_unique<pattern_string>(str_pattern); //  копии паттерна для пеердачи разным потокам
        pthreads[t] = make_unique<thread>(get_find_pos, (*ptstrs[t].get()), t_lines_offset[t], t_str_ptrs[t], t_thr_str_cnt[t], ref(t_results[t]));
    }

    for (size_t t = 0; t < n; ++t)
        pthreads[t]->join();

    for (size_t p = 0; p < n; p++)
    {
        std::copy(t_results[p].begin(), t_results[p].end(), back_inserter(result));
    }
}

/*
  найти паттерновые строки в файле
 */
void print_pattern_in_file(const pattern_string &ps, string filepath)
{
    const size_t str_cnt = 60000; //  количество строк на поток
    auto t_start = std::chrono::high_resolution_clock::now();
    ifstream fstr;
    fstr.open(filepath);
    if (fstr.is_open())
    {
         vector<pair<size_t, pos_string>> result;

        if (!string_processing_in_file(ps, fstr, str_cnt, proc_strings, result))
        {
            cout << " file is empty ";
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        for (auto it : result)
        {
            cout << it.first << " " << it.second.pos << " " << it.second.s << endl;
        }

        cout << "  elapsed time , ms: " << elapsed_time_ms << endl;
    }
    else
    {
        cout << " file " << filepath << " not found\n";
    }
}



void get_pattern_in_file(const pattern_string &ps, string filepath ,  vector<pair<size_t, pos_string>> &result)
{
    const size_t str_cnt = 60000; //  количество строк на поток
    ifstream fstr;
    fstr.open(filepath);
    if (fstr.is_open())
    {
        if (!string_processing_in_file(ps, fstr, str_cnt, proc_strings, result))
        {
            cout << " file is empty ";
        }
    }
    else
    {
        cout << " file " << filepath << " not found\n";
    }
}

int pos_string::operator==(const pos_string &right) const
{
    return (this->pos == right.pos) && (this->s == right.s);
}