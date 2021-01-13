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
#include <string.h>
#include "findtext.h"

// g++  -o ./task1    ./task1.cpp    -std=c++14 -O3 -lpthread
using namespace std;

using  ofsset_and_str   = pair<size_t,string> ;
using num_offset_str  = pair<size_t ,ofsset_and_str >;


 // деление на строки с <перекрытием>
vector<ofsset_and_str>  div_str_overlap(const string &s,const int64_t  max_sz ,const size_t overlap )
{

    vector<ofsset_and_str> vos;
     int64_t  slen = s.length();
     int64_t left  =  slen - max_sz;
     if(left > 0)
     {
        int64_t lcnt = slen/max_sz;
        int64_t clen = slen;
         // не вместилось
         for(size_t i=0;i<lcnt;i++)
         {
             clen -=max_sz;
             size_t p_overlap = overlap *( ((s.length()-i*max_sz) - max_sz) >= overlap);
             vos.emplace_back(make_pair(i*max_sz,s.substr(i*max_sz , max_sz +p_overlap)));
         }
         if(clen > 0) 
         {
              vos.emplace_back(make_pair(slen-clen,s.substr( slen-clen  , clen)));
         }
     }
     else
     {
          // вместилось
          vos.emplace_back(make_pair(0,s));
     }
     return vos;
}
vector<ofsset_and_str> div_str( string s, int64_t  max_sz )
{

    vector<ofsset_and_str> vos;
     int64_t  slen = s.length();
     int64_t left  =  slen - max_sz;
     if(left > 0)
     {
        int64_t lcnt = slen/max_sz;
        int64_t clen = slen;
         // не вместилось
         for(size_t i=0;i<lcnt;i++)
         {
             clen -=max_sz;
             vos.emplace_back(make_pair(i*max_sz,s.substr(i*max_sz , max_sz)));
         }
         if(clen > 0) 
         {
              vos.emplace_back(make_pair(slen-clen,s.substr( slen-clen  , clen)));
         }
     }
     else
     {
          // вместилось
          vos.emplace_back(make_pair(0,s));
     }
     return vos;
}

 using func_consumer =  std::function<void(  vector<num_offset_str> &lines)>;

void distrib_string(ifstream &file , func_consumer consumer ,  size_t overlap = 4  )
{

    vector<num_offset_str> lines;
    uint64_t linenum = 0;
    size_t accum_sz = 0;
    constexpr size_t pcache_sz = 8;// 3072*1000;
    string str;
    bool read_res;
    while( !(read_res =std::getline(file, str).eof()) )
    {
        auto slen = str.length();
         //cout<<"___"<<slen<<endl;;
        if( (accum_sz+slen) < pcache_sz )
        {
             accum_sz+= slen;
             lines.emplace_back( linenum , move(make_pair(0,move(str)) ));
        }
        else
        {
            int64_t lef_to_full = (int64_t)pcache_sz -(int64_t) accum_sz;
            if(lef_to_full > 0) //  дополнить недостающее
            {
                 // подсчет перекрытия
                 size_t offs = overlap * (str.length()  > (lef_to_full+overlap) ) -1*(overlap>1);
                 lines.emplace_back( linenum , move(make_pair(0,str.substr(0,lef_to_full+offs) ) ));
            }

            consumer(lines);
            lines.clear();

            auto line_parts = div_str_overlap( str.substr(lef_to_full) , pcache_sz , overlap );

            for(auto i = line_parts.begin();i!=line_parts.end();++i)
            {
                (*i).first +=lef_to_full; 
                lines.emplace_back(linenum , move(*i));
            }

             consumer(lines);
            accum_sz = 0;

            //TODO массив lines  готов


           // cout<<" ------------finish " <<" "<< linenum <<" "<<  accum_sz<<endl;

        }
        
       
        linenum++;
    }

    size_t cnt = accum_sz / pcache_sz;
    for (size_t i = 0; i < cnt; i++)
    {
        cout << " dv " << pcache_sz << endl;
        accum_sz -= pcache_sz;
    }
}


void distributor_func(  vector<num_offset_str> &lines)
{
    cout<<"--------\n";
    size_t count_bytes = 0;
   for(auto a:lines)
   {
       cout<<a.first<<" N "<<a.second.first <<"|"<< a.second.second<<endl;
       count_bytes += a.second.second.size();
   }
    cout<<"["<<count_bytes <<"]"<<endl;
   
}
int main(int argc, char **argv)
{

 #if 0   
   string  str1 = "";
   for(char a='a';a<='z';++a)
   {
       str1+=a;
   }
  auto v =  div_str_overlap(str1, 7 ,3);

  for(auto &i:v)
  {
      cout<<i.first <<" "<<i.second<<endl;
  }

   return 0;
////////////////////
    ofstream file("./test.txt");
    if(file.is_open())
    {
        char str[1024];
        for (size_t a = 0; a < sizeof(str); ++a)
        {
            str[a]= (char)a+'a';
        }
        str[sizeof(str)-1]= '\0';
        file<<str<<endl;
        file<<str<<endl;
        file<<str<<endl;
        for(size_t c=0;c<5*1024;++c)
        {
            file<<str;
        }
        file<<str<<endl;
 

        for(size_t c=0;c<100*1024;++c)
        {
            file<<str;
        }
        file<<str<<endl;


        for(size_t c=0;c<120*1024;++c)
        {
            file<<str<<endl;
        }

        for(size_t c=0;c<5*1024;++c)
        {
            file<<str;
        }
        file<<str<<endl;

        file<<str<<endl;
        file.close();
    }
#endif


    ifstream f;
    f.open("./test2.txt");
    distrib_string(f ,distributor_func, 4);
    return 0;
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


