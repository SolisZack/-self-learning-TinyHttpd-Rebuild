//
// Created by wwd on 2021/9/14.
//

#include "httpd_handler.h"

Httpd_handler::Httpd_handler(){
    client_fd_ = 0;
}

Httpd_handler::Httpd_handler(int fd, struct  sockaddr_in &addr){
    client_fd_ = fd;
    client_addr_ = addr;
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

void Httpd_handler::close_socket() const {
    if (client_fd_ > 0)
        close(client_fd_);
}

void Httpd_handler::reset() {
    client_fd_ = 0;
}

int Httpd_handler::receive_request(int client_socket) {
    int err = recv(client_socket, buffer_, sizeof(buffer_), 0);

    if (err == -1)
        perror("ERROR: read msg from client failed\n");
    else{
        int substr_start = 0;
        buffer_str_ = buffer_;
#ifdef DEBUG
        std::cout << "\nINCOMING HTTP REQUEST:\n" << buffer_str_ << std::endl;
#endif
        for (int i = 0; i < buffer_str_.size(); i++){
            if (buffer_str_[i] == '\n'){
                std::string temp = buffer_str_.substr(substr_start, i - substr_start);
                substr_start = i + 1;
                buffer_byline_.push_back(temp);
            }
        }
#ifdef DEBUG
        std::cout << "\nINCOMING HTTP REQUEST LINE AND ITS HEAD:\n";
        for (auto& str : buffer_byline_)
            std::cout << str << std::endl;
#endif
    }
}

void Httpd_handler::parse_request() {
    parse_request_line();
    parse_header();
    std::cout << "get url: " << get_url() << std::endl;
    std::cout << "get content-lenght: " << get_content_length() << std::endl;
}

void Httpd_handler::parse_request_line() {
    std::string request_line = buffer_byline_[0];
    int count = 0, substr_start = 0;
    for (int i = 0; i < request_line.size(); i++){
        if (request_line[i] == ' '){
            // parsing method
            if (count == 0){
                method_ = request_line.substr(substr_start, i - substr_start);
                count++;
                substr_start = i + 1;
            }
            // parsing url
            else if (count == 1){
                url_ = request_line.substr(substr_start, i - substr_start);
                break;
            }
        }
    }
#ifdef DEBUG
    std::cout << "method:" << method_ << " url:" << url_ << std::endl;
#endif
}

void Httpd_handler::parse_header() {
    for (int i = 1; i < buffer_byline_.size(); i++){
        std::string key, value;
        for (int j = 0; j < buffer_byline_[i].size(); j++){
            if (buffer_byline_[i][j] == ':'){
                key = buffer_byline_[i].substr(0, j);
                value = buffer_byline_[i].substr(j + 2, buffer_byline_[i].size() - j - 1);
                header_[key] = value;
                break;
            }
        }
    }

#ifdef DEBUG
    std::cout << "HEAD:\n" ;
    for (auto& params : header_)
        std::cout << params.first << ":" << params.second << std::endl;
#endif
}

void Httpd_handler::parse_body() {

}

std::string Httpd_handler::get_url() {
    return url_;
}

int Httpd_handler::get_content_length() {
    auto result = header_.find("Content-Length");
    if (result != header_.end())
        return atoi(result->second.c_str());
    return -1;
}

bool Httpd_handler::is_GET() {

}

bool Httpd_handler::is_POST() {

}

void Httpd_handler::error400() const {
    std::string s = std::string(STATUS_400) +
               "Content-type: text/html\r\n" +
               "\r\n" +
               "<P>Your browser sent a bad request, " +
               "such as a POST without a Content-Length.\r\n";
    send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::error404() const {
    std::string s = std::string(STATUS_404) +
               SERVER_STRING +
               "Content-type: text/html\r\n" +
               "\r\n" +
               "<HTML><TITLE>Not Found</TITLE>\r\n" +
               "<BODY><P>The server could not fulfill\r\n" +
               "your request because the resource specified\r\n" +
               "is unavailable or nonexistent.\r\n" +
               "</BODY></HTML>\r\n";
    send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::error500() const {
    std::string s = std::string(STATUS_500) +
               "Content-Type: text/html\r\n" +
               "\r\n" +
               "<P>Error prohibited CGI execution.\r\n";
    send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::error501() const {
    std::string s = std::string(STATUS_501) +
            SERVER_STRING +
            "Content-Type: text/html\r\n" +
            "\r\n" +
            "<HTML><HEAD><TITLE>Method Not Implemented\r\n" +
            "</TITLE></HEAD>\r\n" +
            "<BODY><P>HTTP request method not supported.\r\n" +
            "</BODY></HTML>\r\n";
    send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::serve_file(const char *path) {

}

int Httpd_handler::discard_body() {

}






