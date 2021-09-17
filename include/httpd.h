//
// Created by wwd on 2021/9/14.
//

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include "httpd_handler.h"

#ifndef MYHTTPD_HTTPD_H
#define MYHTTPD_HTTPD_H

#define SOCKET_QUEUE_SIZE 20
#define EPOLL_FD_SIZE 256
#define BUFFER_SIZE 256

class Httpd{
private:
    int server_socket_;
    // variables for epoll
    int epoll_fd_;
    struct epoll_event event_, event_list_[SOCKET_QUEUE_SIZE];
    std::map<int, Httpd_handler*> record_;
public:
    Httpd();

    ~Httpd();

    // HTTPD RUN
    void start_up(u_short port);

    void loop();

    void wait_for_child(int& pid, int& status);

    void sent_to_parent(char* p, Httpd_handler* handler);

    void recv_from_child(char *p, char buffer[MAX_BUF_SIZE]);

    void modify_event(int& socket, int op, uint32_t events);

    Httpd_handler* get_handler(int& client_socket);
};


#endif //MYHTTPD_HTTPD_H
