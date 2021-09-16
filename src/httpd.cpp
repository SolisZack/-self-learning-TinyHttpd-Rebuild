//
// Created by wwd on 2021/9/14.
//

#include "httpd.h"

Httpd::Httpd() : server_socket_(0){};

Httpd::~Httpd() {
    close(server_socket_);
}

// create server socket
// bind socket
// listen
// htons htonl ntohs ntohl(h = host, n = net, s = short, l = long)
// used to ignore big endian and small endian problem
void Httpd::start_up(u_short port) {
    int err_code;
    // create socket for server
    server_socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    // bind socket with address
    struct sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // listen 0.0.0.0
    err_code = bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr));
    if (err_code == -1){
        perror("ERROR: server socket bind failed\n");
        exit(-1);
    }
    std::cout << "server socket bind success\n";

    // if port == 0, the system will allocate random port
    if (port == 0){
        std::cout << "allocating random port for the server\n";
        socklen_t addr_len = sizeof(addr);
        err_code = getsockname(server_socket_, (struct sockaddr*)&addr, &addr_len);
        if (err_code == -1){
            perror("ERROR: get socket name failed\n");
            exit(-1);
        }
        std::cout << "server socket bind on port:" << ntohs(addr.sin_port) << "\n";
    }

    // set server_socket_ non-block
    // get server_socket_ flags
    int flags = fcntl(server_socket_, F_GETFL);
    if (flags == -1)
        perror("ERROR: get server_socket_ flags failed\n");
    // set server_socket_ flags, add non-block to the flags
    fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);

    // create epoll fd
    epoll_fd_ = epoll_create(EPOLL_FD_SIZE);
    // bind event on server_socket_
    event_.data.fd = server_socket_;
    // use trigger mod ET
    event_.events = EPOLLIN | EPOLLET;
    // register event
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_socket_, &event_);

    // waiting for the connection from client
    err_code = listen(server_socket_, SOCKET_QUEUE_SIZE);
    if (err_code == -1){
        perror("ERROR: server listen failed\n");
        exit(-1);
    }
    std::cout << "server listening\n\n";

    // receive and handle HTTP request
    loop();
}

// Based on epoll and MPM prefork mod
void Httpd::loop() {
    // var for fork
    pid_t pid;
    int status;
    // var for epoll
    int triggered_nums;
    while (true){
        triggered_nums = epoll_wait(epoll_fd_, event_list_, SOCKET_QUEUE_SIZE, 0);
        if (triggered_nums == -1)
            perror("ERROR: epoll wait failed\n");
        for (int i = 0; i < triggered_nums; i++){
            // server_socket_ triggered event EPOLLIN, accept new connection
            // fork anther new process to handle http request
            if (event_list_[i].data.fd == server_socket_){
                while (true){
#ifdef DEBUG
                    std::cout << "ACCEPTING\n";
#endif
                    struct sockaddr_in client_addr{};
                    socklen_t client_addr_size = sizeof(client_addr);
                    std::cout << "accepting new connection\n";
                    int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_addr_size);
                    if (client_socket == -1 && errno == EAGAIN){
                        std::cout << "no more events, stop accepting\n";
                        break;
                    }
                    // register client_socket to epoll
                    event_.data.fd = client_socket;
                    event_.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_socket, &event_);
                }
            }
            // client_socket triggered event EPOLLIN, accept http request
            // try handle request
            else if (event_list_[i].events & EPOLLIN){
#ifdef DEBUG
                std::cout << "READING\n";
#endif
                int client_socket = event_list_[i].data.fd;
                if (client_socket < 0)
                    continue;
                Httpd_handler* handler = get_handler(client_socket);
                handler->receive_request();
                handler->parse_request();
                modify_event(client_socket, EPOLLOUT | EPOLLET);
            }
            else if (event_list_[i].events & EPOLLOUT){
#ifdef DEBUG
                std::cout << "WRITING\n";
#endif
                int client_socket = event_list_[i].data.fd;
                if (client_socket < 0)
                    continue;
                pid = fork();
                if (pid == -1)
                    perror("ERROR: fork child process failed\n");
                // child process
                if (pid == 0){
                    Httpd_handler* handler = record[client_socket];
                    if (!handler->method_legal()){
                        handler->close_socket();
                        exit(0);
                    }
                    if (handler->use_cgi()){
#ifdef DEBUG
                        std::cout << "child process is going to execute cgi\n";
#endif
                        handler->execute_cgi();
                    }
                    else{
#ifdef DEBUG
                        std::cout << "child process is going to serve file\n";
#endif
                        handler->serve_file();
                    }
                    handler->close_socket();
                    exit(0);
                }
                // parent process
                else if (pid > 0){
                    std::cout << "created child process " << pid << "\n";
                    waitpid(pid, &status, 0);
                    if (WEXITSTATUS(status) == 0){
                        std::cout << "child process exit normally\n\n";
                    }else
                        std::cout << "child process exit abnormally, exit signal code:" << WSTOPSIG(status) << "\n\n";
                    modify_event(client_socket, EPOLLIN | EPOLLET);
                    close(client_socket);

                }
            }
        }
    }
}

void Httpd::modify_event(int& socket, uint32_t events) {
    event_.data.fd = socket;
    event_.events = events;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket, &event_);
}

Httpd_handler* Httpd::get_handler(int& client_socket) {
    struct sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int err_code = getsockname(client_socket, (struct sockaddr*)&client_addr, &addr_len);

    if (err_code == -1){
        perror("ERROR: get socket name failed\n");
        exit(-1);
    }

    Httpd_handler* handler = new Httpd_handler(client_socket, client_addr);
    record[client_socket] = handler;

    return handler;
}



