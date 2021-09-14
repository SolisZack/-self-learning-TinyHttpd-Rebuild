//
// Created by wwd on 2021/9/14.
//
#include "httpd_handler.h"

#ifndef MYHTTPD_HTTPD_H
#define MYHTTPD_HTTPD_H

class Httpd{
private:
    struct sockaddr_in server_socket_;
public:
    // INIT
    Httpd();

    ~Httpd();

    // HTTPD RUN
    void accept_request(Httpd_handler* handler);

    void start_up(u_short port);

    void loop();
};


#endif //MYHTTPD_HTTPD_H
