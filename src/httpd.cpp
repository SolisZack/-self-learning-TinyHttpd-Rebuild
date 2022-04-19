//
// Created by wwd on 2021/9/14.
//

#include "httpd.h"

Httpd::Httpd(){
    server_socket_ = 0;
    thread_pool_.init();
}

Httpd::~Httpd() {
    close(server_socket_);
    for (auto& pair : record_){
        delete pair.second;
        pair.second = nullptr;
    }
    thread_pool_.shutPool();
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
// The main process is only in charge of accepting new connection
// The rest of the work is for child process
void Httpd::loop() {
    // var for epoll
    int triggered_nums;
    // loop for accepting request
    while (true){
        triggered_nums = epoll_wait(epoll_fd_, event_list_, SOCKET_QUEUE_SIZE, 0);
        if (triggered_nums == -1)
            perror("ERROR: epoll wait failed\n");
        for (int i = 0; i < triggered_nums; i++){
            // server_socket_ triggered event EPOLLIN, accept new connection
            if (event_list_[i].data.fd == server_socket_){
                thread_pool_.submitFunc(accept_connection, this);
            }
            // client_socket triggered event EPOLLIN, read http request
            else if (event_list_[i].events & EPOLLIN){
                int client_socket = event_list_[i].data.fd;
                if (client_socket < 0)
                    continue;
                thread_pool_.submitFunc(read_request, client_socket, this);
            }
            // server_socket_ triggered event EPOLLOUT
            else if (event_list_[i].events & EPOLLOUT){
                int client_socket = event_list_[i].data.fd;
                if (client_socket < 0)
                    continue;
                thread_pool_.submitFunc(response_request, client_socket, this);
            }
        }
    }
}

// Using ET mod, we don't know exactly how much connection needed to be set once triggered
// So we use while loop to solve the problem above
// This function will accept new connection and add it to the epoll
void Httpd::accept_connection(Httpd* this_ptr) {
    std::cout << "\nSERVER SOCKET " << this_ptr->server_socket_ <<  " ACCEPTING\n";
    while (true){
        struct sockaddr_in client_addr{};
        socklen_t client_addr_size = sizeof(client_addr);
        int client_socket = accept(this_ptr->server_socket_, (struct sockaddr*)&client_addr, &client_addr_size);

        if (client_socket == -1 && errno == EAGAIN){
#ifdef CHECK
            std::cout << "no more events, stop accepting\n";
#endif
            break;
        }
        std::cout << "CLIENT SOCKET " << client_socket <<  " ACCEPTED\n";
        // register client_socket to epoll
        this_ptr->modify_event(client_socket, EPOLL_CTL_ADD, EPOLLIN | EPOLLET);
    }
}

// Fork anther new process to read http request
// Our main process only works on the epoll work
// This function will read and parse http request from client in child process and return info needed to parent process
// After read, the epoll trigger event will change to EPOLLOUT
void Httpd::read_request(int& client_socket, Httpd* this_ptr) {
    std::cout << "CLIENT SOCKET " << client_socket <<  " READING\n";
    // get handler
    Httpd_handler* handler = this_ptr->get_handler(client_socket);
    // recv and read http request
    handler->receive_request();
    handler->parse_request();
    // change event to write event
    this_ptr->modify_event(client_socket, EPOLL_CTL_MOD, EPOLLOUT | EPOLLET);
#ifdef CHECK
    std::cout << "PARSE HTTP REQUEST RESULT:\n";
    handler->check_all();
#endif
}

// Fork anther(yeah, one more another) new process to handle http request
// The child process is in charge of executing http request and return the result to the "parent" process
// The "parent" process will send result to the client
void Httpd::response_request(int &client_socket, Httpd* this_ptr) {
    std::cout << "CLIENT SOCKET " << client_socket <<  " WRITING\n";
    // get handler
    Httpd_handler* handler = this_ptr->record_[client_socket];
    // method check
    if (!handler->method_legal())
        handler->send_error400();
    // response request
    if (handler->use_cgi())
        handler->execute_cgi();
    else
        handler->serve_file();

    // shut down connection
    handler->close_socket();
    // del event
    this_ptr->modify_event(client_socket, EPOLL_CTL_DEL, EPOLLIN | EPOLLET);
    // del current record for accepting new connection
    delete this_ptr->record_[client_socket];
    this_ptr->record_[client_socket] = nullptr;

#ifdef CHECK
    handler->check_all();
#endif
}

// Our HTTPD server has a lot of "fork" action
// This function will help the parent process to wait for the child to complete
void Httpd::wait_for_child(int &pid, int &status) {
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == 0){
        std::cout << "child process exit normally\n";
    }else
        std::cout << "child process exit abnormally, exit signal code:" << WSTOPSIG(status) << "\n";
}

// This function will send the needed http parsing result to the parent process for child process
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

// This function will receive the needed http parsing result from the child process for parent process
void Httpd::recv_from_child(char *p, char* buffer) {
    int index = 0;
    while (*p != '\0'){
        buffer[index] = *p;
        index++;
        p++;
    }
    buffer[index] = '\0';
}

// This function will do something for the current socket based on the operation and events
void Httpd::modify_event(int& socket, int op, uint32_t events) {
    event_.data.fd = socket;
    event_.events = events;
    epoll_ctl(epoll_fd_, op, socket, &event_);
}

// This function will get httpd_handler based on the client socket using func getsockname
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



