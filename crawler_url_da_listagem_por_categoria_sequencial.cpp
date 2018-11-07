#include <iostream>
#include <string>
#include <curl/curl.h>
#include <fstream>
using namespace std;
#include <stdio.h>
#include <regex>
#include <vector>
#include <iterator>
#include <thread>
#include <chrono>
#include <list>



// Convert curl output into a string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// writing a file with program info (HTML page)
void write_file(string info, string filename){
    ofstream myfile;
    myfile.open (filename);
    myfile << info;
    myfile.close();
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

std::vector< string > download_products_links_LOOP(std::string url){
    std::string vazio ="";
    std::vector< string > list_link_products;
    while(url != vazio){
        std::string html_page = curl_downloadHTML(url);
        
        //download_prods_links_________________________________________________________________________
        std::regex linksprod_reg("<a class=\"card-product-url\" href=\"([^\"]+)\"");
        auto words_begin = std::sregex_iterator(html_page.begin(), html_page.end(), linksprod_reg);
        auto words_end = std::sregex_iterator();
        std::string link_com_site_antes_p = "";
        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string match_str_prod = match[1].str();
            link_com_site_antes_p = "https://www.submarino.com.br" + match_str_prod;
            list_link_products.push_back(link_com_site_antes_p);
            // std::cout<<list_link_products.front()<<'\n'<<'\n';
        }
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
    return list_link_products;
}

std::vector< string >  download_HTMLpages_products_LOOP(std::string url){
    std::vector< string > list_HTML_products;
    std::vector< string > list_link_products = download_products_links_LOOP(url);
    
    for (int i = 0; i < list_link_products.size(); ++i){
        std::string link_baixado= list_link_products[i];
        std::string html_page_prod = curl_downloadHTML(link_baixado);
        list_HTML_products.push_back(html_page_prod);
    }
    return list_HTML_products;
}

std::string smatch_regex(std::string link_produto,std::regex reg){
    smatch match;
    if (regex_search(link_produto, match, reg) == true) {
        // cout << "INFOS : " << match[1].str() << endl;
    }
    return match[1].str();
}

void get_infos_productHTML_LOOP(std::string url){
    std::vector< string > list_HTML_products = download_HTMLpages_products_LOOP(url);
    for (int i = 0; i < list_HTML_products.size(); ++i){
        //GET PRODUCT INFO
        std::string HTMLprod = list_HTML_products[i];
        
        std::regex nome_prod_reg ("<h1 class=\"product-name\">([^<]+)</h1>");
        std::regex descricao_prod_reg ("<div><noframes>((.|\n)+)</noframes><iframe");
        std::regex foto_prod_reg ("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
        std::regex preco_a_vista_prod_reg ("<p class=\"sales-price\">([^<]+)</p>");
        std::regex preco_parcelado_prod_reg ("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
        std::regex categoria_prod_reg ("<span class=\"TextUI-iw976r-5 grSSAT TextUI-sc-1hrwx40-0 jIxNod\">([^<]+)</span>");
        
        auto nome =smatch_regex(HTMLprod,nome_prod_reg);
        auto descricao =smatch_regex(HTMLprod,descricao_prod_reg);
        auto foto =smatch_regex(HTMLprod,foto_prod_reg);
        auto p_vista =smatch_regex(HTMLprod,preco_a_vista_prod_reg);
        auto p_parcelado =smatch_regex(HTMLprod,preco_parcelado_prod_reg);
        auto categoria =smatch_regex(HTMLprod,categoria_prod_reg);
        std::string saida =
        "  {\n"
        "    \"nome\" : \"" + nome +"\",\n"
        "    \"descricao\" : \"" + descricao +"\",\n"
        "    \"foto\" : \"" + foto +"\",\n"
        "    \"preco\" : \"" + p_vista +"\",\n"
        "    \"preco_parcelado\" : \"" + p_parcelado +"\",\n"
        "    \"categoria\" : \"" + categoria +"\",\n"
        // "    \"url\" : \"" + url +"\",\n"
        "  },\n";

        cout<< saida;
    }
    
    
    // }
    
}


int main(void)
{
    std:: string url = "https://www.submarino.com.br/busca/controle-remoto-fisher-price?pfm_carac=controle%20remoto%20fisher%20price&pfm_index=8&pfm_page=search&pfm_type=spectreSuggestions";
    std::chrono::  high_resolution_clock::time_point t1, t2, t3,t4,t5,t6,t7,t8;
    std::chrono:: duration<double> tempoProduto;
    std::chrono:: duration<double> tempoTotalOcioso;
    std::chrono:: duration<double> tempoTotalCrawler;
    double tempoProduto1;
    double tempoTotalCrawler1;
    double tempoTotalOcioso1;
    
   
    std::string html_page = curl_downloadHTML(url);
    std::string novo_html = download_products_links_LOOP(url)[0];
    t1 = std::chrono::high_resolution_clock::now();
    std::string html_page2 = curl_downloadHTML(novo_html);
    //GET PRODUCT INFO
    
    std::regex nome_prod_reg ("<h1 class=\"product-name\">([^<]+)</h1>");
    std::regex descricao_prod_reg ("<div><noframes>((.|\n)+)</noframes><iframe");
    std::regex foto_prod_reg ("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
    std::regex preco_a_vista_prod_reg ("<p class=\"sales-price\">([^<]+)</p>");
    std::regex preco_parcelado_prod_reg ("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
    std::regex categoria_prod_reg ("<span class=\"TextUI-iw976r-5 grSSAT TextUI-sc-1hrwx40-0 jIxNod\">([^<]+)</span>");
    
    auto nome =smatch_regex(html_page2,nome_prod_reg);
    auto descricao =smatch_regex(html_page2,descricao_prod_reg);
    auto foto =smatch_regex(html_page2,foto_prod_reg);
    auto p_vista =smatch_regex(html_page2,preco_a_vista_prod_reg);
    auto p_parcelado =smatch_regex(html_page2,preco_parcelado_prod_reg);
    auto categoria =smatch_regex(html_page2,categoria_prod_reg);
    std::string saida =
    "  {\n"
    "    \"nome\" : \"" + nome +"\",\n"
    "    \"descricao\" : \"" + descricao +"\",\n"
    "    \"foto\" : \"" + foto +"\",\n"
    "    \"preco\" : \"" + p_vista +"\",\n"
    "    \"preco_parcelado\" : \"" + p_parcelado +"\",\n"
    "    \"categoria\" : \"" + categoria +"\",\n"
    // "    \"url\" : \"" + url +"\",\n"
    "  },\n";
    cout<< saida;
    t2 = std::chrono::high_resolution_clock::now();
    tempoProduto = std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1);
    tempoProduto1 = tempoProduto.count();
    cout << "Tempo total de um produto: " << tempoProduto1 << '\n';
    
    
    
    
    
    t3 = std::chrono::high_resolution_clock::now();
    std::vector <string> lista_prods =download_products_links_LOOP(url);
    t5 = std::chrono::high_resolution_clock::now();
    download_HTMLpages_products_LOOP(url);
    t6 = std::chrono::high_resolution_clock::now();
    get_infos_productHTML_LOOP(url);
    t4 = std::chrono::high_resolution_clock::now();
    tempoTotalCrawler = std::chrono::duration_cast<std::chrono::duration<double> >(t4 - t3);
    tempoTotalOcioso = std::chrono::duration_cast<std::chrono::duration<double> >(t6 - t5);
    tempoTotalOcioso1 = tempoTotalOcioso.count();
    tempoTotalCrawler1 = tempoTotalCrawler.count();
    cout << "Tempo total de um produto: " << tempoProduto1 << '\n';
    cout << "Tempo total de Execucão do Crawler: " << tempoTotalCrawler1 << '\n';
    cout << "Tempo total de Ociosidade do Crawler: " << tempoTotalOcioso1 << '\n';
    float tempMedio =tempoTotalCrawler1/lista_prods.size();
    cout << "Tempo Medio dos Produtos: " << tempMedio << '\n';
    
    //std:: string url = "https://www.submarino.com.br/busca/controle-remoto-fisher-price?pfm_carac=controle%20remoto%20fisher%20price&pfm_index=8&pfm_page=search&pfm_type=spectreSuggestions";
    // download_products_links_LOOP(url);
    // download_HTMLpages_products_LOOP(url);
    // get_infos_productHTML_LOOP(url);
    return 0;
}
