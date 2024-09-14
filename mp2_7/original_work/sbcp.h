#ifndef SBCP_H
#define SBCP_H

#include <stdint.h>

/* SBCP Header Types */
#define SBCP_HDR_JOIN   2
#define SBCP_HDR_FWD    3
#define SBCP_HDR_SEND   4

/* SBCP Attribute Types */
#define SBCP_ATTR_REASON        1
#define SBCP_ATTR_USERNAME      2
#define SBCP_ATTR_CLIENT_COUNT  3
#define SBCP_ATTR_MESSAGE       4

/* SBCP Header */
typedef struct {
    uint16_t  version_type;
    uint16_t  length;
    sbcp_attr *payload;
} sbcp_hdr;

/* SBCP Attribute */
typedef struct {
    uint16_t type;
    uint16_t length;
    uint8_t  *payload;
} sbcp_attr;

/**
 * @brief Returns SBCP header version.
 * 
 * @param hdr The pointer to the SBCP header
 * 
 * @return SBCP header version
 */
uint8_t sbcp_hdr_get_version(sbcp_hdr *hdr) {
    return (hdr->version_type >> 7);
}

/**
 * @brief Returns SBCP header type.
 * 
 * @param hdr The pointer to the SBCP header
 * 
 * @return SBCP header type
 */
uint8_t sbcp_hdr_get_type(sbcp_hdr *hdr) {
    return (hdr->version_type &= 0x007F);
}

#endif /* SBCP_H */
