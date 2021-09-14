//
// Created by wwd on 2021/9/14.
//

#include <map>
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

    // used to parse http message
    char buffer_[MAX_BUF_SIZE]{};
    size_t buffer_index_, buffer_len_;  //size_t for multi platform

    // parse result
    char *query_;
    char method_[255]{}, url_[255]{};
    std::map<std::string, std::string> m_header_;
public:
    // INIT SOCKET
    Httpd_handler();

    Httpd_handler(int fd, struct  sockaddr_in &addr);

    ~Httpd_handler();

    inline void set_client_fd(int fd);

    inline void set_client_addr(struct sockaddr_in &addr);

    inline void close_socket();

    inline void reset();

    // GET AND ANALYSE REQUEST
    int get_line();

    inline void parse_method();

    inline void parse_url();

    inline void parse_header();

    int parse_params(const char* str, std::string &key, std::string &value);

    inline char* get_url();

    inline int get_content_length();

    inline bool is_POST();

    inline bool is_GET();

    void error501();

    void error500();

    void error404();

    void error400();

    // HANDLE HTTP REQUEST
    void serve_file(const char *path);

    int discard_body();  // ?



};


#endif //MYHTTPD_Httpd_handler_H
