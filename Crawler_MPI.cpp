#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <iostream>
#include <math.h> 
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <stdio.h>
#include <regex>
#include <vector>
#include <iterator>
#include <thread>
#include <chrono>
#include <list>
#include <boost/serialization/string.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/serialization/map.hpp>

using namespace std;
namespace mpi = boost::mpi;


// int square(int i){
//     return pow(i,2);
// }

//function that prints vectors
template<typename T>
void print_vector(std::vector<T> const &v)
{
    for (auto &i: v)
        std::cout << i << ' ';

    std::cout << '\n';
}

// writing a file with program info (HTML page)
void write_file(string info, string filename){
    ofstream myfile;
    myfile.open (filename);
    myfile << info;
    myfile.close();
}

// // Divide a lista de produtos em batches 
// std::vector< string > batches(std::vector< string > vectorz, int n, mpi::communicator world){// input vector of integers, split vector into sub-vectors each of size n
// //     // determine number of sub-vectors of size n
// int size = world.size();
//     //  int size = (vectorz.size() - 1) / n + 1;

//     // create array of vectors to store the sub-vectors
//     std::vector<string> vec[size];

//     // each iteration of this loop process next set of n elements
//     // and store it in a vector at k'th index in vec
//     for (int k = 0; k < size; ++k)
//     {
//         // get range for next set of n elements
//         auto start_itr = std::next(vectorz.cbegin(), k*n);
//         auto end_itr = std::next(vectorz.cbegin(), k*n + n);

//         // allocate memory for the sub-vector
//         vec[k].resize(n);

//         // code to handle the last sub-vector as it might
//         // contain less elements
//         if (k*n + n > vectorz.size()) {
//             end_itr = vectorz.cend();
//             vec[k].resize(vectorz.size() - k*n);
//         }

//         // copy elements from the input range to the sub-vector
//         std::copy(start_itr, end_itr, vec[k].begin());
//     }

//     // print the sub-vectors
//     for (int i = 0; i < size; i++) {
//         std::cout<<vectorz.size()<<'/n';
//         print_vector(vec[i]);
//     }
// }
void batches(mpi::communicator world,std::vector< string > &vectorz,std::vector<vector<string> >&vectorz_cut_total, int n){
// int tam_batch = vectorz.size()/n;


std::vector< string > vectorz_cut;

for (int i = 0; i < vectorz.size(); ++i)
{
    int resto_tam_batch =i%n; //i é oi tamanho total da lista de links e n é o numero de processos
        vectorz_cut_total[resto_tam_batch].push_back(vectorz[i]);
        std::cout <<"size:"<< vectorz_cut_total.size()<<'\n';  
}
for (int i =0; i < vectorz_cut_total.size();i++){
    for (int j =0; j < vectorz_cut_total[i].size() ;j++)
            {
                std::cout <<"info:"<< vectorz_cut_total[i][j]<<'\n';
            }
}
}


// Convert curl output into a string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

//receives as input an URL and makes the download of the HTMl code of the page
string curl_downloadHTML(std::string url){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    std::string header_string;
    curl = curl_easy_init();
    
    if(curl) {
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
        
        res = curl_easy_perform(curl);
        
        curl_easy_cleanup(curl);
        
        write_file(readBuffer,"mycurlfile_parallel.txt");
    }
    
    return readBuffer;
}
// Processo 0 (pt1): (master)
std::vector< string > download_products_links_LOOP(std::string url,mpi::communicator world,std::vector<vector<string> >&vectorz_cut_total){
    std::string vazio ="";
    std::vector< string > list_link_products;
    std::vector< string > pedacos;
    while(url != vazio){//Faz isso para todos os produtos
        std::string html_page = curl_downloadHTML(url);//Coleta página inicial
        
        //download_prods_links_________________________________________________________________________
        std::regex linksprod_reg("<a class=\"card-product-url\" href=\"([^\"]+)\"");
        auto words_begin = std::sregex_iterator(html_page.begin(), html_page.end(), linksprod_reg);
        auto words_end = std::sregex_iterator();
        std::string link_com_site_antes_p = "";
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {//Entra no URL de Categoria no seu website 
            std::smatch match = *i;
            std::string match_str_prod = match[1].str();
            link_com_site_antes_p = "https://www.submarino.com.br" + match_str_prod;
            list_link_products.push_back(link_com_site_antes_p);//Joga na Queue1 
            

        }       
        
        //mpi::scatter(world,vectorz_cut_total,pedacos, 0);
        // for (int i = 0; i < world.size(); i++){
           
        // }

        //_____________________________________________________________________________________________
        
        //download_next_page_________________________________________________________________________
        std::vector< string > list_link_nexts;
        CURL *curl;
        std::regex linkspages_reg("<li class=\"\"><a href=\"([^<]+)\"><span aria-label=\"Next\">");
        auto words_begin2 = std::sregex_iterator(html_page.begin(), html_page.end(), linkspages_reg);
        auto words_end2 = std::sregex_iterator();
        std::string match_str_next;
        std::string link_com_site_antes_n = "";
        for (std::sregex_iterator i = words_begin2; i != words_end2; ++i) {
            std::smatch match = *i;
            std::string match_str_next = match[1].str();
            link_com_site_antes_n = "https://www.submarino.com.br" + match_str_next;
            std::regex amp("amp;");
            link_com_site_antes_n = std::regex_replace(link_com_site_antes_n, amp, "");
            list_link_nexts.push_back(link_com_site_antes_n);
        }
        url = link_com_site_antes_n;
        
        //_____________________________________________________________________________________________
    }
    batches(world,list_link_products,vectorz_cut_total, world.size());
    return list_link_products;
}




int main(int argc, char* argv[])
{
    mpi::environment env(argc, argv);
    mpi::communicator world;
    int x = world.size();

    // if(world.rank()==0){
    //     int data;
    //     world.send(world.rank()+1, 0, square(world.rank()));
    //     world.recv(x-1, 0, data);
    //     std::cout << "Received " << data << " from " << x-1 << "\n";
    // }
    // else if(world.rank()== x-1){
    //     int data;
    //     world.send(0, 0, square(world.rank()));
    //     world.recv(world.rank()-1, 0, data);
    //     std::cout << "Received " << data << " from " << world.rank()-1 << "\n";
    // }
    // else{
    //     int data;
    //     world.send(world.rank()+1, 0, square(world.rank()));
    //     world.recv(world.rank()-1, 0, data);
    //     std::cout << "Received " << data << " from " << world.rank()-1 << "\n";
    // }
    std::vector<vector<string> > vectorz_cut_total(x);
    std:: string url = "https://www.submarino.com.br/busca/controle-remoto-fisher-price?pfm_carac=controle%20remoto%20fisher%20price&pfm_index=8&pfm_page=search&pfm_type=spectreSuggestions";
    if(world.rank()==0){
    download_products_links_LOOP(url,world,vectorz_cut_total);
    }

    
    return 0;
}

//COMPILE:
//mpicxx Crawler_MPI.cpp -o Crawler_MPI -lboost_mpi -lcurl -lboost_serialization -std=c++11
//mpiexec ./Crawler_MPI
//O número de processos usados é controlado pela flag -n. Para executar o programa CrawlerMPI com 16 processos
//mpiexec -n 16 ./Crawler_MPI