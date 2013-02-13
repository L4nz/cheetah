#include "tools.h"

// find crlf in a c string

int find_crlf(const char *buf, int length) {
    int offset = 0;
    for (; offset < (length - 1); offset++)
        if (buf[offset] == 0x0D && buf[offset + 1] == 0x0A)
            return (offset + 2);

    return 0;
}

std::string filter_nick(std::string host_mask) {
    int pos;
    if ((pos = host_mask.find('!')) > 0) {
        return host_mask.substr(0, pos);
    }

    return "";
}

std::string filter_user(std::string host_mask) {
    int pos1 = host_mask.find('!');
    int pos2 = host_mask.find('@');
    if (pos1 > 0 && pos2 > pos1) {
        return host_mask.substr(pos1 + 1, pos2 - pos1 - 1);
    }

    return "";
}

std::string filter_host(std::string host_mask) {
    int pos;
    if ((pos = host_mask.find('@')) > 0) {
        return host_mask.substr(pos + 1);
    }

    return "";
}