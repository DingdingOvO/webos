/**
 * WebOS Kernel - Inter-Process Communication
 * 
 * Implements message-based IPC for process communication.
 */

#include "ipc.h"
#include "scheduler.h"
#include "host_func.h"

/* IPC channels per process */
static ipc_channel_t channels[MAX_PROCESSES];

int32_t ipc_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        channels[i].owner = 0;
        channels[i].msg_count = 0;
        channels[i].capacity = IPC_MAX_PENDING;
    }
    return 0;
}

static ipc_channel_t* get_channel(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (channels[i].owner == pid) {
            return &channels[i];
        }
    }
    /* Allocate a channel if none exists */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (channels[i].owner == 0) {
            channels[i].owner = pid;
            channels[i].msg_count = 0;
            return &channels[i];
        }
    }
    return (ipc_channel_t*)0;
}

int32_t ipc_send(pid_t target, uint32_t type, const void* data, uint32_t len) {
    if (len > IPC_MAX_MSG_SIZE) return -1;  /* ERR_INVAL */

    ipc_channel_t* ch = get_channel(target);
    if (!ch) return -2;  /* ERR_NOENT */

    if (ch->msg_count >= ch->capacity) return -4;  /* ERR_BUSY */

    /* Add message to channel */
    ipc_msg_t* msg = &ch->messages[ch->msg_count];
    msg->sender = process_getpid();
    msg->receiver = target;
    msg->type = type;
    msg->length = len;

    if (data && len > 0) {
        for (uint32_t i = 0; i < len; i++) {
            msg->data[i] = ((const uint8_t*)data)[i];
        }
    }

    ch->msg_count++;

    /* Unblock the target if it's waiting */
    scheduler_unblock(target);

    return 0;
}

int32_t ipc_recv(ipc_msg_t* msg) {
    /* Blocking receive */
    while (1) {
        int32_t result = ipc_recv_nonblock(msg);
        if (result == 0) return 0;
        scheduler_block();
    }
}

int32_t ipc_recv_nonblock(ipc_msg_t* msg) {
    pid_t pid = process_getpid();
    ipc_channel_t* ch = get_channel(pid);
    if (!ch || ch->msg_count == 0) return -4;  /* ERR_BUSY */

    /* Copy the first message */
    if (msg) {
        *msg = ch->messages[0];
    }

    /* Shift remaining messages */
    for (uint32_t i = 1; i < ch->msg_count; i++) {
        ch->messages[i - 1] = ch->messages[i];
    }
    ch->msg_count--;

    return 0;
}

int32_t ipc_call(pid_t target, uint32_t type,
                  const void* data, uint32_t len,
                  void* resp, uint32_t* resp_len) {
    /* Send request */
    int32_t result = ipc_send(target, type, data, len);
    if (result != 0) return result;

    /* Wait for response */
    ipc_msg_t response;
    result = ipc_recv(&response);
    if (result != 0) return result;

    /* Copy response data */
    if (resp && resp_len) {
        uint32_t copy_len = response.length;
        if (copy_len > *resp_len) copy_len = *resp_len;
        for (uint32_t i = 0; i < copy_len; i++) {
            ((uint8_t*)resp)[i] = response.data[i];
        }
        *resp_len = copy_len;
    }

    return 0;
}

int32_t ipc_reply(pid_t target, const void* data, uint32_t len) {
    return ipc_send(target, IPC_MSG_RESPONSE, data, len);
}
