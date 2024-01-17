/*
    Copyright 2024 SAP SE

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <uuid/uuid.h>

#include "sdw.h"

#define LOG(...) {fprintf(stderr, "[%s:%d] %s: ", __FILE__, __LINE__, __func__); fprintf(stderr, __VA_ARGS__);}
#define LOG_ERRNO LOG(strerror(errno));

typedef enum {
    EV_LOOP, EV_CREATE_SCOPE
} events_t;

int create_childprocesses();
void move_to_scope();


pid_t *pids;
int pids_length;
int keep_running;
char *unit_name;
events_t state;


static int started_via_systemd;

/* change state */
void sighup_handler(int s) {
    if (s != SIGHUP) return;
    state = EV_CREATE_SCOPE;
}

void sigusr1_handler(int s) {
    if (s == SIGUSR1)
        keep_running = 0;
}


/* Creation of a daemonized process is required. Else there will be
 * warnings when the new MainPID is no child of PID 1.
 */
void move_to_scope() {
    pid_t p, p2;
    int child_rc = 0;
    int pipe_fd[2]; /* MainPID <- Child sends Grandchild PID */

    pids[pids_length-1] = getpid();
    pipe(pipe_fd);
    
    p=fork();
    if (p==0) { /* Child for daemonization */
        close(pipe_fd[0]);
        p2 = fork();
        if (p2 == 0) { /* Grandchild - future MainPID */
            sleep(50); // in reality some proper multiprocess handling 
            sdw_notify_stopping();
            exit(0);
        }
        write(pipe_fd[1], &p2, sizeof(pid_t));
        close(pipe_fd[1]);
        exit(0);
    }

    close(pipe_fd[1]);
    read(pipe_fd[0], &p2, sizeof(pid_t));
    close(pipe_fd[0]);
    /* "move" the child-child to parent pid 1 */
    kill(p, SIGTERM);
    waitpid(p, &child_rc, 0);

#if 0
    /* For manual checking, of process structure.
     * Remember: Grandchild process also sleeps, don't sleep longer */
    sleep(30);
#endif

    sdw_notify_mainpid(p2);
    sdw_start_auxiliary_scope(unit_name, pids_length, pids);
    kill(p2, SIGTERM);
    waitpid(p2, &child_rc, 0);
}

int create_childprocesses() {
    const int nr_children = pids_length-1;
    int i = 0;
    pid_t p = 0;

    for (i=0; i<nr_children; i++) {
        p = fork();
        if (p == 0) {
            sleep(600);
            exit(0);
        } else if (p == -1) {
            LOG_ERRNO;
            return EXIT_FAILURE;
        }
        pids[i] = p;
    }
    return EXIT_SUCCESS;
}

int main(void) {
    int i = 0, rc = 0;
    uuid_t uuid;
    char uuid_string[37];

    state = EV_LOOP;

    sdw_set_tracelevel(1);

    if (sdw_auxiliary_scope_supported() != 0) {
        LOG("This system does not support auxiliary scopes.\n");
        return EXIT_FAILURE;
    }

    if ((getppid() == 1) && (getenv("INVOCATION_ID") != NULL)) {
        started_via_systemd = 1;
    } else {
        started_via_systemd = 0;
    }

    for (i=0; i<2; i++) {
        if (uuid_generate_time_safe(uuid) == 0)
            break;
        LOG("Warning: uuid_generate_time_safe \"failed\" - UUID may not unique\n");
        sleep(1);
    }

    if ((unit_name=(char*)calloc(128,sizeof(char))) == NULL) {
        LOG_ERRNO;
        return EXIT_FAILURE;
    }

    /* systemd syntax: replace - with _ */
    uuid_unparse(uuid, uuid_string);
    uuid_string[8]  = '_';
    uuid_string[13] = '_';
    uuid_string[18] = '_';
    uuid_string[23] = '_';
    snprintf(unit_name, 128, "foobar_%s.scope", uuid_string);
    if (started_via_systemd == 0)
        LOG("target scope name: '%s'\n", unit_name);

    keep_running = 21;
    pids_length = 3;

    if ((pids=(pid_t*)calloc(pids_length, sizeof(pid_t))) == NULL) {
        LOG_ERRNO;
        free(unit_name);
        return EXIT_FAILURE;
    }

    if ((rc = create_childprocesses()) != EXIT_SUCCESS) {
        goto cleanup;
    }

    signal(SIGUSR1, sigusr1_handler);
    signal(SIGHUP, sighup_handler);

    if (started_via_systemd == 1)
        sdw_notify_ready();
    else
        printf("MainPID = %d\n", getpid());

    do {
        if (state == EV_CREATE_SCOPE ) {
            move_to_scope();
            state = EV_LOOP;
        }
        keep_running--;
        sleep(30);
    } while (keep_running > 0 );


cleanup:
    if (started_via_systemd == 1)
        sdw_notify_stopping();
    for (i=0; i<pids_length-1; i++) {
        if (pids[i] != 0)
            kill(pids[i], SIGTERM);
    }
    free(pids);

    return EXIT_SUCCESS;
}

