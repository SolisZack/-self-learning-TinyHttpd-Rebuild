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
// used to ignore big endian and small endian problem in different OS
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

    // waiting for the connection from client
    err_code = listen(server_socket_, 20);
    if (err_code == -1){
        perror("ERROR: server listen failed\n");
        exit(-1);
    }
    std::cout << "server listening\n\n";

    // receive and handle HTTP request
    loop();
}

//
void Httpd::loop() {
    pid_t pid;
    while (true){
        struct sockaddr_in client_addr{};
        socklen_t client_addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_socket == -1){
            perror("ERROR: accept client connection failed\n");
            exit(-1);
        }

        if ((pid = fork()) == -1){
            perror("ERROR: fork child process failed\n");
            exit(-1);
        }

        // child process, in charge of handling request
        if (pid == 0){
            std::cout << "child process handling HTTP request\n";
            Httpd_handler handler(client_socket, client_addr);
            handle_request(handler);
            exit(0);
        }
        // parent process, in charge of accepting request
        else if (pid > 0){
            std::cout << "created child process " << pid << "\n";
            std::cout << "waiting for another connection\n";
            // DO REMEMBER CLOSE CLIENT SOCKET IN PARENT PROCESS
            close(client_socket);
        }
    }

}

void Httpd::handle_request(Httpd_handler& handler) {
    handler.receive_request();
    handler.parse_request();
    if (handler.method_legal()){
        if (handler.use_cgi())
            handler.execute_cgi();
        else
            handler.serve_file();
    }
    handler.close_socket();


}



