#ifndef IDENT_H
#define	IDENT_H

#include "cheetah.h"
#include "config.h"

// libev
#include <ev++.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>

class Ident {
public:
    Ident(Config irc_config);
    virtual ~Ident();

private:
    void handle_connect(ev::io &watcher, int events_flags);
    void handle_receive(ev::io &watcher, int events_flags);
    void handle_timeout(ev::io &watcher, int events_flags);

    Config config;
    ev::io listen_event;
    ev::io read_event;
    ev::io timeout_event;
    int listen_socket;
    int connect_socket;
    sockaddr_in address;
    socklen_t addr_len;

};

#endif	/* IDENT_H */

