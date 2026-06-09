#ifndef INPUT_DRIVER_H
#define INPUT_DRIVER_H

/* Event types */
#define INPUT_EVENT_KEY_DOWN   1
#define INPUT_EVENT_KEY_UP     2
#define INPUT_EVENT_MOUSE_DOWN 3
#define INPUT_EVENT_MOUSE_UP   4
#define INPUT_EVENT_MOUSE_MOVE 5
#define INPUT_EVENT_WHEEL      6

typedef struct {
    int type;
    int key;        /* Key code for keyboard events */
    int x, y;       /* Mouse position */
    int buttons;     /* Mouse button state */
    int delta;       /* Wheel delta */
} input_event_t;

int input_driver_init(void);
int input_driver_poll_event(input_event_t* event);
void input_driver_push_event(const input_event_t* event);
int input_driver_has_events(void);

#endif
