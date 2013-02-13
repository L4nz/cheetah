#include "client.h"
#include "config.h"
#include "tools.h"
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sstream>
#include <boost/algorithm/string.hpp>

Client::Client(Config irc_config): config(irc_config) {
    buffer_size = 0;
    
    if (!db_connect()) {
        std::cout << "Could not connect to MySQL." << std::endl;
        return;
    }
    
    db_read_channels();
    exit(0);
}

Client::~Client() {
    quit(config.irc_quit_message);
    delete ident;
    db_disconnect();
}

bool Client::connect_to_server() {
    // The Ident object will delete itself after the ident response or if
    // it times out.
    if (config.use_ident) {
        ident = new Ident(config);
    }

    struct hostent* he;
    struct sockaddr_in remoteAddr;

    if ((he = gethostbyname(config.irc_host.c_str())) == NULL) {
        std::cout << "Could not get host!" << std::endl;
        return false;
    }

    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Could not create socket!" << std::endl;
        return false;
    }

    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(config.irc_port);
    remoteAddr.sin_addr = *((struct in_addr *) he->h_addr);
    memset(&(remoteAddr.sin_zero), '\0', 8);

    if (connect(socketFd, (struct sockaddr *) &remoteAddr, sizeof (struct sockaddr)) < 0) {
        std::cout << "Could not connect!" << std::endl;
        return false;
    }

    if (!config.irc_password.empty()) {
        pass(config.irc_password);
    }
    user(config.irc_user_name, config.irc_real_name);
    nick(config.irc_nick_name);

    read_event.set<Client, &Client::handle_receive > (this);
    read_event.start(socketFd, ev::READ);

    return true;
}

bool Client::send_line(const std::string &line) {
    std::cout << "client->'" << line << "'" << std::endl;
    if (send(socketFd, std::string(line + "\r\n").c_str(), line.length() + 2, 0) < 0) {
        std::cout << "Failed to send line to server: " << line << std::endl;
        return false;
    }

    return true;
}

/**
 * handle_receive: splits incomming lines from irc at \r\n and
 * sends them to process_incomming_data. Incomplete lines is stored and
 * merged in buffer over several recv's if necessary. 
 * @param watcher
 * @param events_flags
 */
void Client::handle_receive(ev::io &watcher, int events_flags) {
    char recv_buffer[MAX_BUFFER_SIZE];
    int size = recv(socketFd, &recv_buffer, MAX_BUFFER_SIZE, 0);

    if (size > 0) {
        if (buffer_size + size > MAX_BUFFER_SIZE) {
            std::cout << "Error: buffer overflow" << std::endl;
            buffer_size = 0;
            return;
        }

        memcpy(buffer + buffer_size, recv_buffer, size);
        size = buffer_size + size;
        buffer[size] = '\0';

        int offset;
        while ((offset = find_crlf(buffer, size)) > 1) {
            buffer[offset - 2] = '\0';
            std::string line = buffer;
            process_incomming_data(line);
            memmove(buffer, buffer + offset, size - offset);
            size = size - offset;
        }
    }

    buffer_size = size;
}

/**
 * process_incomming_data: handles a raw irc line minus the \r\n that
 * has already been removed in the handle_receive event.
 * @param line
 */
void Client::process_incomming_data(std::string &line) {
    session_t session;
    int pos;

    line = trim(line);

    //std::cout << "server->'" << line << "'" << std::endl;
        
    // Get the prefix
    if (line.at(0) == ':') {
        if ((pos = line.find(' ')) > 0) {
            session.prefix = line.substr(1, pos - 1);
            line = line.substr(pos);
        }
    }

    line = ltrim(line);

    // Get the code or the command
    session.code = 0;
    if (isdigit(line.at(0)) && isdigit(line.at(1)) && isdigit(line.at(2))) {
        session.code = atoi(line.substr(0, 3).c_str());
        line = line.substr(3);
    } else if ((pos = line.find(' ')) > 0) {
        session.command = line.substr(0, pos);
        line = line.substr(pos);
    } else {
        // just drop line for now.
        return;
    }

    line = ltrim(line);

    // Get the params
    session.param_count = 0;   
    while (line.length() > 0 && session.param_count < 15) {
        // Message
        if (line.at(0) == ':') {
            session.trailing = line.substr(1);
            break;
        }

        // Params
        if ((pos = line.find(' ')) > 0) {
            session.params[session.param_count] = line.substr(0, pos);
            session.param_count++;
            line = line.substr(pos);
            line = ltrim(line);
        } else {
            session.params[session.param_count] = line;
            session.param_count++;
            break;
        }
    }

    //std::cout << "prefix: '" << session.prefix << "', command: '" << session.command << "', code: " << session.code << ", message: '" << session.trailing << "', param_count: " << session.param_count << ", params[0]: '" << session.params[0] << "', params[1]: '" << session.params[1] << "', params[2]: '" << session.params[2] << "'" << std::endl;

    // Handle Codes
    switch (session.code) {
            // We use the MOTD as a signal that we are fully connected to IRC.
        case RPL_ENDOFMOTD:
        case ERR_NOMOTD:
            on_connected();
            break;
        case ERR_NONICKNAMEGIVEN:
        case ERR_ERRONEUSNICKNAME:
        case ERR_NICKNAMEINUSE:
        case ERR_NICKCOLLISION:
            on_nick_error(session);
    }

    // Handle Commands
    if (session.command == "JOIN") on_join(session);
    else if (session.command == "NICK") on_nick(session);
    else if (session.command == "NOTICE") on_notice(session);
    else if (session.command == "PART") on_part(session);
    else if (session.command == "PING") on_ping(session);
    else if (session.command == "PRIVMSG") on_privmsg(session);
    else if (session.command == "QUIT") on_quit(session);

    //std::cout << "---------------------------------------------------------------------------------------------------------------------------" << std::endl;

}

bool Client::db_connect() {
    return connection.connect(config.mysql_db.c_str(), config.mysql_host.c_str(), 
            config.mysql_username.c_str(), config.mysql_password.c_str(), 0);
}

void Client::db_disconnect() {
    connection.disconnect();
}

void Client::db_read_channels() {
   mysqlpp::Query query = connection.query("SELECT channel_name, min_permission FROM irc_channels;");
   if (mysqlpp::StoreQueryResult res = query.store()) {
       size_t num_rows = res.num_rows();
       for (size_t i = 0; i < num_rows; i++) {
           std::cout << "channel: " << res[i][0] << " min_permission: " << res[i][1] << std::endl;
       }
   }
}

/*******************************************************************************
 * IRC commands
 *******************************************************************************
 */
void Client::join(std::string channel) {
    send_line("join " + channel);
    std::cout << "joining " << channel << std::endl;
}

void Client::nick(std::string nick_name) {
    send_line("NICK " + nick_name);
    nick_name = nick_name;
}

void Client::notice(std::string to, std::string message) {
    send_line(":" + config.irc_nick_name + " NOTICE " + filter_nick(to) + " :" + message);
}

void Client::pass(std::string password) {
    send_line("PASS " + password);
}

void Client::privmsg(std::string to, std::string message) {
    send_line(":" + config.irc_nick_name + " PRIVMSG " + filter_nick(to) + " :" + message);
}

void Client::quit(std::string message) {
    send_line("QUIT :" + message);
    close(socketFd);
}

void Client::user(std::string user_name, std::string real_name) {
    send_line("USER " + user_name + " 0 * :" + real_name);
}

/*******************************************************************************
 * Event handlers
 *******************************************************************************
 */
void Client::on_connected() {
    Splitter channels = Splitter(config.irc_channels, " ");
    if (channels.size() > 0) {
        for (Splitter::size_type i = 0; i < channels.size(); i++) {
            join(channels[i]);
        }
    } else {
        std::cout << "No channels to join specified." << std::endl;
        exit(1);
    }
}

void Client::on_join(session_t &session) {

}

void Client::on_nick(session_t &session) {

}

void Client::on_nick_error(session_t& session) {
    std::cout << "Nick name was not accepted: " << session.trailing << std::endl;
    nick(config.irc_nick_name + "_");
}

void Client::on_notice(session_t &session) {

}

void Client::on_part(session_t &session) {

}

void Client::on_ping(session_t &session) {
    send_line("PONG " + session.trailing);
}

void Client::on_privmsg(session_t &session) {
    if (session.params[0] == config.irc_nick_name) {
        Splitter params(session.trailing, " ");
        if (params.size() == 0) {
            return;
        }
        std::string command = boost::to_upper_copy(params[0]);
        if (command == "ENTER") {
            if (params.size() != 4) {
                privmsg(session.prefix, "ENTER #channel(,#channel2,#channel3) nick irckey");
                return;
            }
            Splitter channels(params[1], ",");
            std::cout << "enter channels " << channels.size() << std::endl;
        }
    }
}

void Client::on_quit(session_t& session) {

}
