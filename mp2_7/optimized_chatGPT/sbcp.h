#ifndef SBCP_H
#define SBCP_H

#include <stdint.h>

/* SBCP Message Types */
#define SBCP_MSG_JOIN   2
#define SBCP_MSG_FWD    3
#define SBCP_MSG_SEND   4

/* SBCP Attribute Types */
#define SBCP_ATTR_REASON        1
#define SBCP_ATTR_USERNAME      2
#define SBCP_ATTR_CLIENT_COUNT  3
#define SBCP_ATTR_MESSAGE       4

// Forward declaration
typedef struct sbcp_attr sbcp_attr;

/* SBCP Attribute */
struct sbcp_attr {
    uint16_t type;
    uint16_t length;
    uint8_t  *payload;
};

/* SBCP Message */
typedef struct {
    uint16_t  version_type;
    uint16_t  length;
    sbcp_attr *payload;  
} sbcp_msg;

/* Header structure */
typedef struct {
    int version;
    int type;
} Header;

/* MessageAttribute structure */
typedef struct {
    int type;
    int length;
    char payload[512];
} MessageAttribute;

/* Message structure */
typedef struct {
    Header header;
    MessageAttribute attribute[10];
} Message;

/* infoClient structure: client username, socket file descriptor, client count */
typedef struct infoClient {
    char username[100];
    int fd;
    int clientCount;
} infoClient;


/**
 * @brief Returns SBCP message version.
 * 
 * @param msg The pointer to the SBCP message
 * 
 * @return SBCP message version
 */
uint16_t sbcp_msg_get_version(sbcp_msg *msg) {
    return (msg->version_type >> 7);
}

/**
 * @brief Returns SBCP message type.
 * 
 * @param msg The pointer to the SBCP message
 * 
 * @return SBCP message type
 */
uint16_t sbcp_msg_get_type(sbcp_msg *msg) {
    return (msg->version_type &= 0x007F);
}

/**
 * @brief Sets SBCP message version.
 * 
 * @param msg The pointer to the SBCP message
 */
void sbcp_msg_set_version(sbcp_msg *msg, uint16_t version) {
    msg->version_type &= 0x007F;  // Clear version bits
    msg->version_type |= (version << 7);  // Set new version
}

/**
 * @brief Sets SBCP message type.
 * 
 * @param msg The pointer to the SBCP message
 */
void sbcp_msg_set_type(sbcp_msg *msg, uint16_t type) {
    msg->version_type &= 0xFF80;  // Clear type bits
    msg->version_type |= (type & 0x007F);  // Set new type ensuring it's within bounds
}

#endif // SBCP_H
