#ifndef CONFIG_H
#define	CONFIG_H

#include <string>

class Config {
public:
    Config();
    std::string irc_host;
    unsigned int irc_port;
    bool use_ident;
    unsigned int ident_timeout;
    std::string irc_password;
    std::string irc_nick_name;
    std::string irc_user_name;
    std::string irc_real_name;
    std::string irc_quit_message;
    std::string irc_channels;

    std::string mysql_db;
    std::string mysql_host;
    std::string mysql_username;
    std::string mysql_password;
    
};


#endif	/* CONFIG_H */

