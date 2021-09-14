#include <iostream>
#include "httpd_handler.h"
#include "httpd.h"

int main() {
    Httpd httpd;
    u_short port = 8080;
    printf("starting up HTTPD at port:%d\n", port);
    httpd.start_up(port);
}
