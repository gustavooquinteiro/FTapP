# :earth_americas: FTapP
![Plataforma](https://img.shields.io/badge/platform-linux-blue) ![GTK version](https://img.shields.io/badge/gtk-3.20-orange) ![Licença](https://img.shields.io/cocoapods/l/AFNetworking) ![GitHub release (latest by date)](https://img.shields.io/github/v/release/gustavooquinteiro/FTapP?color=green) ![Contribuidores](https://img.shields.io/github/contributors/gustavooquinteiro/FTapP?color=yellow) 
Trabalho realizado para a disciplina MATA59 - Redes de Computadores, ministrada pelo professor Gustavo Bittencourt Figueiredo, do Departamento de Ciência da Computação do Instituto de Matemática e Estatistica da Universidade Federal da Bahia.

## :dart: Objetivo 

O trabalho visa criar uma aplicação de transferência de arquivos entre hosts de uma rede, utilizando como base o protocolo **FTP**, objetivando a implementação de todas as camadas de protocolos do [modelo TCP/IP](https://pt.wikipedia.org/wiki/TCP/IP).

## Panorama Geral

O projeto está modularizado da seguinte forma: há um arquivo de código para a aplicação do [servidor](src/server.c), outro para a aplicação do [cliente](src/client.c), ambos funcionando como código principal e executarão suas respectivas funcionalidades. 

Agregando as funcionalidades das camadas do modelo TCP/IP, será modularizado, em arquivos diferentes, as demais camadas. Ou seja, cada uma das camadas do TCP/IP é implementada de forma independente, evitando acoplamento, assim se aproximando de uma implementação real.

A aplicação do cliente contém uma interface gráfica simples, em [GTK](https://www.gtk.org), que é um toolkit multi plataforma para a criação de interfaces gráficas, onde tem um campo onde será inserido o endereço IP da máquina destinatária, além de um campo para escolher um arquivo a ser enviado. Além de um botão para realizar o envio do arquivo.

![Screenshot1](assets/application-screenshot.jpg)  
```
Screenshot da aplicação do cliente em GTK. O tema da aplicação é o tema padrão do computador 
```
### :desktop_computer: Camada de Aplicação

Há duas aplicações: uma para o cliente e outra para o servidor. A aplicação de servidor não tem interface gráfica. 

Nessa camada opera o protocolo a ser implementado: o FTP, onde o arquivo será empacotado e repassado à camada de transporte, que fará seu envio. Além do empacotamento, o protocolo fará o controle de dados.

#### :three::left_right_arrow::handshake: Descrição do protocolo implementado
1. **Estabelecimento da Conexão**  
O servidor escuta na porta **8074**, à espera de um cliente que deseje estabelecer conexão. Neste momento o servidor vai estar esperando uma mensagem de um cliente X contendo o caractere ``` A ``` da tabela ASCII. Neste momento, o servidor envia uma mensagem de 4 *bytes* para o cliente X contendo o **tamanho máximo de cada fragmento do arquivo que receberá**. Logo após, para confirmar o estabelecimento da conexão da camada de aplicação, o cliente envia outra mensagem, contendo **informações do tamanho máximo do arquivo que deseja enviar** e o **tamanho de cada fragmento que irá enviar**, baseado no máximo estabelecido pelo servidor. O formato dessa reposta do cliente ao servidor é definido como uma mensagem de 12 *bytes* onde:  
```python
8 bytes mais significativos são destinados para o tamanho do arquivo e os 4 bytes menos significativos para o tamanho dos fragmentos.
```
2. **Conexão Estabelecida**  
Uma vez com a conexão estabelecida, cada requisição que o servidor receber será colocada na **fila de conexões TCP**. Enquanto isso, através da mesma porta (8074), o servidor passará a receber sucessivamente fragmentos contíguos de dados do arquivo, com tamanho estabelecido anteriormente pelo cliente. Ele será responsável por concatenar os dados e, consequentemente, salvar o arquivo no disco. O cliente será responsável por enviar o arquivo, através do envio desses fragmentos.
Caso haja qualquer erro durante a execução desses passos, o envio será cancelado, e o servidor sucederá para a próxima conexão, fechando a atual. Nessas mesmas condições, uma mensagem de erros será mostrada ao usuário da aplicação cliente referente ao erro ocorrido.

3. **Término do Envio**  
Ao fim do envio com sucesso, o servidor envia uma mensagem de confirmação para o cliente com a frase "*Arquivo recebido com sucesso*". Mesmo que a mensagem não seja enviada, o arquivo ainda terá sido enviado com sucesso, porém sem a confirmação do servidor e uma mensagem de aviso aparecerá para o usuário da aplicação cliente. Neste momento, as conexões **TCP são finalizadas**. 


### :articulated_lorry: Camada de Transporte

A camada de transporte implementará a confiabilidade da transferência dos dados de acordo com o protocolo TCP.

###  :satellite: Camada de Rede

Serão adicionados os endereços aos cabeçalhos do pacotes.

### :electric_plug: Camada Física

A camada física será simulada através de sockets, utilizando a biblioteca em C *<sys/socket.h>* com datagramas, pois esse método não garante a confiabilidade dos dados, assim como a camada física real.

## :busts_in_silhouette: Contribuições
Ao contribuir com esse projeto fique atento para permanecer aos padrões desse repositório:  
   - Commits e comentários no código em português  
   - Nomes de funções e de variáveis em inglês  
   - Nomes de constantes e macros em MAIÚSCULAS  
   - Nomes de funções em `snake_case`  
   - Typedef de structs em `CammelCase`  
   - Chaves de função em nova linha, como no exemplo abaixo:  
   ```c
   int foo_bar(int args)
   {
    \\ código da função identado com <TAB>
   }
   ```  
   - Exceções à regra acima são condicionais e/ou laços de repetição com corpo de uma única linha, exemplo:  
   ```c
   int min(int i, int j)
   {
        if (i > j) return j;
        else return i;
    }
   int fatorial(int n)
   {
        int fatorial = 1
        for (int i = 1; i < n + 1; i++)
            fatorial *= i;
        return fatorial;
    }
   ```

## :copyright: Contribuidores
Em agradecimento às pessoas que contribuiram para o desenvolvimento desse projeto, veja [AUTHORS](AUTHORS)
