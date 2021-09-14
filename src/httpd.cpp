//
// Created by wwd on 2021/9/14.
//

#include "httpd.h"

Httpd::Httpd() : server_socket_(0){};

// create server socket
// bind socket
// listen
void Httpd::start_up(u_short port) {
    int err_code;
    // create socket for server
    server_socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    // bind socket with address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET,
    addr.sin_port = htons(port),
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // listen 0.0.0.0
    err_code = bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr));
    if (err_code == -1){
        perror("ERROR: server socket bind failed\n");
        exit(-1);
    }
    printf("server socket bind success\n");

    // if port == 0, the system will allocate random port
    if (port == 0){
        socklen_t addr_len = sizeof(addr);
        err_code = getsockname(server_socket_, (struct sockaddr*)&addr, &addr_len);
        if (err_code == -1){
            perror("ERROR: get socket name failed\n");
            exit(-1);
        }
        printf("server socket bind on port:%d\n", addr.sin_port);
    }

    // waiting for the connection from client
    err_code = listen(server_socket_, 5);
    if (err_code == -1){
        perror("ERROR: server listen failed\n");
        exit(-1);
    }
    printf("server listening\n");

    // receive and handle HTTP request
    loop();
}

//
void Httpd::loop() {
    struct sockaddr_in client_addr{};
    socklen_t client_addr_size = sizeof(client_addr);

    int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_addr_size);
    if (client_socket == -1){
        perror("ERROR: accept client connection failed\n");
        exit(-1);
    }

    Httpd_handler handler;
    handle_request(handler, client_socket);

}

void Httpd::handle_request(Httpd_handler& handler, int& client_socket) {
    handler.receive_request(client_socket);
    handler.parse_request();

}



