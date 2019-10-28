#include "../include/client.h"


int send_package(FILE * arquivo)
{
    return submit_package(arquivo);
}

int main(int argc, char **argv)
{
    return app_start();
}
