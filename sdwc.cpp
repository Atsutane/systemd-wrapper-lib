/*
    Copyright 2023 SAP SE

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

#include "sdw.h"

extern char *optarg;
extern int opterr;

struct {
    char *unit_name = NULL;
    unsigned pid = 0;
    int trc_level = 1;
    unsigned wait_sec = 0;
} cfg;

static void usage(void) {
    printf("usage:\n"
           "    Start -u <UNIT> [-w <WAIT_SECONDS>]\n"
           "    Restart -u <UNIT> [-w <WAIT_SECONDS>]\n"
           "    Stop -u <UNIT> [-w <WAIT_SECONDS>]\n"
           "    GetUnitByPID -p <PID>\n"
           "    GetMainPID -u <UNIT>\n"
           "    CheckPID -p <PID> -u <UNIT>\n"
           "    CheckControlPID -p <PID> -u <UNIT>\n"
           "    GetActiveState -u <UNIT>\n"
           "    GetSubState -u <UNIT>\n"
           "    GetUnitFileState -u <UNIT>\n"
           "    IsSupported\n"
           "    GetVersion\n"
           "    Encode -u <UNIT-ENCODED>\n"
           "    Decode -u <UNIT>\n"
           "    Enable -u <UNIT>\n"
           "    Disable -u <UNIT>\n"
           "    Reload\n"
           "    # valid for all commands:\n"
           "      [-v <0-2>]    # verbose (ERROR, INFO, DEBUG)\n");
    exit(1);
}

static int my_getopt(int ac, char **av, const char *opt) {
    int c;

    opterr = 0;

    while ((c = getopt(ac, av, opt)) != -1) {
        switch (c) {
            case 'p':
                {
                    cfg.pid = (unsigned) atoi(optarg);
                    break;
                }
            case 'u':
                {
                    cfg.unit_name = strdup(optarg);
                    break;
                }
            case 'v':
                {
                    cfg.trc_level = atoi(optarg);
                    break;
                }
            case 'w':
                {
                    cfg.wait_sec = (unsigned) atoi(optarg);
                    break;
                }
            default:
                {
                    usage();
                    break;
                }
        }
    }

    sdw_set_tracelevel(cfg.trc_level);

    return 0;
}

// map rc to valid range of OS
static inline int map_rc(int rc) {
    return abs(rc) & 0xff;
}

int main(int argc, char **argv) {
    int rc;

    if (argc < 2 ||
        strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        usage();

    argc--;
    argv++;

    if (strcmp(argv[0], "Start") == 0) {
        if (my_getopt(argc, argv, "u:w:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        if (cfg.wait_sec)
            rc = sdw_start(cfg.unit_name, cfg.wait_sec);
        else
            rc = sdw_start(cfg.unit_name);

        if (0 == rc)
            printf("started '%s'\n", cfg.unit_name);
        else
            printf("Start '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "Stop") == 0) {
        if (my_getopt(argc, argv, "u:w:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        if (cfg.wait_sec)
            rc = sdw_stop(cfg.unit_name, cfg.wait_sec);
        else
            rc = sdw_stop(cfg.unit_name);

        if (0 == rc)
            printf("stopped '%s'\n", cfg.unit_name);
        else
            printf("Stop '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    }
    if (strcmp(argv[0], "Restart") == 0) {
        if (my_getopt(argc, argv, "u:w:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        if (cfg.wait_sec)
            rc = sdw_restart(cfg.unit_name, cfg.wait_sec);
        else
            rc = sdw_restart(cfg.unit_name);

        if (0 == rc)
            printf("restarted '%s'\n", cfg.unit_name);
        else
            printf("Restart '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    }
    if (strcmp(argv[0], "GetVersion") == 0) {
        char *version = NULL;

        if (my_getopt(argc, argv, "v:") != 0)
            usage();

        rc = sdw_get_version(&version);
        if (0 == rc)
            printf("version: '%s'\n", version);
        else
            printf("GetVersion failed (rc=%d)\n", rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "GetUnitByPID") == 0) {
        if (my_getopt(argc, argv, "p:v:") != 0)
            usage();

        if (0 == cfg.pid)
            usage();

        rc = sdw_get_unit_by_pid(cfg.pid, &cfg.unit_name);
        if (0 == rc)
            printf("found unit '%s' for pid '%u' (rc=%d)\n", cfg.unit_name,
                   cfg.pid, rc);
        else
            printf("GetUnitByPID '%u' failed (rc=%d)\n", cfg.pid, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "CheckPID") == 0) {
        if (my_getopt(argc, argv, "p:u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        if (0 == cfg.pid)
            cfg.pid = getpid();

        rc = sdw_check_pid(cfg.unit_name, cfg.pid);
        if (0 == rc)
            printf("found unit '%s' for pid '%u'\n", cfg.unit_name, cfg.pid);
        else
            printf("CheckPid '%s' pid '%u' failed (rc=%d)\n",
                   cfg.unit_name, cfg.pid, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "CheckControlPID") == 0) {
        if (my_getopt(argc, argv, "p:u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_check_controlpid(cfg.unit_name, cfg.pid);
        if (0 == rc)
            printf("found unit '%s' for control pid '%u'\n", cfg.unit_name,
                   cfg.pid);
        else
            printf("CheckControlPID '%s' pid '%u' failed (rc=%d)\n",
                   cfg.unit_name, cfg.pid, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "GetMainPID") == 0) {
        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_get_mainpid(cfg.unit_name, &cfg.pid);
        if (0 == rc)
            printf("mainPID: '%u'\n", cfg.pid);
        else
            printf("GetMainPID '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "GetActiveState") == 0) {
        char *state = NULL;

        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_get_activestate(cfg.unit_name, &state);
        if (rc > 0 && NULL != state)
            printf("ActiveState: %d '%s'\n", rc, state);
        else
            printf("GetActiveState '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "GetSubState") == 0) {
        char *state = NULL;

        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_get_substate(cfg.unit_name, &state);
        if (rc > 0 && NULL != state)
            printf("SubState: %d '%s'\n", rc, state);
        else
            printf("GetSubState '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "GetUnitFileState") == 0) {
        char *state = NULL;
        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_get_unitfilestate(cfg.unit_name, &state);
        if (rc > 0 && NULL != state)
            printf("UnitFileState: %d '%s'\n", rc, state);
        else
            printf("GetUnitFileState '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "IsSupported") == 0) {
        if (my_getopt(argc, argv, "v:") != 0)
            usage();

        rc = sdw_is_supported();
        printf("systemd version is%s supported\n", 0 == rc ? "" : " not");

        return map_rc(rc);
    } else if (strcmp(argv[0], "Encode") == 0) {
        char *encoded = NULL;

        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_encode(cfg.unit_name, &encoded);
        if (0 == rc)
            printf("encoded: '%s'\n", encoded);

        return map_rc(rc);
    } else if (strcmp(argv[0], "Decode") == 0) {
        char *decoded = NULL;

        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_decode(cfg.unit_name, &decoded);
        if (0 == rc)
            printf("decoded: '%s'\n", decoded);

        return map_rc(rc);
    } else if (strcmp(argv[0], "Enable") == 0) {
        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_enable(cfg.unit_name);
        if (0 == rc)
            printf("enabled '%s'\n", cfg.unit_name);
        else
            printf("Enable '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "Disable") == 0) {
        if (my_getopt(argc, argv, "u:v:") != 0)
            usage();

        if (NULL == cfg.unit_name)
            usage();

        rc = sdw_disable(cfg.unit_name);
        if (0 == rc)
            printf("disabled '%s'\n", cfg.unit_name);
        else
            printf("Disable '%s' failed (rc=%d)\n", cfg.unit_name, rc);

        return map_rc(rc);
    } else if (strcmp(argv[0], "Reload") == 0) {
        if (my_getopt(argc, argv, "v:") != 0)
            usage();

        rc = sdw_reload();
        if (0 == rc)
            printf("reloaded units\n");
        else
            printf("Reload failed (rc=%d)\n", rc);

        return map_rc(rc);
    }

    usage();

    return 1;
}
