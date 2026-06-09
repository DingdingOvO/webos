/**
 * WebOS Input Driver (.Wdll)
 * 
 * Captures keyboard and mouse events from the browser,
 * maintains an event queue, and provides polling API for processes.
 */

#include "input_driver.h"
#include <emscripten.h>

/* JS host functions */
extern void js_input_subscribe(int callback_id);
extern int js_input_get_event(void* event_ptr);

#define MAX_EVENT_QUEUE 128

static input_event_t event_queue[MAX_EVENT_QUEUE];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;

EMSCRIPTEN_KEEPALIVE
int input_driver_init(void) {
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;
    js_input_subscribe(1);
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int input_driver_poll_event(input_event_t* event) {
    /* Try to get event from JS */
    int got = js_input_get_event(event);
    if (got > 0) return 1;
    
    /* Check local queue */
    if (queue_count == 0) return 0;
    
    *event = event_queue[queue_head];
    queue_head = (queue_head + 1) % MAX_EVENT_QUEUE;
    queue_count--;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
void input_driver_push_event(const input_event_t* event) {
    if (queue_count >= MAX_EVENT_QUEUE) return;
    event_queue[queue_tail] = *event;
    queue_tail = (queue_tail + 1) % MAX_EVENT_QUEUE;
    queue_count++;
}

EMSCRIPTEN_KEEPALIVE
int input_driver_has_events(void) {
    return queue_count > 0 ? 1 : 0;
}
