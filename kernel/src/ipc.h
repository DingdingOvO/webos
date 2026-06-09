#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include "process.h"

/* IPC message types */
#define IPC_MSG_SYSCALL   0x01   /* Syscall request */
#define IPC_MSG_SIGNAL    0x02   /* Signal notification */
#define IPC_MSG_DATA      0x03   /* Data transfer */
#define IPC_MSG_REQUEST   0x04   /* Service request */
#define IPC_MSG_RESPONSE  0x05   /* Service response */
#define IPC_MSG_EVENT     0x06   /* System event */

/* Maximum IPC message size */
#define IPC_MAX_MSG_SIZE  4096

/* Maximum pending messages per process */
#define IPC_MAX_PENDING   32

/* IPC message structure */
typedef struct {
    pid_t sender;
    pid_t receiver;
    uint32_t type;
    uint32_t length;
    uint8_t data[IPC_MAX_MSG_SIZE];
} ipc_msg_t;

/* IPC channel */
typedef struct {
    pid_t owner;
    uint32_t msg_count;
    uint32_t capacity;
    ipc_msg_t messages[IPC_MAX_PENDING];
} ipc_channel_t;

/**
 * Initialize IPC subsystem.
 */
int32_t ipc_init(void);

/**
 * Send an IPC message.
 * 
 * @param target  Target process ID
 * @param type    Message type
 * @param data    Message data
 * @param len     Data length
 * @return        0 on success, negative on error
 */
int32_t ipc_send(pid_t target, uint32_t type, const void* data, uint32_t len);

/**
 * Receive an IPC message (blocking).
 * 
 * @param msg  Output message buffer
 * @return     0 on success, negative on error
 */
int32_t ipc_recv(ipc_msg_t* msg);

/**
 * Receive an IPC message (non-blocking).
 * 
 * @param msg  Output message buffer
 * @return     0 on success, -4 (BUSY) if no messages
 */
int32_t ipc_recv_nonblock(ipc_msg_t* msg);

/**
 * Make a synchronous IPC call (send + wait for response).
 * 
 * @param target  Target process
 * @param type    Request type
 * @param data    Request data
 * @param len     Request length
 * @param resp    Response buffer
 * @param resp_len  Response buffer size
 * @return        0 on success, negative on error
 */
int32_t ipc_call(pid_t target, uint32_t type,
                  const void* data, uint32_t len,
                  void* resp, uint32_t* resp_len);

/**
 * Reply to a synchronous IPC call.
 * 
 * @param target  Process to reply to
 * @param data    Response data
 * @param len     Response length
 * @return        0 on success, negative on error
 */
int32_t ipc_reply(pid_t target, const void* data, uint32_t len);

#endif /* IPC_H */
