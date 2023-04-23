#ifndef FOTA_BY_HTTP_H
#define FOTA_BY_HTTP_H

//parse url output <host> <port> <path>
int httpurl_parse(char *url, char **host, short unsigned int *port, char **path);
//ota by http task
void fota_by_http();

#endif
