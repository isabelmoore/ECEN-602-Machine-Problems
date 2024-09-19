#ifndef TYPES_H
#define TYPES_H

typedef struct {
    int version;
    int type;
} Header;

typedef struct {
    int type;
    int length;
    char payload[512];
} MessageAttribute;

typedef struct {
    Header header;
    MessageAttribute attribute[10];
} Message;

typedef struct InfoCli {
    char username[100];
    int fd;
    int NoofClients;
} InfoCli;

#endif // TYPES_H
