//
// Created by wwd on 2021/9/14.
//

#include "httpd.h"

Httpd::Httpd() : server_socket_(0){};

Httpd::~Httpd() {
    close(server_socket_);
    for (auto& pair : record_){
        delete pair.second;
        pair.second = nullptr;
    }
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
                    struct sockaddr_in client_addr{};
                    socklen_t client_addr_size = sizeof(client_addr);
                    int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_addr_size);

                    if (client_socket == -1 && errno == EAGAIN){
#ifdef CHECK
                        std::cout << "no more events, stop accepting\n";
#endif
                        break;
                    }
#ifdef CHECK
                    std::cout << "CLIENT SOCKET " << client_socket <<  " ACCEPTED\n";
#endif
                    // register client_socket to epoll
                    modify_event(client_socket, EPOLL_CTL_ADD, EPOLLIN | EPOLLET);
                }
            }

            // client_socket triggered event EPOLLIN, accept http request
            // try handle request
            else if (event_list_[i].events & EPOLLIN){
                int client_socket = event_list_[i].data.fd;
#ifdef CHECK
                std::cout << "CLIENT SOCKET " << client_socket <<  " READING\n";
#endif
                if (client_socket < 0)
                    continue;
                Httpd_handler* handler = get_handler(client_socket);

                char* p = (char*) mmap(nullptr, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
                if (p == MAP_FAILED){
                    perror("ERROR: mmap failed\n");
                    handler->send_error500();
                    return;
                }

                // fork child process to handle request;
                pid = fork();
                if (pid == -1){
                    handler->send_error500();
                    perror("ERROR: fork child process failed\n");
                    return;
                }

                // child process, in charge of receiving and parsing http request
                if (pid == 0){
                    std::cout << "child process handling read event\n";
                    handler->receive_request();
                    handler->parse_request();
                    sent_to_parent(p, handler);
                    munmap(p, BUFFER_SIZE);
                    exit(0);
                }
                else if (pid > 0){
                    char buffer[BUFFER_SIZE];
                    wait_for_child(pid, status);
                    recv_from_child(p, buffer);
                    std::string buffer_str = buffer;
                    handler->set_base_info(buffer_str);
                    modify_event(client_socket, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET);
                    munmap(p, BUFFER_SIZE);
#ifdef CHECK
                    std::cout << "Receive base info from child: " << buffer_str << "\n";
                    handler->check_all();
#endif
                }
            }

            // server_socket_ triggered event EPOLLOUT
            // fork anther new process to handle http request
            else if (event_list_[i].events & EPOLLOUT){
                int client_socket = event_list_[i].data.fd;
                if (client_socket < 0)
                    continue;
#ifdef CHECK
                std::cout << "CLIENT SOCKET " << client_socket <<  " WRITING\n";
#endif
                Httpd_handler* handler = record_[client_socket];
                // fork child process to handle request;
                pid = fork();
                if (pid == -1)
                    perror("ERROR: fork child process failed\n");
                // child process, in charge of responding to http request
                if (pid == 0){
#ifdef CHECK
                    handler->check_all();
#endif
                    if (!handler->method_legal()){
                        handler->close_socket();
                        exit(0);
                    }
                    if (handler->use_cgi()){
                        handler->execute_cgi();
                    }
                    else{
                        handler->serve_file();
                    }
                    handler->close_socket();
                    exit(0);
                }
                // parent process
                else if (pid > 0){
                    std::cout << "created child process to handle write event\n";
                    wait_for_child(pid, status);
                    modify_event(client_socket, EPOLL_CTL_DEL, EPOLLIN | EPOLLET);
                    close(client_socket);
                    delete record_[client_socket];
                    record_[client_socket] = nullptr;
                }
            }
        }
    }
}

void Httpd::wait_for_child(int &pid, int &status) {
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == 0){
        std::cout << "child process exit normally\n";
    }else
        std::cout << "child process exit abnormally, exit signal code:" << WSTOPSIG(status) << "\n";
}

void Httpd::sent_to_parent(char* p, Httpd_handler* handler) {
    std::string ret = handler->get_base_info();
    const char *info = ret.c_str();
    while (*info != '\0'){
        *p = *info;
        p++;
        info++;
    }
    *p = '\0';
}

void Httpd::recv_from_child(char *p, char* buffer) {
    int index = 0;
    while (*p != '\0'){
        buffer[index] = *p;
        index++;
        p++;
    }
    buffer[index] = '\0';
}

void Httpd::modify_event(int& socket, int op, uint32_t events) {
    event_.data.fd = socket;
    event_.events = events;
    epoll_ctl(epoll_fd_, op, socket, &event_);
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
    record_[client_socket] = handler;

    return handler;
}



