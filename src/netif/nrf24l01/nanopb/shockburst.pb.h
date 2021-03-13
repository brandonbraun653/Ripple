/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.4 */

#ifndef PB_SHOCKBURST_PB_H_INCLUDED
#define PB_SHOCKBURST_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _ShockBurstResponse {
    uint64_t sender;
    bool ack;
    uint32_t packet_id;
} ShockBurstResponse;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define ShockBurstResponse_init_default          {0, 0, 0}
#define ShockBurstResponse_init_zero             {0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define ShockBurstResponse_sender_tag            1
#define ShockBurstResponse_ack_tag               2
#define ShockBurstResponse_packet_id_tag         3

/* Struct field encoding specification for nanopb */
#define ShockBurstResponse_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT64,   sender,            1) \
X(a, STATIC,   REQUIRED, BOOL,     ack,               2) \
X(a, STATIC,   REQUIRED, UINT32,   packet_id,         3)
#define ShockBurstResponse_CALLBACK NULL
#define ShockBurstResponse_DEFAULT NULL

extern const pb_msgdesc_t ShockBurstResponse_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define ShockBurstResponse_fields &ShockBurstResponse_msg

/* Maximum encoded size of messages (where known) */
#define ShockBurstResponse_size                  19

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
