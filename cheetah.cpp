#include <ev++.h>

#include "cheetah.h"
#include "ident.h"
#include "client.h"
#include "signal.h"
#include "config.h"
#include "tools.h"

Client *irc_client;

static void sig_handler(int sig) {
    std::cout << "Caught SIGINT/SIGTERM" << std::endl;
    delete irc_client;
    exit(0);
}

int main(int argc, char** argv) {
    Config config;
    
    // Handle signals.
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    irc_client = new Client(config);
    irc_client->connect_to_server();
    
    ev::default_loop loop;
    loop.run(0);
       
    return 0;
}

