#include <sstream>

#include "ident.h"

Ident::Ident(Config irc_config) : config(irc_config) {
    // Set up a listen socket.
    memset(&address, 0, sizeof (address));
    addr_len = sizeof (address);

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    int yes = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1) {
        std::cout << "Ident: Could not reuse socket" << std::endl;
        return;
    }

    listen_event.set<Ident, &Ident::handle_connect > (this);
    listen_event.start(listen_socket, ev::READ);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    // Ident is always on port 113 afaik
    int port = 113;
    address.sin_port = htons(port);

    if (bind(listen_socket, (sockaddr *) & address, sizeof (address)) == -1) {
        std::cout << "Ident: Bind failed " << errno << ": " << strerror(errno) << std::endl;
        return;
    }

    if (listen(listen_socket, 1) == -1) {
        std::cout << "Ident: Listen failed" << std::endl;
        return;
    }

    // Set socket to non-blocking
    int flags = fcntl(listen_socket, F_GETFL);
    if (flags == -1) {
        std::cout << "Ident: Could not get socket flags" << std::endl;
        return;
    }
    if (fcntl(listen_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cout << "Ident: Could not set non-blocking" << std::endl;
        return;
    }

    // Set up a general timeout for class to live or wait for receive.
    timeout_event.set<Ident, &Ident::handle_timeout > (this);
    timeout_event.set(config.ident_timeout, 0);
    timeout_event.start();
}

Ident::~Ident() {
    read_event.stop();
    timeout_event.stop();
    close(listen_socket);
    close(connect_socket);
}

void Ident::handle_connect(ev::io &watcher, int events_flags) {
    listen_event.stop();
    timeout_event.stop();

    connect_socket = accept(listen_socket, (sockaddr *) & address, &addr_len);

    if (connect_socket == -1) {
        std::cout << "Ident: Accept failed, errno " << errno << ": " << strerror(errno) << std::endl;
        delete this;
        return;
    }

    int flags = fcntl(connect_socket, F_GETFL);
    if (flags == -1) {
        std::cout << "Ident: Could not get connect socket flags" << std::endl;
        delete this;
        return;
    }
    if (fcntl(connect_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cout << "Ident: Could not set non-blocking" << std::endl;
        delete this;
        return;
    }

    read_event.set<Ident, &Ident::handle_receive > (this);
    read_event.start(connect_socket, ev::READ);

    timeout_event.start();

}

void Ident::handle_receive(ev::io &watcher, int events_flags) {
    read_event.stop();

    char buffer[1024 + 1];
    memset(buffer, 0, 1024 + 1);

    int status = recv(connect_socket, &buffer, 1024, 0);

    if (status == -1) {
        std::cout << "Ident: incorrect incomming ident" << std::endl;
        delete this;
        return;
    }

    std::string line = buffer;

    // TODO: better checking
    int p = line.find(",");
    if (p > 0) {
        int first = atoi(line.substr(0, p - 1).c_str());
        int last = atoi(line.substr(p + 1).c_str());

        std::stringstream identResponse;
        identResponse << last << " , " << first << " : USERID : UNIX : " << config.irc_nick_name;
        std::string identResponseStr = identResponse.str();

        // Send the response string.
        send(connect_socket, identResponseStr.c_str(), identResponseStr.size(), MSG_NOSIGNAL);
    }

    delete this;
}

void Ident::handle_timeout(ev::io &watcher, int eventsFlags) {
    timeout_event.stop();
    read_event.stop();
    std::cout << "Ident: timed out." << std::endl;
    delete this;
}
