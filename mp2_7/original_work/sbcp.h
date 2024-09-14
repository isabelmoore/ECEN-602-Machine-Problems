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

/* SBCP Message */
typedef struct {
    uint16_t  version_type;
    uint16_t  length;
    sbcp_attr *payload;
} sbcp_msg;

/* SBCP Attribute */
typedef struct {
    uint16_t type;
    uint16_t length;
    uint8_t  *payload;
} sbcp_attr;

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
    msg->version_type |= version << 7;
}

/**
 * @brief Sets SBCP message type.
 * 
 * @param msg The pointer to the SBCP message
 */
void sbcp_msg_set_type(sbcp_msg *msg, uint16_t type) {
    msg->version_type |= type;
}

#endif /* SBCP_H */
