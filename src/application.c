// #include "../include/user_interface.h"
#include "../include/client.h"
#include "../include/transport.h"

int main(int argc, char **argv)
{
	GBN_transport_init(CLIENT);
	send_file("arquivo", "127.0.0.1");
    // return app_start();
    GBN_transport_end();
}
