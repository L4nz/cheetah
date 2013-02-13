/* 
 * File:   IRC.h
 * Author: lanz
 *
 * Created on den 11 januari 2013, 16:25
 */

#ifndef CLIENT_H
#define	CLIENT_H

#include "config.h"
#include "ident.h"
#include <mysql++/mysql++.h>
#include <ev++.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <tr1/functional>

    /*
        This IRC bot uses the RFC 1459 naming convention for the different parts
        of the IRC line.
    
        <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
        <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
        <command>  ::= <letter> { <letter> } | <number> <number> <number>
        <SPACE>    ::= ' ' { ' ' }
        <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]

        <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
                       or NUL or CR or LF, the first of which may not be ':'>
        <trailing> ::= <Any, possibly *empty*, sequence of octets not including
                         NUL or CR or LF>

        <crlf>     ::= CR LF          
     */

    typedef struct {
        std::string prefix;
        std::string command;
        unsigned int code;
        std::string trailing;
        std::string params[15];
        unsigned int param_count;
    } session_t;

    enum {
        RPL_ENDOFMOTD = 376,

        ERR_NOMOTD = 422,

        ERR_NONICKNAMEGIVEN = 431,
        ERR_ERRONEUSNICKNAME = 432,
        ERR_NICKNAMEINUSE = 433,
        ERR_NICKCOLLISION = 436
    };

    class Client {
    public:
        Client(Config irc_config);
        virtual ~Client();
        bool connect_to_server();
        bool send_line(const std::string &line);

        void join(std::string channel);
        void nick(std::string nick_name);
        void notice(std::string to, std::string message);
        void pass(std::string password);
        void privmsg(std::string to, std::string message);
        void quit(std::string message);
        void user(std::string user_name, std::string real_name);

    protected:
        virtual void on_connected();
        virtual void on_join(session_t &session);
        virtual void on_nick(session_t &session);
        virtual void on_nick_error(session_t &session);
        virtual void on_notice(session_t &session);
        virtual void on_part(session_t &session);
        virtual void on_ping(session_t &session);
        virtual void on_privmsg(session_t &session);
        virtual void on_quit(session_t &session);

    private:
        void handle_receive(ev::io &watcher, int events_flags);
        void process_incomming_data(std::string &line);
        bool db_connect();
        void db_disconnect();
        void db_read_channels();
        
        Config config;
        static const int MAX_BUFFER_SIZE = 4096;
        int buffer_size;
        Ident *ident;
        int socketFd;
        ev::io read_event;
        ev::io error_event;
        char buffer[MAX_BUFFER_SIZE];
        
        mysqlpp::Connection connection;
    };


#endif	/* CLIENT_H */

