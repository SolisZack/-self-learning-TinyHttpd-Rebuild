//
// Created by wwd on 2021/9/14.
//
#include "httpd_handler.h"

#ifndef MYHTTPD_HTTPD_H
#define MYHTTPD_HTTPD_H

class Httpd{
private:
    int server_socket_;
public:
    Httpd();

    ~Httpd();

    // HTTPD RUN
    void start_up(u_short port);

    void loop();

    void handle_request(Httpd_handler& handler);
};


#endif //MYHTTPD_HTTPD_H
