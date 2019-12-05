# :earth_americas: FTapP
![](https://img.shields.io/badge/platform-windows%20%7C%20linux-blue) ![](https://img.shields.io/badge/gtk-3.20-orange) ![](https://img.shields.io/cocoapods/l/AFNetworking)

Trabalho realizado para a disciplina MATA59 - Redes de Computadores, ministrada pelo professor Gustavo Bittencourt Figueiredo, do Departamento de Ciência da Computação do Instituto de Matemática e Estatistica da Universidade Federal da Bahia.

> :warning: Ao contribuir com a aplicação fique atento para permanecer aos padrões desse repositório:
>   - Commits e comentários no código em português 
>   - Nomes de funções e de variáveis em inglês
>   - Nomes de constantes e macros em MAIÚSCULAS 
>   - Nomes de funções em `snake_case`
## :dart: Objetivo 

O trabalho visa criar uma aplicação de transferência de arquivos entre hosts de uma rede, utilizando o protocolo **FTP**, objetivando a implementação de todas as camadas de protocolos do modelo TCP/IP.

## Panorama Geral

O projeto será modularizado da seguinte forma: haverá um arquivo de código para aplicação do servidor, outro para a aplicação do cliente (estas duas funcionarão como código main), um outro para camada de transporte, e mais um para camada de rede. 

Cada um deles será implementado de forma independente, evitando acoplamento, assim se aproximando de uma implementação real da pilha de camadas TCP/IP.

A aplicação do cliente contará com uma interface gráfica simples, em GTK, que é um toolkit multi plataforma para a criação de interfaces gráficas, liberado sob a licença GNU LGPL, onde terá um campo onde será inserido o endereço IP da máquina destinatária, além de um campo para escolher um arquivo a ser enviado. Além de um botão para realizar o envio do arquivo.

### :desktop_computer: Camada de Aplicação
> :warning: :timer_clock: Na segunda etapa do trabalho, deverá ser entregue a aplicação rodando e um relatório parcial da camada de aplicação.
Haverá duas aplicações: uma para o cliente e outra para o servidor. A aplicação de servidor não terá interface gráfica. No servidor terá um tipo de segurança deixando a critério do usuário a aceitação do arquivo.

Nessa camada opera o protocolo a ser implementado: o FTP, onde o arquivo será empacotado e repassado à camada de transporte, que fará seu envio. Além do empacotamento, o protocolo fará o controle de dados na porta 21.


### :articulated_lorry: Camada de Transporte

A camada de transporte implementará a confiabilidade da transferência dos dados de acordo com o protocolo TCP.

###  :satellite: Camada de Rede

Serão adicionados os endereços aos cabeçalhos do pacotes.

### Camada Física

A camada física será simulada através de sockets, utilizando a biblioteca em C *<sys/socket.h>* com datagramas, pois esse método não garante a confiabilidade dos dados, assim como a camada física real.
