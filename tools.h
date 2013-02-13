/* 
 * File:   tools.h
 * Author: lanz
 *
 * Created on den 14 januari 2013, 22:33
 */

#ifndef TOOLS_H
#define	TOOLS_H

#include <string>
#include <iostream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <vector>

// trim from start

static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end

static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends

static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

int find_crlf(const char *buf, int length);

std::string filter_nick(std::string host_mask);
std::string filter_user(std::string host_mask);
std::string filter_host(std::string host_mask);

class Splitter {
    std::vector<std::string> _tokens;
public:
    typedef std::vector<std::string>::size_type size_type;

    Splitter(const std::string& src, const std::string& delim) {
        reset(src, delim);
    }

    std::string& operator[] (size_type i) {
        return _tokens.at(i);
    }

    size_type size() const {
        return _tokens.size();
    }

    void reset(const std::string& src, const std::string& delim) {
        std::vector<std::string> tokens;
        std::string::size_type start = 0;
        std::string::size_type end;
        for (;;) {
            end = src.find(delim, start);
            tokens.push_back(src.substr(start, end - start));
            // We just copied the last token
            if (end == std::string::npos)
                break;
            // Exclude the delimiter in the next search
            start = end + delim.size();
        }
        _tokens.swap(tokens);
    }
};

#endif	/* TOOLS_H */

