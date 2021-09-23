//
// Created by wwd on 2021/9/14.
//

#include <wait.h>
#include "httpd_handler.h"

Httpd_handler::Httpd_handler(){
    client_fd_ = 0;
    path_ = "/home/wwd/CLionProjects/MyHttpd/htdocs";
}

Httpd_handler::Httpd_handler(int& fd, struct  sockaddr_in& addr){
    client_fd_ = fd;
    client_addr_ = addr;
    path_ = "/home/wwd/CLionProjects/MyHttpd/htdocs";
}

Httpd_handler::~Httpd_handler(){
    Httpd_handler::reset();
}

void Httpd_handler::close_socket() const {
    if (client_fd_ > 0)
        close(client_fd_);
}

void Httpd_handler::reset() {
    client_fd_ = 0;
}

// receive the whole http request and store it in string buffer_str_
// and divide the string by line and store them into vector buffer_by_line_
int Httpd_handler::receive_request() {
    char buffer[MAX_BUF_SIZE];
    if (client_fd_ == 0)
        perror("ERROR: no client socket accept");
    int err = recv(client_fd_, buffer, sizeof(buffer), 0);
    if (err == -1)
        perror("ERROR: read msg from client failed\n");
    else{
        int substr_start = 0;
        buffer_str_ = buffer;
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
    }
}

// functions below are added keywords "inline", so can't directly use them in class Httpd
void Httpd_handler::parse_request() {
    parse_request_line();
    parse_header();
    parse_body();
}

// parse http request's first line, including method, url
// and if the method is GET, parse its query
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
                // parsing query
                int index = url_.find('?');
                if (index != std::string::npos){
                    parse_params(url_.substr(index + 1, url_.size() - index), query_);
                    url_ = url_.substr(0, index);
                }
                substr_start = i + 1;
                break;
            }
        }
    }
    // parse version of HTTP
    ver_ = request_line.substr(substr_start, request_line.size() - substr_start);
#ifdef DEBUG
    std::cout << "URL:" << url_ << std::endl;
    std::cout << "QUERY:\n" ;
    check_maps(query_);
    std::cout << "VER:" << ver_ << std::endl;
#endif
}

// parse header, store info into a map
void Httpd_handler::parse_header() {
    for (int i = 1; i < buffer_byline_.size(); i++){
        std::string key, value;
        // Avoid receive multiple Http request
        if (strstr(buffer_byline_[i].c_str(), "POST") || strstr(buffer_byline_[i].c_str(), "GET"))
            break;
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
    check_maps(header_);
#endif
}

// if http's method is POST, parse parameters in body, store parameters into a map called params_
void Httpd_handler::parse_body() {
    int content_length = get_content_length();
    if (content_length == -1){
        if (is_POST()){
            send_error400();
            close_socket();
            exit(0);
        }
        else
            std::cout << "Request's body is empty\n";
        return;
    }
    std::string body = buffer_str_.substr(buffer_str_.size() - content_length, content_length);
    parse_params(body, params_);
#ifdef DEBUG
    std::cout << "BODY: " << body << std::endl;
    std::cout << "PUT PARAMS:\n" ;
    check_maps(params_);
#endif
}

// parameters from GET AND POST are stored in different map
// Create a function that can be reused to handle situation above
void Httpd_handler::parse_params(const std::string& params_str, std::map<std::string, std::string>&params_map) {
    int substr_start = 0;
    std::string key, value;
    for (int i = 0; i < params_str.size(); i++){
        if (params_str[i] == '='){
            key = params_str.substr(substr_start, i - substr_start);
            substr_start = i + 1;
        }else if (params_str[i] == '&'){
            value = params_str.substr(substr_start, i - substr_start);
            substr_start = i + 1;
            params_map[key] = value;
        }
    }
    params_map[key] = params_str.substr(substr_start, params_str.size() - substr_start);
}

// for DEBUG use, print maps
void Httpd_handler::check_maps(std::map<std::string, std::string>& params_map){
    for (auto& params : params_map)
        std::cout << params.first << ":" << params.second << std::endl;
}

// for POST, get header info Content-Length
// get parameters based on Content-Length
int Httpd_handler::get_content_length() {
    std::map<std::string, std::string>::iterator result = header_.find("Content-Length");
    if (result != header_.end())
        return atoi(result->second.c_str());
    return -1;
}

bool Httpd_handler::is_POST() {
    return method_ == "POST";
}

bool Httpd_handler::is_GET() {
    return method_ == "GET";
}

bool Httpd_handler::method_legal() {
    if (!is_POST() && !is_GET()){
        send_error501();
        exit(0);
    }
    return true;
}

// only execute cgi when the url contains /*.cgi,
bool Httpd_handler::use_cgi() {
    if (strstr(url_.c_str(), ".cgi") != nullptr)
        return true;
    return false;
}

void Httpd_handler::send_status200() const {
    std::string s = std::string(STATUS_200) +
                    SERVER_STRING +
                    "Content-Type: text/html\r\n" +
                    "\r\n";
    ::send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::send_error400() const {
    std::string s = std::string(STATUS_400) +
               "Content-type: text/html\r\n" +
               "\r\n" +
               "<P>Your browser sent a bad request, " +
               "such as a POST without a Content-Length.\r\n";
    send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::send_error404() const {
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

void Httpd_handler::send_error500() const {
    std::string s = std::string(STATUS_500) +
               "Content-Type: text/html\r\n" +
               "\r\n" +
               "<P>Error prohibited CGI execution.\r\n";
    send(client_fd_, s.c_str(), strlen(s.c_str()), 0);
}

void Httpd_handler::send_error501() const {
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

// serve default index.html to user
void Httpd_handler::serve_file() {
    std::string buffer;
    if (url_ == "/")
        url_ += "index.html";
    path_ += url_;

    // open html
    std::ifstream file(path_);
    if (!file.is_open()){
        send_error404();
        exit(0);
    }

    // send header
    send_status200();

    // send body
    while (std::getline(file,  buffer)){
#ifdef DEBUG
        std::cout << "sending: " << buffer.c_str() << std::endl;
#endif
        send(client_fd_, buffer.c_str(), strlen(buffer.c_str()), 0);
    }
    std::cout << "sending complete\n";
    file.close();
}

// execute cgi and transfer the execution result to the user
// we fork a child process to execute cgi
// the parent process is in charge of transferring the execution result
void Httpd_handler::execute_cgi() {
    char temp;
    pid_t pid;
    int status;
    int pipe_to_parent[2];

    if (url_ == "/")
        url_ += "test.cgi";
    path_ += url_;

    // judge if the file exists
    std::ifstream file(path_);
    if (!file.is_open()){
        send_error404();
        exit(0);
    }

    // send header
    send_status200();

    // create one-way channel
    if ((pipe(pipe_to_parent)) == -1){
        send_error500();
        exit(0);
    }

    // fork to have 2 processes
    if ((pid = fork()) < 0){
        send_error500();
        exit(0);
    }

    // child process, execute cgi
    if (pid == 0){
        std::cout << "child process executing cgi\n";
        // close read end
        close(pipe_to_parent[0]);
        // redirect STDOUT to pipe, so the execution result can transfer to parent process
        dup2(pipe_to_parent[1], STDOUT);
        // create environment variable for cgi
        char url_env[255];
        sprintf(url_env, "URL=%s", url_.c_str());
        putenv(url_env);
        char version_env[255];
        sprintf(version_env, "REQUEST_VERSION=%s", ver_.c_str());
        putenv(version_env);
        char method_env[255];
        sprintf(method_env, "REQUEST_METHOD=%s", method_.c_str());
        putenv(method_env);
        char connection_env[255];
        sprintf(connection_env, "CONNECTION=%s", header_.find("Connection")->second.c_str());
        putenv(connection_env);
        // execute cgi
        execl(path_.c_str(), NULL);
        close(pipe_to_parent[1]);
        // after execution, the child process will exit
        exit(0);
    }
    // parent process
    else if (pid > 0){
        printf("creat child process %d\n", pid);
        // close write end
        close(pipe_to_parent[1]);
        // read cgi execution result from pipe's
        // WE HAVE TO READ RESULT FROM PIPE ONE BY ONE
        while (read(pipe_to_parent[0], &temp, sizeof(temp)) > 0){
            // send the result to the client
            send(client_fd_, &temp, sizeof(temp), 0);
        }
        // wait for the child to exit
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) == 0){
            std::cout << "child process exit normally\n\n";
        }else
            std::cout << "child process exit abnormally, exit signal code:" << WSTOPSIG(status) << "\n\n";
        close(pipe_to_parent[0]);
    }
}






