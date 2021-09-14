//
// Created by wwd on 2021/9/14.
//

#include "httpd_handler.h"

Httpd_handler::Httpd_handler(){
    client_fd_ = 0;
    buffer_index_ = 0;
    buffer_len_ = 0;
    query_ = nullptr;
}

Httpd_handler::Httpd_handler(int fd, struct  sockaddr_in &addr){
    client_fd_ = fd;
    client_addr_ = addr;
    buffer_index_ = 0;
    buffer_len_ = 0;
    query_ = nullptr;
}

Httpd_handler::~Httpd_handler(){
    Httpd_handler::reset();
}

void Httpd_handler::set_client_fd(int fd){
    client_fd_ = fd;
}

void Httpd_handler::set_client_addr(struct sockaddr_in &addr){
    client_addr_ = addr;
}

void Httpd_handler::close_socket() {
    if (client_fd_ > 0)
        close(client_fd_);
}

void Httpd_handler::reset() {
    client_fd_ = 0;
    buffer_index_ = 0;
    buffer_len_ = 0;
    query_ = nullptr;
}

int Httpd_handler::get_line() {

}

void Httpd_handler::parse_method() {

}

void Httpd_handler::parse_url() {

}

void Httpd_handler::parse_header() {

}

int Httpd_handler::parse_params(const char *str, std::string &key, std::string &value) {

}

char * Httpd_handler::get_url() {

}

int Httpd_handler::get_content_length() {

}

bool Httpd_handler::is_GET() {

}

bool Httpd_handler::is_POST() {

}

void Httpd_handler::error400() {

}

void Httpd_handler::error404() {

}

void Httpd_handler::error500() {

}

void Httpd_handler::error501() {

}

void Httpd_handler::serve_file(const char *path) {

}

int Httpd_handler::discard_body() {

}






