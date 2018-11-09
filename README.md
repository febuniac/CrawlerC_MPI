# CRAWLER MPI

## Introdução

O objetivo deste projeto é trabalhar com download e análise de páginas web utilizando processos. O projeto consiste na criação de um crawler que identifica páginas de produtos em um site de e-commerce e extrai as informações básicas dos produtos. 
O crawling de páginas é uma etapa do sistemas de comparação de preços de sites. Após esta extração estes sistemas cruzam as informações dos sites de modo a identificar quais produtos estão presentes em ambos e fazer as páginas de comparação de preços. A parte inicial do projeto consiste em criar um crawler que coleta essas informações de forma sequencial e depois transformar este em um crawler que trabalha com diferentes processos em diversas máquinas, tornando-o mais rápido.





## Compilação

Para mudar o url de categoria do site mude a variável url no código.


Compilação:

primeiro comando: cmake .

segundo comando:make

Segunda opção para compilação:
comando:mpicxx Crawler_MPI.cpp -o Crawler_MPI -lboost_mpi -lcurl -lboost_serialization -std=c++11
## Execução

mpiexec -n numero de processos ./Crawler_MPI

O numero de processos é um valor inteiro

Para rodar de forma sequencial:

mpiexec -n 1 ./Crawler_MPI
