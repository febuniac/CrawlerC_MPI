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
        #include <boost/serialization/vector.hpp>
        #include <boost/serialization/map.hpp>
        #include <boost/mpi.hpp>
        #include <ctime>
        #include <boost/mpi/nonblocking.hpp>
        // All about MPI Scatter and Gather http://mpitutorial.com/tutorials/mpi-scatter-gather-and-allgather/

        using namespace std;
        namespace mpi = boost::mpi;

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

        //Function that receives a vector  of strings and creates a vector with suvectors(batches) of string acording to the size of processes specified by the user.
        void batches(mpi::communicator world,std::vector< string > &vectorz,std::vector<vector<string> >&vectorz_cut_total, int n){
            for (int i = 0; i < vectorz.size(); ++i)//size of the whole string vector
            {
                int resto_tam_batch =i%n; //i é o tamanho total da lista de links e n é o numero de processos
                vectorz_cut_total[resto_tam_batch].push_back(vectorz[i]);//cretaing the new vector of batches
            }

            //Print batches per vector
            for (int i =0; i < vectorz_cut_total.size();i++){
                for (int j =0; j < vectorz_cut_total[i].size() ;j++)
                        {
                            std::cout <<"info: "<<"pos vetor (i) :"<<i<<" tam vetor (j): "<<j<< vectorz_cut_total[i][j]<<'\n';
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
        void download_products_links_LOOP(std::string url,mpi::communicator world,std::vector<vector<string> >&vectorz_cut_total){
            std::string vazio ="";
            std::vector< string > list_link_products;
            
            
            while(url != vazio){//Faz isso para todos os produtos
                std::string html_page = curl_downloadHTML(url);//Coleta página inicial
                
                //download_prods_links_________________________________________________________________________
                std::regex linksprod_reg("<a class=\"card-product-url\" href=\"([^\"]+)\"");
                auto words_begin = std::sregex_iterator(html_page.begin(), html_page.end(), linksprod_reg);
                auto words_end = std::sregex_iterator();
                std::string link_com_site_antes_p = "";
                for (std::sregex_iterator i = words_begin; i != words_end; ++i) {// Enters the Category URL in the specified URL 
                    std::smatch match = *i;
                    std::string match_str_prod = match[1].str();
                    link_com_site_antes_p = "https://www.submarino.com.br" + match_str_prod;
                    list_link_products.push_back(link_com_site_antes_p);//Creates list of links (complete) 
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
            batches(world,list_link_products,vectorz_cut_total, world.size());//divides the complete list of links into a vector uf sub lists of smaller sizes
        }


        //Function that sends batches to processes --> if the btaches are over send empy batches  else send and receive the batches to all running processes
        void envia_recebe(mpi::communicator world,std::vector<vector<string> >&vectorz_cut_total,std::vector< string > &pedacos,std::vector<vector<string> > &vazio_vec)
            {
                if (vectorz_cut_total.size()<=0)//Condição de parada --> manda a lista vazia
                    {
                        mpi::scatter(world,vazio_vec,pedacos, 0);
                    }
                else
                    {
                        mpi::scatter(world,vectorz_cut_total,pedacos, 0); // Manda os batches para todos os processos pedacos é a lista que cada processo ira receber com seu batch especifico
                    }
            }


        //Function that download all HTML from the products lists and puts it in a list
        std::vector< string >  download_HTMLpages_products_LOOP(mpi::communicator world,std::vector< string > &pedacos,std::vector< string > &list_HTML_products){
            for (int i = 0; i <= pedacos.size(); ++i)
                {
                    std::string link_baixado= pedacos[i];
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

        //Function that gets infos of all HTML downloaded from prodcuts
        void get_infos_productHTML_LOOP(std::string url,std::vector< string > &list_HTML_products, std::vector< string > &list_Jsons){
            for (int i = 0; i <= list_HTML_products.size(); ++i)
                {
                    //GET PRODUCT INFO
                    std::string HTMLprod = list_HTML_products[i];
                    
                    std::regex nome_prod_reg ("<h1 class=\"product-name\">([^<]+)</h1>");
                    std::regex descricao_prod_reg ("<div><noframes>((.|\n)+)</noframes><iframe");
                    std::regex foto_prod_reg ("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
                    std::regex preco_a_vista_prod_reg ("<p class=\"sales-price\">([^<]+)</p>");
                    std::regex preco_parcelado_prod_reg ("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
                    std::regex categoria_prod_reg ("<span class=\"TextUI-iw976r-5 grSSAT TextUI-sc-1hrwx40-0 jIxNod\">([^<]+)</span>");
                    
                    auto nome =smatch_regex(HTMLprod,nome_prod_reg);
                    // auto descricao =smatch_regex(HTMLprod,descricao_prod_reg);
                    auto foto =smatch_regex(HTMLprod,foto_prod_reg);
                    auto p_vista =smatch_regex(HTMLprod,preco_a_vista_prod_reg);
                    auto p_parcelado =smatch_regex(HTMLprod,preco_parcelado_prod_reg);
                    auto categoria =smatch_regex(HTMLprod,categoria_prod_reg);
                    std::string saida =
                    "  {\n"
                    "    \"nome\" : \"" + nome +"\",\n"
                    // "    \"descricao\" : \"" + descricao +"\",\n"
                    "    \"foto\" : \"" + foto +"\",\n"
                    "    \"preco\" : \"" + p_vista +"\",\n"
                    "    \"preco_parcelado\" : \"" + p_parcelado +"\",\n"
                    "    \"categoria\" : \"" + categoria +"\",\n"
                    // "    \"url\" : \"" + url +"\",\n"
                    "  },\n";
                    list_Jsons.push_back(saida);
                }    
        }        

        // Function that sends lists of json fron the proceesses running to the master process     
        void envia_master(mpi::communicator world,std::vector< string > &list_Jsons,std::vector<vector<string> >&pedacos_json)
            {
                mpi::gather(world,list_Jsons,pedacos_json, 0);// list_Jsons is the list of Json from each process and  pedacos_json is the type of list_Json is a list of list that receives the list of json and add them together
            }
            

        int main(int argc, char* argv[])
        {
            mpi::environment env(argc, argv);
            mpi::communicator world;

            int num_of_procs = world.size();//Number of processes running

            std::vector<vector<string> > vectorz_cut_total(num_of_procs);//vector with batches of links product
            std::vector< string > pedacos;//vector that each process receives from the big list of lists
            std::vector< string > list_HTML_products;//list of HTMLs
            std::vector< string > list_Jsons; //List of Json from each process
            std::vector<vector<string> > pedacos_json; //vector that each process  wiil send their sub list of jsons 
            std::vector<vector<string> > vazio_vec(num_of_procs);//List of list with nothing inside to crete the stop condition
            std:: string url = "https://www.submarino.com.br/busca/controle-remoto-fisher-price?pfm_carac=controle%20remoto%20fisher%20price&pfm_index=8&pfm_page=search&pfm_type=spectreSuggestions";
            
                if(world.rank()==0)
                {
                    std::string vazio ="";
                    std::vector< string > list_link_products;
            
            
                    while(url != vazio){//Faz isso para todos os produtos
                        std::string html_page = curl_downloadHTML(url);//Coleta página inicial
                        
                        //download_prods_links_________________________________________________________________________
                        std::regex linksprod_reg("<a class=\"card-product-url\" href=\"([^\"]+)\"");
                        auto words_begin = std::sregex_iterator(html_page.begin(), html_page.end(), linksprod_reg);
                        auto words_end = std::sregex_iterator();
                        std::string link_com_site_antes_p = "";
                        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {// Enters the Category URL in the specified URL 
                            std::smatch match = *i;
                            std::string match_str_prod = match[1].str();
                            link_com_site_antes_p = "https://www.submarino.com.br" + match_str_prod;
                            list_link_products.push_back(link_com_site_antes_p);//Creates list of links (complete) 
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
                        for (std::sregex_iterator i = words_begin2; i != words_end2; ++i) 
                            {
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
                        for (int i = 0; i < list_link_products.size(); ++i)//size of the whole string vector
                            {
                                int resto_tam_batch =i%world.size(); //i é o tamanho total da lista de links e n é o numero de processos
                                vectorz_cut_total[resto_tam_batch].push_back(list_link_products[i]);//cretaing the new vector of batches
                            }
                    }
                    if (vectorz_cut_total.size()<=0)//Condição de parada --> manda a lista vazia
                        {
                            mpi::scatter(world,vazio_vec,pedacos, 0);
                        }
                    else
                        {
                            mpi::scatter(world,vectorz_cut_total,pedacos, 0); // Manda os batches para todos os processos pedacos é a lista que cada processo ira receber com seu batch especifico
                            std::cout<< "enviei para: "<< world.rank()<< '\n';
                        }

                    if (pedacos.size()<=0)// If there are links still
                    {
                        std::cout<<world.rank()<< "parei";
                        return 0;//break
                       
                    }
                    else
                    {
                         //std::cout<< "estou rodando o processo:"<< world.rank();
                        for (int i = 0; i < pedacos.size(); ++i)
                            {
                                std::cout<< "estou rodando o processo:"<< world.rank()<<'\n';
                                std::cout <<"Tamanho do pedaço"<<pedacos.size() << "estou nesta posição do pedaço"<<i<<'\n';
                                std::string link_baixado= pedacos[i];
                                std::string html_page_prod = curl_downloadHTML(link_baixado);
                                list_HTML_products.push_back(html_page_prod);
                               
                            }
                      
                        for (int i = 0; i < pedacos.size(); ++i)
                            {
                               
                                //GET PRODUCT INFO
                                std::string HTMLprod = list_HTML_products[i];
                                
                                std::regex nome_prod_reg ("<h1 class=\"product-name\">([^<]+)</h1>");
                                std::regex descricao_prod_reg ("<div><noframes>((.|\n)+)</noframes><iframe");
                                std::regex foto_prod_reg ("<img class=\"swiper-slide-img\" alt=\"(.+)\" src=\"([^\"]+)\"");
                                std::regex preco_a_vista_prod_reg ("<p class=\"sales-price\">([^<]+)</p>");
                                std::regex preco_parcelado_prod_reg ("<p class=\"payment-option payment-option-rate\">([^<]+)</p>");
                                std::regex categoria_prod_reg ("<span class=\"TextUI-iw976r-5 grSSAT TextUI-sc-1hrwx40-0 jIxNod\">([^<]+)</span>");
                                
                                //std::cout<<"nome pre  "<<world.rank()<<'\n';
                                auto nome =smatch_regex(HTMLprod,nome_prod_reg);
                                //std::cout<<"nome done  "<<world.rank()<<'\n';
                                //auto descricao =smatch_regex(HTMLprod,descricao_prod_reg);
                                
                                auto foto =smatch_regex(HTMLprod,foto_prod_reg);
                                //std::cout<<"foto done  "<<world.rank()<<'\n';
                                auto p_vista =smatch_regex(HTMLprod,preco_a_vista_prod_reg);
                                //std::cout<<"vista done  "<<world.rank()<<'\n';
                                auto p_parcelado =smatch_regex(HTMLprod,preco_parcelado_prod_reg);
                                //std::cout<<"parcela done  "<<world.rank()<<'\n';
                                auto categoria =smatch_regex(HTMLprod,categoria_prod_reg);
                                //std::cout<<"categoria done  "<<world.rank()<<'\n';
                                std::string saida =
                                "  {\n"
                                "    \"nome\" : \"" + nome +"\",\n"
                                //"    \"descricao\" : \"" + descricao +"\",\n"
                                "    \"foto\" : \"" + foto +"\",\n"
                                "    \"preco\" : \"" + p_vista +"\",\n"
                                "    \"preco_parcelado\" : \"" + p_parcelado +"\",\n"
                                "    \"categoria\" : \"" + categoria +"\",\n"
                                // "    \"url\" : \"" + url +"\",\n"
                                "  },\n";
                                list_Jsons.push_back(saida);
                                std::cout<< "Eu sou o processo:" <<world.rank()<<'\n';

                                std::cout<< "O tamnho da minha lista  de JSON é  :" <<list_Jsons.size()<<'\n';

                            }    
                    }
                    //returning evrything to master process
                    mpi::gather(world,list_Jsons,pedacos_json, 0);
                        for (int i =0; i < pedacos_json.size();i++){
                        for (int j =0; j < pedacos_json[i].size() ;j++)
                            {
                                std::cout << pedacos_json[i][j];
                            }
                    }
            return 0;
        }

        //COMPILE:
        //mpicxx crawler_mpi2.cpp -o crawler_mpi2 -lboost_mpi -lcurl -lboost_serialization -std=c++11
        // mpiexec -n 4 ./crawler_mpi2


