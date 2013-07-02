#ifndef EXTENSION_H
#define EXTENSION_H

enum {
    WGET_EVENT_START,
    WGET_EVENT_PREPARE_DOWNLOAD,
    WGET_EVENT_START_DOWNLOAD,
    WGET_EVENT_FINISH_DOWNLOAD,
    WGET_EVENT_EXIT,
};

int extension_init();

int extension_fire_event(int evt, ...);

#endif /* EXTENSION_H */
