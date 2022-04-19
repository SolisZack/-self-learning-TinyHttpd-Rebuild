#include <iostream>
#include "httpd_handler.h"
#include "httpd.h"

int main() {
    Httpd* httpd = new Httpd;
    u_short port = 8081;
    printf("starting up httpd at port:%d\n", port);
    httpd->start_up(port);

}
