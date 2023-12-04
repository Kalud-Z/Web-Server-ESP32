// websocket.h
#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "esp_http_server.h" 

// Declare the functions that will be called from other files
esp_err_t ws_handler(httpd_req_t *req);
void start_webserver(void);


#endif // WEBSOCKET_H
