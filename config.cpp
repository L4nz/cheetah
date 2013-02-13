#include "config.h"

Config::Config() {
    irc_host = "shire.digitalirc.org";
    //host = "dreamhack.se.quakenet.org";
    //host = "irc.eu.dal.net";
    irc_port = 6667;
    use_ident = true;
    ident_timeout = 30;
    irc_password = "";
    irc_nick_name = "mydrone";
    irc_user_name = "the_drone";
    irc_real_name = "IamTheDrone";
    irc_quit_message = "Good Bye";
    irc_channels = "#emptesting";
    
    mysql_db = "gazelle";
    mysql_host = "127.0.0.1:3306";
    mysql_username = "root";
    mysql_password = "";    
}

