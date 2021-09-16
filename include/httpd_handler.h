//
// Created by wwd on 2021/9/14.
//

#include <map>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifndef MYHTTPD_Httpd_handler_H
#define MYHTTPD_Httpd_handler_H

#define STDIN 0
#define STDOUT 1
#define MAX_BUF_SIZE 1024
#define STATUS_200 "HTTP/1.0 200 OK\r\n"
#define STATUS_400 "HTTP/1.0 400 BAD REQUEST\r\n"
#define STATUS_404 "HTTP/1.0 404 NOT FOUND\r\n"
#define STATUS_500 "HTTP/1.0 500 Internal Server Error\r\n"
#define STATUS_501 "HTTP/1.0 501 Method Not Implemented\r\n"
#define SERVER_STRING "Server: httpd++/1.0.0\r\n"

class Httpd_handler {
private:
    // socket
    int client_fd_;
    struct sockaddr_in client_addr_{};

    // used to parse http msg's first line and its head
    std::string buffer_str_;
    std::vector<std::string> buffer_byline_;

    // parse result
    std::string method_, url_, ver_;
    std::map<std::string, std::string> header_, query_, params_;

    // web
    std::string path_;

public:
    // INIT SOCKET
    Httpd_handler();

    Httpd_handler(int& fd, struct sockaddr_in& addr);

    ~Httpd_handler();

    void close_socket() const;

    inline void reset();

    // GET AND ANALYSE REQUEST
    int receive_request();

    void parse_request();

    inline void parse_request_line();

    inline void parse_header();

    inline void parse_body();

    inline void parse_params(const std::string& params_str, std::map<std::string, std::string>& params_map);

    inline void check_maps(std::map<std::string, std::string>& params_map);

    inline int get_content_length();

    inline bool is_POST();

    inline bool is_GET();

    bool use_cgi();

    inline void send_status200() const;

    inline void send_error400() const;

    inline void send_error404() const;

    inline void send_error500() const;

    inline void send_error501() const;

    // HANDLE HTTP REQUEST
    void serve_file();

    void execute_cgi();

};

#endif //MYHTTPD_Httpd_handler_H
