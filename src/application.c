#include "../include/user_interface.h"
#include "../include/client.h"
#include "../include/transport.h"

int main(int argc, char **argv)
{
	GBN_transport_init(CLIENT);
    return app_start();
}
