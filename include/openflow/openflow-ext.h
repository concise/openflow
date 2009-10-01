/*
 * Copyright (c) 2007, 2008 The Board of Trustees of The Leland
 * Stanford Junior University
 */


#ifndef OPENFLOW_OPENFLOW_EXT_H
#define OPENFLOW_OPENFLOW_EXT_H 1

#include "openflow/openflow.h"

/*
 * The following are vendor extensions from OpenFlow.  This is a 
 * means of allowing the introduction of non-standardized 
 * proposed code.
 *
 * Structures in this file are 32-bit aligned in size.
 */

#define OPENFLOW_VENDOR_ID 0x000026e1

/****************************************************************
 *
 * OpenFlow Queue Configuration Operations
 *
 ****************************************************************/

enum openflow_queue_commands { /* Queue configuration commands */
    /* Add, modify, delete queue operations */
    OFQ_CMD_ADD,
    OFQ_CMD_MODIFY,
    OFQ_CMD_DELETE,

    /* Response to one of the above */
    OFQ_CMD_REPLY
};

struct openflow_queue_header {
    struct ofp_header header;
    uint32_t vendor;            /* OPENFLOW_VENDOR_ID. */
    uint32_t subtype;           /* openflow_queue_commands value */
};
OFP_ASSERT(sizeof(struct openflow_queue_header) == sizeof(ofp_header) + 8);

struct openflow_queue_delete {
    uint16_t port;              /* Port addressed */
    uint8_t pad[2];
    uint32_t queue_id;          /* Queue addressed */
};
OFP_ASSERT(sizeof(struct openflow_queue_delete) == 8);

/* Currently, only Min Rate is supported as a queuing discipline */
enum openflow_queue_disciplines {
    OFQ_DSP_MIN_RATE,           /* Minimum rate guarantee */
    OFQ_DSP_COUNT               /* Last please */
};

struct openflow_queue_min_rate {
    uint16_t min_rate;
    uint8_t pad[2];
};
OFP_ASSERT(sizeof(struct openflow_queue_min_rate) == 4);

typedef union ofq_discipline_param_u {
    struct openflow_queue_min_rate min_rate;
} ofq_discipline_param_t;
OFP_ASSERT(sizeof(ofq_discipline_param_t) == 4);

struct openflow_queue_discipline {
    uint16_t discipline;
    uint8_t pad[2];
    ofq_discipline_param_t param;
}
OFP_ASSERT(sizeof(struct openflow_queue_discipline) == 8);

struct openflow_queue_discipline_set {
    uint16_t port;              /* Port addressed */
    uint8_t pad[2];
    uint32_t queue_id;          /* Queue addressed */
    /* Array length inferred from header length */
    struct openflow_queue_discipline discipline[0];
};
OFP_ASSERT(sizeof(struct openflow_queue_discipline_set) == 8);

#define openflow_queue_add openflow_queue_discipline_set
#define openflow_queue_modify openflow_queue_discipline_set

enum openflow_queue_error {
    OFQ_ERR_NONE,               /* Success */
    OFQ_ERR_FAIL,               /* Unspecified failure */
    OFQ_ERR_NOT_FOUND,          /* Queue not found */
    OFQ_ERR_DISCIPLINE,         /* Discipline not supported */
    OFQ_ERR_BW_UNAVAIL,         /* Bandwidth unavailable */
    OFQ_ERR_QUEUE_UNAVAIL,      /* Queue unavailable */
    OFQ_ERR_COUNT               /* Last please */
}

/* Used for delete */
struct openflow_queue_reply {
    uint16_t port;              /* Port addressed */
    uint8_t pad[2];
    uint32_t queue_id;          /* Queue addressed */
    uint32_t return_code;       /* OFQ_ERR_ success/error indication */
};
OFP_ASSERT(sizeof(struct openflow_queue_reply) == 12);

#define OPENFLOW_QUEUE_ERROR_STRINGS_DEF {                      \
    "Success",                  /* OFQ_ERR_NONE */              \
    "Unspecified failure",      /* OFQ_ERR_FAIL */              \
    "Queue not found",          /* OFQ_ERR_NOT_FOUND */         \
    "Discipline not supported", /* OFQ_ERR_DISCIPLINE */        \
    "Bandwidth unavailable",    /* OFQ_ERR_BW_UNAVAIL */        \
    "Queue unavailable"         /* OFQ_ERR_QUEUE_UNAVAIL */     \
}

extern char *openflow_queue_error_strings[];

#define ofq_error_string(rv) (((rv) < OFQ_ERR_COUNT) && ((rv) >= 0) ? \
    openflow_queue_error_strings[rv] : "Unknown error code")


#endif /* OPENFLOW_OPENFLOW_EXT_H */
