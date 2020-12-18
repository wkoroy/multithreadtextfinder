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

// g++  -o ./task1    ./task1.cpp    -std=c++14 -O3 -lpthread
using namespace std;


/**
 * класс сравнения паттерна
*/
class pattern_string
{
public:
    pattern_string(const string &ptrn)
    {
        pattern = ptrn;
        len = pattern.length();

        bool startwr = false;
        bool endwr = false;
        basic_word_offset = 0;
        
        pair <size_t , pair<size_t, string>> word;
        word.first = 0;


        // определение индексов для сравнения
        //  и выбор опорного слова -наибольшего по длине
        for (size_t ic = 0; ic < pattern.length(); ic++)
        {
            char c = pattern[ic];
            if (c != '?')
            {
                compared_index.push_back(ic);
                startwr =true;
                if(!basic_word_offset) basic_word_offset = ic;
            } 
            else
            {
                if(startwr) endwr = true;
            }

            if(startwr && !endwr)
            {
                basic_word += c;
            }
            else
            {
                //   выбрать слово максимальной длины
                if (basic_word.length() > word.first)
                {
                    cout << basic_word.length() << "  " << basic_word_offset << " - - |" << basic_word << "|" << endl;
                    word = make_pair(basic_word.length() , make_pair(basic_word_offset, basic_word));
                }

                startwr = false;
                endwr = false;
                
                basic_word = "";
                basic_word_offset = 0;
            }
        }

        // если срока не кончается [?] , то собрать накопившиеся символы и вычислить опорное слово
         if(startwr && !endwr)
         {
             if (basic_word.length() > word.first)
                {
                    cout << basic_word.length() << "  " << basic_word_offset << " - - |" << basic_word << "|" << endl;
                    word = make_pair(basic_word.length() , make_pair(basic_word_offset, basic_word));
                }
         }
        basic_word = word.second.second;
        basic_word_offset =   word.second.first;
    }

    bool compare(const string &str) const
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

    size_t get_len() const
    {
        return len;
    }

    string get_basic_word(size_t &offset)const
    {
        offset = basic_word_offset;
        return basic_word;
    }

private:
    vector<size_t> compared_index; // индексы значащих ячеек  паттерна
    string pattern; 
    size_t len; // длина паттерна
    string basic_word; // опорное слово паттерна
    size_t basic_word_offset; // смешение опорного слова
};

/**
 * создать случайную строку с  фиксированным алфавитом и заданной длиной
*/
std::string random_string(std::size_t length)
{
    const std::string CHARACTERS = " ABCDE FGHIJKL MNOPQRS TUVWXYZabc de_fghijklmn opqrstuv wxyz";

    static std::random_device random_device;
    static std::mt19937 generator(random_device());
    static std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}


/**
 * сгенерировать файл для поиска
*/
void gen_file()
{
    ofstream strfile;
    strfile.open("./strfile.txt");
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(100, 500);

    const int line_count = 10050000;
    vector<size_t> str_words{500, 900, 4800,4800*4 ,4800*5 , 4800*12,4800/17 , line_count - 2};
    if (strfile.is_open())
    {
        string fnded_tword = "sde main";
        for (size_t i = 0; i < line_count; i++)
        {
            string ins_words = fnded_tword;

            if (count(str_words.begin(), str_words.end(), i) == 0)
                ins_words = "";
            else
            {
                cout << " i " << i << "  " << count(str_words.begin(), str_words.end(), i) << endl;
            }
            strfile << random_string(distribution(generator)) << ins_words << random_string(distribution(generator)) << endl;
        }
    }

    cout << " gen ok \n";
    strfile.close();
}

/**
 * сбор массивов строк для последующей пердачи на обработку
*/
bool string_processing_in_file(const pattern_string &str_pattern, ifstream &file, size_t limit, std::function<void(const pattern_string &ptrn, uint64_t line_number, const vector<string> &vstr)> proc_strings)
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
            thwaiter = async(std::launch::async, proc_strings, str_pattern, (line_counter - strings[switch_vectors].size() - is_last_data), ref(strings[switch_vectors]));
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
void get_find_pos(const pattern_string &val, const size_t offset_str_number, const string *array, const size_t count, vector<pair<size_t, string>> &result)
{
    for (size_t i = 0; i < count; ++i)
    {
        int sr = 0;
        size_t offs = 0;
        const string s = val.get_basic_word(offs); //
        auto found = array[i].find(s);   // определить наличие первого вхождения опрного слова
        if (found != std::string::npos && found >= offs)
        {
            sr = found - offs; //  сместить на начало найденного паттерна
            do
            {
                string forfind = array[i].substr(sr, val.get_len());
                if (val.compare(forfind))
                {
                    auto pr = make_pair(i + offset_str_number, to_string(sr) + " [" + forfind + "]"); //   добавить строку в выдачу
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
void proc_strings(const pattern_string &str_pattern, const uint64_t line_number, const vector<string> &vstr)
{
    // std::thread::id this_id = std::this_thread::get_id();
    const unsigned int n = std::thread::hardware_concurrency();

    //>> цикл расчета нагрузки по потокам
    auto number_str_for_threads = vstr.size() / n;
    size_t str_count = vstr.size();
    const string *str_ptrs = vstr.data();
    static size_t line_offset = 0;
    line_offset = line_number;

    vector<pair<size_t, string>> t_results[n]; //  массив результатов для потоков
    const string *t_str_ptrs[n]{nullptr};      // массив указателей на начало данных для потоков
    size_t t_thr_str_cnt[n]{0};                // массив количества обрабатываемых строк   для потоков
    size_t t_lines_offset[n]{0};               // массив смещений строк для потоков

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

    for (auto v : t_results)
    {
        for (auto it : v)
        {
            cout << it.first << " " << it.second << endl;
        }
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
        if (!string_processing_in_file(ps, fstr, str_cnt, proc_strings))
        {
            cout << " file is empty ";
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        cout << "  elapsed time , ms: " << elapsed_time_ms << endl;
    }
    else
    {
        cout<< " file "<<filepath<<" not found\n";
    }
    
}



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

#if 0


 pattern_string str_pattern("??sde main??") ;

    const size_t str_cnt = 60000;
    auto t_start = std::chrono::high_resolution_clock::now();
    ifstream fstr;
    fstr.open("./strfile.txt");
    if (fstr.is_open())
    {
       if(! string_processing_in_file(str_pattern, fstr, str_cnt, proc_strings) )
       {
            cout<<" file is empty ";
       }
    }
    auto t_end = std::chrono::high_resolution_clock::now();

    double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    cout  << "  elapsed_time_ms " << elapsed_time_ms << endl;


    for (size_t i = 0; i < 100; i++)
    {
        const size_t str_cnt = i*1000;
        auto t_start = std::chrono::high_resolution_clock::now();
        ifstream fstr;
        fstr.open("./strfile.txt");
        if (fstr.is_open())
        {
            find_str_in_file(str_pattern ,fstr, str_cnt, proc_strings);
        }
        auto t_end = std::chrono::high_resolution_clock::now();

        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        cout << str_cnt << "  elapsed_time_ms " << elapsed_time_ms << endl;
    }
#endif
}


