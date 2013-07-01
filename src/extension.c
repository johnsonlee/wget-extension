#include "wget.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "utils.h"
#include "extension.h"

#define WGET_EXTENSION_DIR ".wget"

#define WGET_EXTENSION_HOOK_DIR WGET_EXTENSION_DIR"/hooks"

#define WGET_EXTENSION_START_HOOK            WGET_EXTENSION_HOOK_DIR"/on-start"
#define WGET_EXTENSION_PREPARE_DOWNLOAD_HOOK WGET_EXTENSION_HOOK_DIR"/on-prepare-download"
#define WGET_EXTENSION_START_DOWNLOAD_HOOK   WGET_EXTENSION_HOOK_DIR"/on-start-download"
#define WGET_EXTENSION_FINISH_DOWNLOAD_HOOK  WGET_EXTENSION_HOOK_DIR"/on-finish-download"
#define WGET_EXTENSION_EXIT_HOOK             WGET_EXTENSION_HOOK_DIR"/on-exit"

typedef int (*extension_event_handler_t)(const char *, const char *);

struct extension_event_registry {
    int event;
    extension_event_handler_t handler;
};

static int extension_execute(int argc, const char *argv[])
{
    struct stat buf;
    const char *envp[] = { NULL };

    logprintf(LOG_VERBOSE, "Trigger hook `%s' %s %s\n", argv[0], argv[1], argv[2]);

    if (stat(argv[0], &buf)) {
        if (ENOENT == errno)
            return 0;

        fprintf(stderr, "Access `%s' failed: %s\n", argv[0], strerror(errno));
        return -1;
    }

    if (!S_ISREG(buf.st_mode)) {
        fprintf(stderr, "%s isn't a file\n", argv[0]);
        return 0;
    }

    return execve(argv[0], argv, envp);
}

static __attribute__((constructor)) extension_fire_start_event()
{
    const char *argv[] = { WGET_EXTENSION_START_HOOK };
 
    (void) extension_execute(sizeof(argv) / sizeof(argv[0]), argv);
}

static int extension_fire_prepare_download(const char *url, const char *file)
{
    const char *argv[] = { WGET_EXTENSION_PREPARE_DOWNLOAD_HOOK, url, file };

    return extension_execute(sizeof(argv) / sizeof(argv[0]), argv);
}

static int extension_fire_start_download(const char *url, const char *file)
{
    const char *argv[] = { WGET_EXTENSION_START_DOWNLOAD_HOOK, url, file };

    return extension_execute(sizeof(argv) / sizeof(argv[0]), argv);
}

static int extension_fire_finish_download(const char *url, const char *file)
{
    const char *argv[] = { WGET_EXTENSION_FINISH_DOWNLOAD_HOOK, url, file };

    return extension_execute(sizeof(argv) / sizeof(argv[0]), argv);
}

static __attribute__((destructor)) extension_fire_exit_event()
{
    const char *argv[] = { WGET_EXTENSION_EXIT_HOOK };

    (void) extension_execute(sizeof(argv) / sizeof(argv[0]), argv);
}

int extension_init()
{
    // TODO initialize extension environment
    return 0;
}

int extension_fire_event(int evt, const char *url, const char *file)
{
    assert(evt >= WGET_EVENT_START && evt <= WGET_EVENT_EXIT);

    int status;
    pid_t pid;
    struct stat buf;

    if (stat(WGET_EXTENSION_DIR, &buf)) {
        if (ENOENT == errno)
            return 0;

        fprintf(stderr, "Access %s failed\n", WGET_EXTENSION_DIR);
        return -1;
    }

    if (!S_ISDIR(buf.st_mode)) {
        fprintf(stderr, "%s isn't a directory\n", WGET_EXTENSION_DIR);
        return 0;
    }

    switch (pid = fork()) {
    case -1:
        perror("Unable to create child process");
        return -1;
    case 0:
        logprintf(LOG_VERBOSE, "Fire event %d\n", evt);

        switch (evt) {
        case WGET_EVENT_START:
            extension_fire_start_event();
            break;
        case WGET_EVENT_PREPARE_DOWNLOAD:
            _exit(extension_fire_prepare_download(url, file));
            break;
        case WGET_EVENT_START_DOWNLOAD:
            _exit(extension_fire_start_download(url, file));
            break;
        case WGET_EVENT_FINISH_DOWNLOAD:
            _exit(extension_fire_finish_download(url, file));
            break;
        case WGET_EVENT_EXIT:
            extension_fire_exit_event();
            break;
        default:
            break;
        }
        _exit(0);
    default:
        if (-1 == waitpid(pid, &status, 0)) {
            perror("Fetal error");
            abort();
        }

        return status;
    }

}


