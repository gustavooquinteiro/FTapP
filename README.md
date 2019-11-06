# :earth_americas: FTapP
![](https://img.shields.io/badge/platform-windows%20%7C%20linux-blue) ![](https://img.shields.io/badge/gtk-3.20-orange) ![](https://img.shields.io/cocoapods/l/AFNetworking) ![GitHub release (latest by date)](https://img.shields.io/github/v/release/gustavooquinteiro/FTapP?color=green)

Trabalho realizado para a disciplina MATA59 - Redes de Computadores, ministrada pelo professor Gustavo Bittencourt Figueiredo, do Departamento de Ciência da Computação do Instituto de Matemática e Estatistica da Universidade Federal da Bahia.

## :busts_in_silhouette: Contribuições
Ao contribuir com esse projeto fique atento para permanecer aos padrões desse repositório:
   - Commits e comentários no código em português 
   - Nomes de funções e de variáveis em inglês
   - Nomes de constantes e macros em MAIÚSCULAS 
   - Nomes de funções em `snake_case`
   - Chaves de função em nova linha, como no exemplo abaixo:
   ```c
   int foo_bar(int args)
   {
    \\ código da função identado com <TAB>
   }
   ```
   - Exceções à regra acima são condicionais e/ou laços de repetição de uma única linha

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

#### Descrição do protocolo implementado
1. **Estabelecimento da Conexão**  
O servidor escuta na porta **8074**, à espera de um cliente que deseje estabelecer Conexão. Neste momento o servidor vai estar esperando uma mensagem contendo o caractere ``` A ``` da tabela ASCII. Neste momento, o servidor envia uma mensagem de 4 *bytes* para o cliente X contendo o tamanho máximo de cada fragmento do arquivo que receberá. Logo após, para confirmar o estabelecimento da conexão da camada de aplicação, o cliente envia outra mensagem, contendo informações do tamanho máximo do arquivo que deseja enviar e o tamanho de cada fragmento que irá enviar, baseado no máximo estabelecido pelo servidor. O formato é 8 *bytes* mais significativos para o tamanho do arquivo, e os 4 *bytes* menos significativos para o tamanho dos fragmentos.
2. **Conexão Estabelecida**  
Uma vez com a conexão estabelecida, cada requisição que o servidor receber será colocada na fila de conexões TCP. Enquanto isso, através da mesma porta (8074), o servidor passará a receber sucessivamente fragmentos contíguos de dados do arquivo, com tamanho estabelecido anteriormente pelo cliente. Ele será responsável por concatenar os dados e, consequentemente, salvar o arquivo no disco. O cliente será responsável por enviar o arquivo, através do envio desses fragmentos.
Caso haja qualquer erro durante a execução desses passos, o envio será cancelado, e o servidor sucederá para a próxima conexão, fechando a atual. Nessas mesmas condições, uma mensagem de erros será mostrada ao usuário da aplicação cliente referente ao erro ocorrido.
3. **Término do Envio**  
Ao fim do envio com sucesso, o servidor envia uma mensagem de confirmação para o cliente com a frase "Arquivo recebido com sucesso". Mesmo que a mensagem não seja enviada, o arquivo ainda terá sido enviado com sucesso, porém sem a confirmação do servidor e uma mensagem de aviso aparecerá para o usuário da aplicação cliente. Neste momento, as conexões TCP são finalizadas. 


### :articulated_lorry: Camada de Transporte

A camada de transporte implementará a confiabilidade da transferência dos dados de acordo com o protocolo TCP.

###  :satellite: Camada de Rede

Serão adicionados os endereços aos cabeçalhos do pacotes.

### Camada Física

A camada física será simulada através de sockets, utilizando a biblioteca em C *<sys/socket.h>* com datagramas, pois esse método não garante a confiabilidade dos dados, assim como a camada física real.

## :copyright: Contribuidores
Em agradecimento às pessoas que contribuiram para o desenvolvimento desse projeto, veja [AUTHORS](AUTHORS)
