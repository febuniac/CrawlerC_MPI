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
                download_products_links_LOOP(url,world,vectorz_cut_total);// Only the master process can download the links, put them in a big list of links and then create a vector of vectors rthat contain the batches according to the number of process
                }
                envia_recebe(world,vectorz_cut_total,pedacos,vazio_vec);// the batches are sent to all the running processes including the master (scatter)
                if (pedacos.size()>0)// If there are links still
                {
                    std::cout<<world.rank()<< "estou rodando";
                    download_HTMLpages_products_LOOP(world,pedacos,list_HTML_products);
                    get_infos_productHTML_LOOP(url,list_HTML_products,list_Jsons);
                }
                else
                {
                    std::cout<<world.rank()<< "parei";
                    return 0;//break
                }
                //returning evrything to master process
                envia_master(world,list_Jsons,pedacos_json);//send the jsons back to a vector in the master

                for (int i =0; i < pedacos_json.size();i++){
                    for (int j =0; j < pedacos_json[i].size() ;j++)
                        {
                            std::cout << pedacos_json[i][j];
                        }
                }
            return 0;
        }

        //COMPILE:
        //mpicxx Crawler_MPI.cpp -o Crawler_MPI -lboost_mpi -lcurl -lboost_serialization -std=c++11
        //mpiexec ./Crawler_MPI
        //O número de processos usados é controlado pela flag -n. Para executar o programa CrawlerMPI com 16 processos
        //mpiexec -n 16 ./Crawler_MPI



















        //mpicxx -o exemplo1 exemplo1.cpp -lboost_mpi -lboost_serialization -std=c++11
        //mpiexec exemplo1
        //https://theboostcpplibraries.com/boost.mpi-asynchronous-data-exchange
        // int main(int argc, char *argv[]) {

            
        //     if (world.rank() == 0) {
        //         mpi::request reqs[2];
        //         std::string s[2];
        // //        auto r1 = world.irecv(1, 10, s[0]);
        // //        auto r2 = world.irecv(2, 20, s[1]);
                
        //         reqs[0] = world.irecv(1, 10, s[0]);
        //         reqs[1] = world.irecv(2, 20, s[1]);
        //         auto  resposta1 =boost::mpi::wait_any(reqs, reqs + 2);//ponteiro para o primeiro e ultimo item do vetor( reqs + 2 ultimo cara do vetor)
        //         std::cout <<  s[(resposta1.second - reqs)] << "; ";
        //         auto  resposta2 = boost::mpi::wait_any(reqs, reqs + 2);
        //         std::cout <<  s[(resposta2.second - reqs)] << "; ";
        // //        reqs[0].wait();
        // //        std::cout << s[0] << "\n" << std::endl;
        // //
        // //        reqs[1].wait();
        // //        std::cout << s[1] << '\n';


        //     } else if (world.rank() == 1) {
        //         std::string s = "Hello, world!";

        //         world.send(0, 10, s);
        //         std::cout << "Fim rank 1 " << std::endl;
        //     } else if (world.rank() == 2) {
        //         std::string s = "Hello, moon!";

        //         world.send(0, 20, s); 
        //         std::cout << "Fim rank 2 " << std::endl;
        //     }
        // }




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


            //returning evrything to master process
            // for (int y =0 ; y< world.size(); ++y)
            // {
            //     envia_master(world,list_Jsons,pedacos_json);//send the jsons back to a vector in the master
                
            //     //Print
            //     // for (int i =0; i < list_Jsons.size();i++){
            //     //     std::cout<< list_Jsons[i];
            //     // }
                
            //     for (int i =0; i < pedacos_json.size();i++){
            //         for (int j =0; j < pedacos_json[i].size() ;j++)
            //             {
            //                 std::cout << pedacos_json[i][j];
            //             }
            //     }
                
            // }