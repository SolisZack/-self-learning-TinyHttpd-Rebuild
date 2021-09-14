//
// Created by wwd on 2021/9/14.
//

#include <map>
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
    char buffer_[MAX_BUF_SIZE]{};
    std::string buffer_str_;
    std::vector<std::string> buffer_byline_;

    // parse result
    std::string method_, url_;
    std::map<std::string, std::string> header_, query_, params_;

public:
    // INIT SOCKET
    Httpd_handler();

    Httpd_handler(int fd, struct  sockaddr_in &addr);

    ~Httpd_handler();

    inline void set_client_fd(int fd);

    inline void set_client_addr(struct sockaddr_in &addr);

    inline void close_socket() const;

    inline void reset();

    // GET AND ANALYSE REQUEST
    int receive_request(int client_socket);

    void parse_request();

    inline void parse_request_line();

    inline void parse_header();

    inline void parse_body();

    inline std::string get_url();

    inline int get_content_length();

    inline bool is_POST();

    inline bool is_GET();

    void error501() const;

    void error500() const;

    void error404() const;

    void error400() const;

    // HANDLE HTTP REQUEST
    void serve_file(const char *path);

    int discard_body();  // ?


};

#endif //MYHTTPD_Httpd_handler_H
