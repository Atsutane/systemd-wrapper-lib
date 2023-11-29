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
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#include <regex.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-daemon.h>

#include "sdw.h"

#define MAX_UNIT_NAME_LEN       64
#define MAX_RESPONSE_LEN        256

#define LOG_DEBUG(fmt, ...)                                             \
    do {                                                                \
      if (2 <= trc_level) {                                             \
        fprintf(stdout, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__);       \
      }                                                                 \
    } while (0)

#define LOG_INFO(fmt, ...)                                              \
    do {                                                                \
      if (1 <= trc_level) {                                             \
        fprintf(stdout, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__);       \
      }                                                                 \
    } while (0)

#define LOG_ERROR(fmt, ...)                                     \
    do {                                                        \
        last_error_msg[0] = '\0';                               \
        snprintf(last_error_msg, sizeof(last_error_msg),        \
                  "%s: " fmt, __FUNCTION__, ##__VA_ARGS__);     \
        fprintf(stderr, last_error_msg);                        \
    } while (0)

typedef struct {
    char name[MAX_UNIT_NAME_LEN];
    char encoded[MAX_UNIT_NAME_LEN];
} unit_t;

// internal types are named sdbus instead of sd_bus
typedef enum {
    SDBUS_START_UNIT = 0,
    SDBUS_RESTART_UNIT,
    SDBUS_STOP_UNIT
} sdbus_cmd_t;

typedef enum {
    JOB_UNKNOWN = 0,
    JOB_DONE,
    JOB_FAILED
} job_status_t;

typedef struct {
    sdbus_cmd_t cmd;
    job_status_t status;
    time_t ts_end;
    unsigned wait_sec;
    sd_bus_slot *slot;
    char *path;                 // /org/freedesktop/systemd1/job/993490
    char *result;
} job_info_t;

typedef union {
    char *s;
    unsigned u;
    int d;
} response_t;

#ifdef SDW_DLSYM
// sd_bus function declarations
typedef int (*fn_sd_bus_open_system_t)
 (sd_bus ** ret);

typedef int (*fn_sd_bus_call_method_t)
 (sd_bus * bus,
  const char *destination,
  const char *path,
  const char *interface,
  const char *member,
  sd_bus_error * ret_error, sd_bus_message ** reply, const char *types, ...);

typedef int (*fn_sd_bus_message_read_t)
 (sd_bus_message * m, const char *types, ...);

typedef sd_bus_message *(*fn_sd_bus_message_unref_t)
 (sd_bus_message * m);

typedef void (*fn_sd_bus_error_free_t)
 (sd_bus_error * e);

typedef int (*fn_sd_bus_get_property_t)
 (sd_bus * bus,
  const char *destination,
  const char *path,
  const char *interface,
  const char *member,
  sd_bus_error * ret_error, sd_bus_message ** reply, const char *type);

typedef int (*fn_sd_bus_path_encode_t)
 (const char *prefix, const char *external_id, char **ret_path);

typedef int (*fn_sd_bus_path_decode_t)
 (const char *path, const char *prefix, char **ret_external_id);

typedef int (*fn_sd_notify_t)
 (int unset_environment, const char *state);

typedef int (*fn_sd_bus_cal_method_t)
 (sd_bus ** ret);

typedef int (*fn_sd_bus_add_match_t)
 (sd_bus * bus,
  sd_bus_slot ** slot,
  const char *match, sd_bus_message_handler_t callback, void *userdata);

typedef int (*fn_sd_bus_message_handler_t)
 (sd_bus_message * m, void *userdata, sd_bus_error * ret_error);

typedef sd_bus_slot *(*fn_sd_bus_slot_unref_t)
 (sd_bus_slot * slot);

typedef int (*fn_sd_bus_wait_t)
 (sd_bus * bus, uint64_t timeout_usec);

typedef int (*fn_sd_bus_process_t)
 (sd_bus * bus, sd_bus_message ** ret);

typedef int (*fn_sd_bus_message_enter_container_t)
 (sd_bus_message * m, char type, const char *contents);

typedef int (*fn_sd_bus_message_exit_container_t)
 (sd_bus_message * m);

// sd_bus function pointer
static fn_sd_bus_add_match_t fn_sd_bus_add_match;
static fn_sd_bus_call_method_t fn_sd_bus_call_method;
static fn_sd_bus_error_free_t fn_sd_bus_error_free;
static fn_sd_bus_get_property_t fn_sd_bus_get_property;
static fn_sd_bus_message_read_t fn_sd_bus_message_read;
static fn_sd_bus_message_unref_t fn_sd_bus_message_unref;
static fn_sd_bus_open_system_t fn_sd_bus_open_system;
static fn_sd_bus_path_decode_t fn_sd_bus_path_decode;
static fn_sd_bus_path_encode_t fn_sd_bus_path_encode;
static fn_sd_bus_process_t fn_sd_bus_process;
static fn_sd_bus_slot_unref_t fn_sd_bus_slot_unref;
static fn_sd_bus_wait_t fn_sd_bus_wait;
static fn_sd_notify_t fn_sd_notify;
static fn_sd_bus_message_enter_container_t fn_sd_bus_message_enter_container;
static fn_sd_bus_message_exit_container_t fn_sd_bus_message_exit_container;

#define FN_SD_BUS_ADD_MATCH fn_sd_bus_add_match
#define FN_SD_BUS_CALL_METHOD fn_sd_bus_call_method
#define FN_SD_BUS_ERROR_FREE fn_sd_bus_error_free
#define FN_SD_BUS_GET_PROPERTY fn_sd_bus_get_property
#define FN_SD_BUS_MESSAGE_READ fn_sd_bus_message_read
#define FN_SD_BUS_MESSAGE_UNREF fn_sd_bus_message_unref
#define FN_SD_BUS_OPEN_SYSTEM fn_sd_bus_open_system
#define FN_SD_BUS_PATH_DECODE fn_sd_bus_path_decode
#define FN_SD_BUS_PATH_ENCODE fn_sd_bus_path_encode
#define FN_SD_BUS_PROCESS fn_sd_bus_process
#define FN_SD_BUS_SLOT_UNREF fn_sd_bus_slot_unref
#define FN_SD_BUS_WAIT fn_sd_bus_wait
#define FN_SD_NOTIFY fn_sd_notify
#define FN_SD_BUS_MESSAGE_ENTER_CONTAINER fn_sd_bus_message_enter_container
#define FN_SD_BUS_MESSAGE_EXIT_CONTAINER fn_sd_bus_message_exit_container

#else

#define FN_SD_BUS_ADD_MATCH sd_bus_add_match
#define FN_SD_BUS_CALL_METHOD sd_bus_call_method
#define FN_SD_BUS_ERROR_FREE sd_bus_error_free
#define FN_SD_BUS_GET_PROPERTY sd_bus_get_property
#define FN_SD_BUS_MESSAGE_READ sd_bus_message_read
#define FN_SD_BUS_MESSAGE_UNREF sd_bus_message_unref
#define FN_SD_BUS_OPEN_SYSTEM sd_bus_open_system
#define FN_SD_BUS_PATH_DECODE sd_bus_path_decode
#define FN_SD_BUS_PATH_ENCODE sd_bus_path_encode
#define FN_SD_BUS_PROCESS sd_bus_process
#define FN_SD_BUS_SLOT_UNREF sd_bus_slot_unref
#define FN_SD_BUS_WAIT sd_bus_wait
#define FN_SD_NOTIFY sd_notify
#define FN_SD_BUS_MESSAGE_ENTER_CONTAINER sd_bus_message_enter_container
#define FN_SD_BUS_MESSAGE_EXIT_CONTAINER sd_bus_message_exit_container

#endif

static const char *sdbus_lib_name = "libsystemd.so.0";
static const char sdbus_service_contact[] = "org.freedesktop.systemd1";
static const char sdbus_object_path[] = "/org/freedesktop/systemd1";
static const char sdbus_interface_mgr[] = "org.freedesktop.systemd1.Manager";
static const char sdbus_interface_srv[] = "org.freedesktop.systemd1.Service";
static const char sdbus_interface_unit[] = "org.freedesktop.systemd1.Unit";
static const char sdbus_match[] = "type='signal',"
    "sender='org.freedesktop.systemd1',"
    "interface='org.freedesktop.systemd1.Manager',"
    "member='JobRemoved'," "path='/org/freedesktop/systemd1'";
static const char sdbus_prefix[] = "/test";     // prefix for {en,de}code

static sd_bus *bus = NULL;      // reuse the sd_bus connection
static char last_error_msg[512];
static int trc_level = 0;
static enum { INITIAL = 0, CHECK_VERSION, LOADED, FAILED, INVALID_VERSION
} lib_stat = INITIAL;

/* static functions */
static void sdwi_load_lib(void);
static int sdwi_check_version(const char *version);
static char *sdwi_regex_match(const char *str, const char *pattern,
                                  unsigned want);
static int sdwi_get_unit_by_pid(unsigned pid, char **ret_unit_name);
static int sdwi_sdbus_cmd(const char *unit,
                              char **response, sdbus_cmd_t cmd);
static int sdwi_encode(unit_t *unit);
static int sdwi_decode(unit_t *unit);
static int sdwi_notify(int flag, const char *msg);
static int sdwi_msg_handler(sd_bus_message *msg,
                                void *userdata, sd_bus_error * error);
static int sdwi_job_prepare(job_info_t *job);
static int sdwi_job_wait(job_info_t *job);
static void sdwi_job_remove(job_info_t *job);
static int sdwi_get_srv_property(const char *unit_name,
                                 const char *property,
                                 const char *response_format,
                                 response_t *response);
static int sdwi_get_unit_property(const char *unit_name,
                                  const char *service,
                                  const char *interface,
                                  const char *property,
                                  const char *response_format,
                                  response_t *response);
static int sdwi_get_property(const char *path,
                             const char *service_contact,
                             const char *interface,
                             const char *property,
                             const char *response_format,
                             response_t *response);
static int sdwi_get_unitfilestate(const char *unit_name, char **ret_state);
static int sdwi_get_activestate(const char *unit_name_encoded, char **ret_state);
static int sdwi_get_substate(const char *unit_name_encoded, char **ret_state);
static int sdwi_enable(const char *unit_name, bool runtime, bool force);
static int sdwi_disable(const char *unit_name, bool runtime);

__attribute__((constructor))
static void sdwi_load_lib(void) {
#ifdef SDW_DLSYM
    static void *hdl = NULL;
#endif
    char *error = NULL;
    response_t response;
    int rc;

    response.s = NULL;

    memset(last_error_msg, 0, sizeof(last_error_msg));
#ifdef SDW_DLSYM
    lib_stat = FAILED;
    hdl = dlopen(sdbus_lib_name, RTLD_NOW);
    if (NULL == hdl) {
        LOG_ERROR("can't load %s\n", sdbus_lib_name);
        goto cleanup;
    }
#define DL_FUNCTION(FN)                         \
  do {                                          \
    fn_##FN = (fn_##FN##_t) dlsym(hdl, #FN);    \
    if(NULL == fn_##FN) {                       \
      LOG_ERROR("can't load %s()\n", #FN);      \
      goto cleanup;                             \
    }                                           \
  } while (0)

    DL_FUNCTION(sd_bus_add_match);
    DL_FUNCTION(sd_bus_call_method);
    DL_FUNCTION(sd_bus_error_free);
    DL_FUNCTION(sd_bus_get_property);
    DL_FUNCTION(sd_bus_message_read);
    DL_FUNCTION(sd_bus_message_unref);
    DL_FUNCTION(sd_bus_open_system);
    DL_FUNCTION(sd_bus_path_decode);
    DL_FUNCTION(sd_bus_path_encode);
    DL_FUNCTION(sd_bus_process);
    DL_FUNCTION(sd_bus_slot_unref);
    DL_FUNCTION(sd_bus_wait);
    DL_FUNCTION(sd_notify);
    DL_FUNCTION(sd_bus_message_enter_container);
    DL_FUNCTION(sd_bus_message_exit_container);

#undef DL_FUNCTION
#endif

    /* Connect to the system bus */
    if (NULL == bus) {
        int r = FN_SD_BUS_OPEN_SYSTEM(&bus);
        if (r < 0) {
            LOG_ERROR("failed to connect to systemd D-Bus: %s\n", strerror(-r));
            goto cleanup;
        }
    }

    lib_stat = INVALID_VERSION;

    rc = sdwi_get_property(sdbus_object_path, sdbus_service_contact,
                           sdbus_interface_mgr, "Version", "s", &response);

    if (0 == rc && NULL != response.s) {

        rc = sdwi_check_version(response.s);

        if (0 == rc) {
            LOG_INFO("successfully loaded %s\n", sdbus_lib_name);
            lib_stat = LOADED;
        }

        free(response.s);
        return;
    }

cleanup:
    error = dlerror();
    LOG_ERROR("dlerror: %s\n", NULL == error ? "unknown error" : error);
}

static char *sdwi_regex_match(const char *str, const char *pattern, unsigned want) {
    int rc, start, len;
    char *sub = NULL;
    regex_t re;
    const unsigned max_match = 16;
    regmatch_t m[max_match];

    if (want < 1 || want >= max_match) {
        LOG_ERROR("want %d - invalid value\n", want);
        return NULL;
    }

    rc = regcomp(&re, pattern, REG_EXTENDED);
    if (0 != rc) {
        LOG_ERROR("regcomp() failed - %d\n", rc);
        return NULL;
    }

    rc = regexec(&re, str, max_match, &m[0], 0);

    LOG_DEBUG("regexec('%s', '%s') - %d\n", str, pattern, rc);

    if (0 != rc) {
        LOG_ERROR("regexec() failed - %d\n", rc);
        goto cleanup;
    }

    start = m[want].rm_so;
    len = m[want].rm_eo - start;
    sub = strndup(&str[start], len);
    if (NULL == sub)
        goto cleanup;
    LOG_DEBUG("matched '%s'\n", sub);

cleanup:
    regfree(&re);

    return sub;
}

static int sdwi_check_version(const char *version) {
    int rc = SDW_EVERSION;
    char *match = NULL, *end = NULL;

    if (NULL == version)
        return SDW_EINVAL;

    // use regex to find first number in version string
    match = sdwi_regex_match(version, "^[^0-9]*([0-9]+)", 1);
    if (NULL != match) {

        unsigned num = strtoul(match, &end, 10);

        LOG_DEBUG("systemd version %s\n", version);

        if (end != match) {     // valid number ([0-9]+)
            unsigned min_version = 234; // SLES 15.0 GA 234, RHEL 8.0 GA 239

            if (num >= min_version) {
                LOG_INFO("systemd version %u is supported\n", num);
                rc = 0;
                goto cleanup;
            }
        }
    }

cleanup:
    if (NULL != match)
        free(match);

    return rc;
}

static int sdwi_get_unit_by_pid(unsigned pid, char **ret_unit_name) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;
    int rc = 0;
    char *unit_name = NULL;

    // non alnum() characters are encoded as _xx in the dbus response
    // request the encoded unit name and check the response

    LOG_DEBUG("'%s' '%s' '%s' '%s' '%u'\n",
              sdbus_service_contact, sdbus_object_path,
              sdbus_interface_mgr, "GetUnitByPID", pid);

    rc = FN_SD_BUS_CALL_METHOD(bus, sdbus_service_contact, sdbus_object_path,
                               sdbus_interface_mgr, "GetUnitByPID", &error,
                               &msg, "u", pid);
    if (rc < 0) {
        LOG_ERROR("GetUnitByPID '%u' - failed: %s\n", pid, error.message);
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_READ(msg, "o", &unit_name);
    if (0 > rc)
        goto cleanup;

    if (unit_name != NULL)
        *ret_unit_name = strdup(unit_name);

    if (NULL != *ret_unit_name) {
        LOG_INFO("unit '%s' found for PID '%u'\n", *ret_unit_name, pid);
    } else {
        LOG_INFO("no unit found for PID '%u'\n", pid);
        rc = SDW_EINVAL;
    }

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);

    if (rc >= 0)
        return 0;

    return SDW_EINVAL;
}

static int sdwi_enable(const char *unit_name, bool runtime, bool force) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;
    int rc = 0;
    char *change[3] = { NULL, NULL, NULL };     // a(sss)
    int inst_info = 0;          // from sd_bus_message_read.3 -> "b" int * (NB not bool *)

    LOG_DEBUG("SDBUS_ENABLE_UNIT - '%s' '%s' '%s' '%s' '%s' %d %d\n",
              sdbus_service_contact, sdbus_object_path,
              sdbus_interface_mgr, "EnableUnitFiles", unit_name, runtime,
              force);

    rc = FN_SD_BUS_CALL_METHOD(bus, sdbus_service_contact, sdbus_object_path,
                               sdbus_interface_mgr, "EnableUnitFiles", &error,
                               &msg, "asbb", 1, unit_name, runtime, force);

    if (rc < 0) {
        LOG_ERROR("failed to issue method call: %s\n", error.message);
        goto cleanup;
    }

    /* Parse the response message */
    rc = FN_SD_BUS_MESSAGE_READ(msg, "b", &inst_info);

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_ENTER_CONTAINER(msg, 'a', "(sss)");

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_READ(msg, "(sss)", &change[0], &change[1],
                                &change[2]);

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_EXIT_CONTAINER(msg);

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    if (inst_info) {
        LOG_INFO("EnableUnitFiles %d '%s' '%s' '%s'\n", inst_info,
                 change[0] == NULL ? "NULL" : change[0],
                 change[1] == NULL ? "NULL" : change[1],
                 change[2] == NULL ? "NULL" : change[2]);
    } else {
        LOG_INFO("EnableUnitFiles 0, 'NULL' 'NULL' 'NULL'\n");
    }

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);
    if (rc >= 0)
        return 0;

    return SDW_EINVAL;
}

static int sdwi_disable(const char *unit_name, bool runtime) {
    int rc = 0;
    char *change[3] = { NULL, NULL, NULL };     // a(sss)

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;

    LOG_DEBUG("'%s' '%s' '%s' '%s' '%s' %d\n",
              sdbus_service_contact, sdbus_object_path,
              sdbus_interface_mgr, "DisableUnitFiles", unit_name, runtime);

    rc = FN_SD_BUS_CALL_METHOD(bus, sdbus_service_contact, sdbus_object_path,
                               sdbus_interface_mgr, "DisableUnitFiles", &error,
                               &msg, "asb", 1, unit_name, runtime);

    if (rc < 0) {
        LOG_ERROR("failed to issue method call: %s\n", error.message);
        goto cleanup;
    }

    /* Parse the response message */
    rc = FN_SD_BUS_MESSAGE_ENTER_CONTAINER(msg, 'a', "(sss)");

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_READ(msg, "(sss)", &change[0], &change[1],
                                &change[2]);

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_EXIT_CONTAINER(msg);

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    LOG_INFO("DisableUnitFiles '%s' '%s' '%s'\n",
             change[0] == NULL ? "NULL" : change[0],
             change[1] == NULL ? "NULL" : change[1],
             change[2] == NULL ? "NULL" : change[2]);

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);

    if (rc >= 0)
        return 0;

    return SDW_EINVAL;
}

static int sdwi_get_srv_property(const char *unit_name_encoded,
                                 const char *property,
                                 const char *response_format,
                                 response_t *response) {
    int rc = 0;
    char *path = NULL;

    rc = asprintf(&path, "%s/unit/%s", sdbus_object_path, unit_name_encoded);
    if (-1 == rc) {
        LOG_ERROR("asprintf() failed\n");
        goto cleanup;
    }

    rc = sdwi_get_property(path, sdbus_service_contact, sdbus_interface_srv,
                           property, response_format, response);

cleanup:
    if (NULL != path)
        free(path);

    return rc;
}

static int sdwi_get_unit_property(const char *unit_name_encoded,
                                  const char *service,
                                  const char *interface,
                                  const char *property,
                                  const char *response_format,
                                  response_t *response) {
    int rc = 0;
    char *path = NULL;

    rc = asprintf(&path, "%s/unit/%s", sdbus_object_path, unit_name_encoded);
    if (-1 == rc) {
        LOG_ERROR("intf() failed\n");
        goto cleanup;
    }

    rc = sdwi_get_property(path, service, interface, property, response_format, response);

cleanup:
    if (NULL != path)
        free(path);

    return rc;
}

static int sdwi_get_property(const char *path,
                             const char *service_contact,
                             const char *interface,
                             const char *property,
                             const char *response_format,
                             response_t *response) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;
    int rc = 0;

    memset(response, 0, sizeof(response_t));

    LOG_DEBUG("'%s' '%s' '%s' '%s'\n", service_contact, path, interface,
              property);

    rc = FN_SD_BUS_GET_PROPERTY(bus, service_contact, path,
                                interface, property, &error, &msg,
                                response_format);

    if (rc < 0) {
        LOG_ERROR("failed to issue method call: %s\n", error.message);
        goto cleanup;
    }

    /* Parse the response message */
    rc = FN_SD_BUS_MESSAGE_READ(msg, response_format, response);

    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    if (strcmp("s", response_format) == 0) {
        response->s = strdup(response->s);
        LOG_INFO("unit property %s: %s\n", property, response->s);
    } else if (strcmp("u", response_format) == 0) {
        LOG_INFO("unit property %s: %u\n", property, response->u);
    }

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);

    if (rc >= 0)
        return 0;

    return SDW_EINVAL;
}

static int sdwi_sdbus_cmd(const char *unit_name, char **response, sdbus_cmd_t cmd) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;
    char *s = NULL;
    int r = 0;
    const char *c, *cmd_str[] = {
        [SDBUS_START_UNIT] = "StartUnit",
        [SDBUS_RESTART_UNIT] = "RestartUnit",
        [SDBUS_STOP_UNIT] = "StopUnit"
    };

    c = cmd_str[cmd];

    LOG_DEBUG("'%s' '%s' '%s' '%s' '%s'\n",
              sdbus_service_contact, sdbus_object_path,
              sdbus_interface_mgr, c, unit_name);

    r = FN_SD_BUS_CALL_METHOD(bus, sdbus_service_contact, sdbus_object_path,
                              sdbus_interface_mgr, c, &error, &msg,
                              "ss", unit_name, "replace");
    if (r < 0) {
        LOG_ERROR("%s '%s' - failed: %s\n", c, unit_name, error.message);
        goto cleanup;
    }

    /* Parse the response message */
    r = FN_SD_BUS_MESSAGE_READ(msg, "o", &s);
    if (r < 0 || NULL == s) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-r));
        goto cleanup;
    }

    if (response != NULL)
        *response = strdup(s);

    LOG_INFO("%s: queued service job as %s.\n", c, s);

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);

    if (r >= 0)
        return 0;

    return SDW_EINVAL;
}

static int sdwi_encode(unit_t *unit) {
    char *buf = NULL;
    int l, rc;

    l = FN_SD_BUS_PATH_ENCODE(sdbus_prefix, unit->name, &buf);

    if (l < 0 || NULL == buf) {
        LOG_ERROR("failed to encode '%s'\n", unit->name);
        rc = SDW_EINVAL;
        goto cleanup;
    }

    if (strlen(buf) <= (strlen(sdbus_prefix) + 1)) {
        LOG_ERROR("invalid length of '%s'\n", buf);
        rc = SDW_EINVAL;
        goto cleanup;
    }
    // skip '<PREFIX>/'
    strncpy(unit->encoded, (buf + strlen(sdbus_prefix) + 1),
            sizeof(unit->encoded) - 1);

    LOG_DEBUG("encoded '%s' to '%s'\n", unit->name, unit->encoded);

    rc = 0;

cleanup:
    if (NULL != buf)
        free(buf);

    return rc;
}

static int sdwi_decode(unit_t *unit) {
    char *buf = NULL;
    char *path = NULL;
    int rc = asprintf(&path, "%s/%s", sdbus_prefix, unit->encoded);
    if (-1 == rc || NULL == path) {
        LOG_ERROR("failed to decode '%s'\n", unit->encoded);
        rc = SDW_EINVAL;
        goto cleanup;
    }

    rc = FN_SD_BUS_PATH_DECODE(path, sdbus_prefix, &buf);
    if (rc <= 0 || NULL == buf) {
        LOG_ERROR("failed to decode '%s'\n", unit->encoded);
        rc = SDW_EINVAL;
        goto cleanup;
    }

    if (strnlen(buf, sizeof(unit->name)) < sizeof(unit->name)) {
        strncpy(unit->name, buf, sizeof(unit->name) - 1);
        LOG_DEBUG("decoded '%s' to '%s'\n", unit->encoded, unit->name);
        rc = 0;
        goto cleanup;
    }

cleanup:
    if (NULL != path)
        free(path);
    if (NULL != buf)
        free(buf);

    return rc;
}

static int sdwi_notify(int flag, const char *msg) {
    int rc = FN_SD_NOTIFY(flag, msg);

    // If $NOTIFY_SOCKET was not set and hence no status message could be sent, 0 is returned.
    if (0 == rc) {
        LOG_ERROR("message could not be sent, NOTIFY_SOCKET not set\n");
        return SDW_ENOTIFYSOCK;
    }
    // On failure, these calls return a negative errno-style error code.
    if (0 > rc) {
        LOG_ERROR("message could not be sent %s", strerror(-rc));
        return SDW_EINVAL;
    }

    LOG_INFO("notify(%d, '%s'), (rc=%d)\n", flag, msg, rc);

    // If the status was sent, these functions return a positive value.
    // --> map rc to 0
    return 0;
}

static int sdwi_set_unit_name(unit_t *unit, const char *unit_name) {

    if (NULL == unit_name) {
        LOG_ERROR("invalid unit name 'NULL'\n");
        return SDW_EINVAL;
    }
    // just check the length of the unit name

    if (strnlen(unit_name, MAX_UNIT_NAME_LEN + 2) >= MAX_UNIT_NAME_LEN) {
        LOG_ERROR("invalid unit name '%s'\n", unit_name);
        return SDW_EINVAL;
    }

    strncpy(unit->name, unit_name, sizeof(unit->name) - 1);

    return 0;

}

static int sdwi_job_prepare(job_info_t *job) {
    int rc = 0;

    job->status = JOB_UNKNOWN;
    job->ts_end = time(NULL) + job->wait_sec;
    job->slot = NULL;
    job->path = NULL;
    job->result = NULL;

    rc = FN_SD_BUS_ADD_MATCH(bus, &job->slot, sdbus_match, sdwi_msg_handler,
                             (void *) job);

    if (rc < 0) {
        LOG_ERROR("sd_bus_add_match(,,%s,,) failed, (rc=%d,%s)\n", sdbus_match,
                  rc, strerror(-rc));
        return SDW_EINVAL;
    }

    LOG_INFO("sd_bus_add_match(,,%s,,)  (rc=%d)\n", sdbus_match, rc);

    return 0;
}

static int sdwi_job_wait(job_info_t *job) {
    int rc = 0;

    LOG_INFO("waiting %us for job %s to finish\n", job->wait_sec, job->path);

    while (JOB_UNKNOWN == job->status) {
        time_t ts_now = time(NULL);
        uint64_t wait_usec;

        if (job->ts_end > ts_now) {
            // usec waittime for sd_bus_wait
            wait_usec = (uint64_t) 1000 *1000 * (job->ts_end - ts_now);
        } else {
            // expired without finished job
            LOG_INFO("wait time %u expired for job %s\n", job->wait_sec,
                     job->path);
            return SDW_ETIMEOUT;
        }

        // wait for I/O on sdbus
        rc = FN_SD_BUS_WAIT(bus, wait_usec);
        if (rc < 0) {
            LOG_ERROR("sd_bus_wait failed %s\n", strerror(rc));
            return SDW_EINVAL;
        }
        // call the sdbus lib for handover to the callback
        rc = FN_SD_BUS_PROCESS(bus, NULL);
        if (rc < 0) {
            LOG_ERROR("sd_bus_process failed %s\n", strerror(rc));
            return SDW_EINVAL;
        }
    }

    if (JOB_DONE == job->status)
        return 0;

    return SDW_EINVAL;
}

static void sdwi_job_remove(job_info_t *job) {
    if (NULL != job->path)
        free(job->path);
    if (NULL != job->result)
        free(job->result);

    if (NULL != job->slot)
        FN_SD_BUS_SLOT_UNREF(job->slot);
}

static int sdwi_msg_handler(sd_bus_message *msg,
                                void *userdata, sd_bus_error *error) {
    const char *path, *unit, *result;
    uint32_t id;
    int rc;
    job_info_t *job = (job_info_t *) userdata;

    if (NULL == job) {
        LOG_ERROR("invalid job (NULL)\n");
        return 0;
    }

    if (NULL == job->path) {
        LOG_ERROR("invalid job->path (NULL)\n");
        return 0;
    }

    rc = FN_SD_BUS_MESSAGE_READ(msg, "uoss", &id, &path, &unit, &result);
    if (rc < 0) {
        LOG_ERROR("%s\n", error->message);
        return 0;
    }

    LOG_INFO("id: %u, path: '%s', unit: '%s', result: '%s'\n", id, path, unit,
             result);

    // the job.path must be checked because we don't want to process signals from other jobs

    if (strcmp(job->path, path) != 0) {
        LOG_INFO("'%s' ignore signal for %u result: '%s'\n", job->path, id,
                 result);
        return 0;
    }

    job->result = strdup(result);

    if (strcmp(result, "done") == 0) {
        LOG_INFO("job '%s' finished\n", path);
        job->status = JOB_DONE;
    } else {
        // map possible errors to JOB_FAILED
        // for further details see bus_wait_for_jobs and check_wait_response
        if (
            // TODO, check correctnes of #if_0
#if 0
               // known 'failed' results
               (strcmp(result, "canceled") == 0) ||
               (strcmp(result, "timeout") == 0) ||
               (strcmp(result, "dependency") == 0) ||
               (strcmp(result, "invalid") == 0) ||
               (strcmp(result, "assert") == 0) ||
               (strcmp(result, "unsupported") == 0) ||
               (strcmp(result, "collected") == 0) ||
               (strcmp(result, "once") == 0) ||
#endif
               // cover any 'failed' results
               // 'done' was already checked
               strcmp(result, "skipped") != 0) {

            LOG_ERROR("job '%s' canceled with '%s'\n", path, result);
            job->status = JOB_FAILED;
        }
    }

    return 0;
}

static int sdwi_get_unitfilestate(const char *unit_name, char **ret_state) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;
    char *response = NULL;
    int rc = 0;
    const char *cmd = "GetUnitFileState";

    LOG_INFO("'%s' '%s' '%s' '%s' '%s'\n",
             sdbus_service_contact, sdbus_object_path,
             sdbus_interface_mgr, cmd, unit_name);

    rc = FN_SD_BUS_CALL_METHOD(bus, sdbus_service_contact, sdbus_object_path,
                               sdbus_interface_mgr, cmd, &error, &msg,
                               "s", unit_name);
    if (rc < 0) {
        LOG_ERROR("%s '%s' - failed: %s\n", cmd, unit_name, error.message);
        goto cleanup;
    }

    rc = FN_SD_BUS_MESSAGE_READ(msg, "s", &response);
    if (rc < 0) {
        LOG_ERROR("failed to parse response message: %s\n", strerror(-rc));
        goto cleanup;
    }

    LOG_INFO("unit file state: %s.\n", response);

    if (0 <= rc) {
        if (ret_state != NULL) {
            if (strlen(response) <= MAX_RESPONSE_LEN) {
                *ret_state = strdup(response);
            }
        }

        if (strcmp(response, "enabled") == 0)
            rc = SDW_UNIT_FILE_STAT_ENABLED;

        else if (strcmp(response, "disabled") == 0)
            rc = SDW_UNIT_FILE_STAT_DISABLED;
    }

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);

    if (rc >= 0)
        return rc;

    return SDW_EINVAL;
}

static int sdwi_get_activestate(const char *unit_name_encoded, char **ret_state) {
    response_t response;
    int rc;

    response.s = NULL;

    rc = sdwi_get_unit_property(unit_name_encoded,
                                sdbus_service_contact, sdbus_interface_unit,
                                "ActiveState", "s", &response);

    if (0 == rc) {
        if (NULL != ret_state &&
            NULL != response.s && strlen(response.s) <= MAX_RESPONSE_LEN) {

            *ret_state = strdup(response.s);
        }

        rc = SDW_UNIT_ACTIVE_STAT_UNKNOWN;

        if (NULL != response.s) {

            if (strcmp("activating", response.s) == 0)
                rc = SDW_UNIT_ACTIVE_STAT_ACTIVATING;

            if (strcmp("active", response.s) == 0)
                rc = SDW_UNIT_ACTIVE_STAT_ACTIVE;

            else if (strcmp("reloading", response.s) == 0)
                rc = SDW_UNIT_ACTIVE_STAT_RELOADING;

            else if (strcmp("deactivating", response.s) == 0)
                rc = SDW_UNIT_ACTIVE_STAT_DEACTIVATING;

            else if (strcmp("inactive", response.s) == 0)
                rc = SDW_UNIT_ACTIVE_STAT_INACTIVE;

            else if (strcmp("failed", response.s) == 0)
                rc = SDW_UNIT_ACTIVE_STAT_FAILED;
        }
    }

    if (NULL != response.s)
        free((void *) response.s);

    return rc;
}

// dead/start/running/stop-sigterm
static int sdwi_get_substate(const char *unit_name_encoded, char **ret_state) {
    response_t response;
    int rc;

    response.s = NULL;

    rc = sdwi_get_unit_property(unit_name_encoded, sdbus_service_contact,
                                sdbus_interface_unit, "SubState", "s", &response);
    if (0 == rc) {
        if (NULL != ret_state &&
            NULL != response.s && strlen(response.s) <= MAX_RESPONSE_LEN) {
            *ret_state = strdup(response.s);
        }

        rc = SDW_UNIT_SUB_STAT_UNKNOWN;

        if (NULL != response.s) {

            if (strcmp("start", response.s) == 0)
                rc = SDW_UNIT_SUB_STAT_START;

            else if (strcmp("running", response.s) == 0)
                rc = SDW_UNIT_SUB_STAT_RUNNING;

            else if (strcmp("stop-sigterm", response.s) == 0)
                rc = SDW_UNIT_SUB_STAT_STOP_SIGTERM;

            else if (strcmp("dead", response.s) == 0)
                rc = SDW_UNIT_SUB_STAT_DEAD;
        }
    }

    if (NULL != response.s)
        free(response.s);

    return rc;
}

/* external functions */

int sdw_start(const char *unit_name, unsigned wait_sec) {
    int rc;
    job_info_t job;
    char *response = NULL;

    job.cmd = SDBUS_START_UNIT;
    job.wait_sec = wait_sec;

    // async call
    if (0 == wait_sec)
        return sdwi_sdbus_cmd(unit_name, NULL, job.cmd);

    // sync call:
    // - register for message on sdbus
    // - start the unit
    // - wait for final job status
    rc = sdwi_job_prepare(&job);
    if (rc != 0)
        goto cleanup;

    rc = sdwi_sdbus_cmd(unit_name, &response, job.cmd);
    job.path = response;

    if (rc != 0)
        goto cleanup;

    LOG_INFO("waiting %us for job %s to finish\n", wait_sec, job.path);

    rc = sdwi_job_wait(&job);

cleanup:
    sdwi_job_remove(&job);

    return rc;
}

int sdw_restart(const char *unit_name, unsigned wait_sec) {
    int rc;
    job_info_t job;
    char *response = NULL;

    job.cmd = SDBUS_RESTART_UNIT;
    job.wait_sec = wait_sec;

    // async call
    if (0 == wait_sec)
        return sdwi_sdbus_cmd(unit_name, NULL, job.cmd);

    // sync call:
    // - register for message on sdbus
    // - restart the unit
    // - wait for final job status
    rc = sdwi_job_prepare(&job);
    if (rc != 0)
        goto cleanup;

    rc = sdwi_sdbus_cmd(unit_name, &response, job.cmd);
    job.path = response;

    if (rc != 0)
        goto cleanup;

    LOG_INFO("waiting %us for job %s to finish\n", wait_sec, job.path);

    rc = sdwi_job_wait(&job);

cleanup:
    sdwi_job_remove(&job);

    return rc;
}

int sdw_stop(const char *unit_name, unsigned wait_sec) {
    int rc;
    job_info_t job;
    char *response = NULL;

    job.cmd = SDBUS_STOP_UNIT;
    job.wait_sec = wait_sec;

    // async call
    if (0 == wait_sec)
        return sdwi_sdbus_cmd(unit_name, NULL, job.cmd);

    // sync call:
    // - register for message on sdbus
    // - stop the unit
    // - wait for final job status
    rc = sdwi_job_prepare(&job);
    if (rc != 0)
        goto cleanup;

    rc = sdwi_sdbus_cmd(unit_name, &response, job.cmd);
    job.path = response;

    if (rc != 0)
        goto cleanup;

    LOG_INFO("waiting %us for job %s to finish\n", wait_sec, job.path);

    rc = sdwi_job_wait(&job);

cleanup:
    sdwi_job_remove(&job);

    return rc;
}

int sdw_get_version(char **ret_version) {
    response_t response;
    int rc;

    response.s = NULL;

    if (NULL == ret_version)
        return SDW_EINVAL;

    rc = sdwi_get_property(sdbus_object_path, sdbus_service_contact,
                           sdbus_interface_mgr, "Version", "s", &response);

    if (0 == rc &&
        NULL != response.s &&
        strnlen(response.s, MAX_RESPONSE_LEN) < (MAX_RESPONSE_LEN - 1)) {
        *ret_version = strdup(response.s);
    }

    if (NULL != response.s)
        free(response.s);

    return rc;
}

int sdw_get_unitfilestate(const char *unit_name, char **ret_state) {
    return sdwi_get_unitfilestate(unit_name, ret_state);
}

int sdw_check_pid(const char *unit_name, unsigned pid) {
    unit_t unit;
    char *response = NULL;
    int rc;

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    rc = sdwi_encode(&unit);
    if (rc != 0)
        return rc;

    if (0 == pid)
        pid = (unsigned) getpid();

    rc = sdwi_get_unit_by_pid(pid, &response);
    if (0 != rc || NULL == response) {
        rc = SDW_EINVAL;
        goto cleanup;
    }

    if (NULL != strstr(response, unit.encoded)) {
        LOG_INFO("unit '%s' found for PID '%u'\n", unit_name, pid);
    } else {
        LOG_INFO("no unit found for PID '%u'\n", pid);
        rc = SDW_EINVAL;
    }

cleanup:

    if (NULL != response)
        free(response);

    return rc;
}

int sdw_check_controlpid(const char *unit_name, unsigned pid) {
    unit_t unit;
    unsigned ctrl_pid = ~0U;
    response_t response;
    int rc;

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    rc = sdwi_encode(&unit);
    if (rc != 0)
        return rc;

    response.u = 0;

    rc = sdwi_get_unit_property(unit.encoded, sdbus_service_contact,
                                sdbus_interface_srv, "ControlPID",
                                "u", &response);
    if (rc != 0)
        return rc;

    ctrl_pid = response.u;

    if (ctrl_pid != pid) {
        LOG_INFO("ControlPID %u != PID %u\n", ctrl_pid, pid);
        return SDW_EINVAL;
    }

    LOG_INFO("ControlPID %u == PID %u\n", ctrl_pid, pid);
    return 0;
}

int sdw_get_mainpid(const char *unit_name, unsigned *pid) {
    unit_t unit;
    response_t response;
    int rc;

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    rc = sdwi_encode(&unit);
    if (rc != 0)
        return rc;

    response.u = 0;

    if (NULL == pid)
        return SDW_EINVAL;

    rc = sdwi_get_srv_property(unit.encoded, "MainPID", "u", &response);
    *pid = response.u;

    return rc;
}

int sdw_get_controlpid(const char *unit_name, unsigned *pid) {
    response_t response;
    int rc;

    response.u = 0;

    if (NULL == pid)
        return SDW_EINVAL;

    rc = sdwi_get_unit_property(unit_name, sdbus_service_contact,
                                sdbus_interface_srv, "ControlPID",
                                "u", &response);
    *pid = response.u;

    return rc;
}

int sdw_notify_ready(void) {

    return sdwi_notify(0, "READY=1");
}

int sdw_notify_stopping(void) {

    return sdwi_notify(0, "STOPPING=1");
}

int sdw_notify_mainpid(unsigned pid) {
    char *msg = NULL;
    int rc;

    rc = asprintf(&msg, "MAINPID=%u", pid);
    if (-1 == rc || NULL == msg)
        return SDW_EINVAL;

    rc = sdwi_notify(0, msg);

    free(msg);
    return rc;
}

const char *sdw_get_error_message(void) {
    return last_error_msg;
}

// call initialization without trace
// in non systemd setups we want to suppress errors/warnings
int sdw_is_supported(void) {
    if (LOADED == lib_stat)
        return 0;

    if (INVALID_VERSION == lib_stat)
        return SDW_EVERSION;

    return SDW_EINIT;
}

int sdw_encode(const char *unit_name, char **ret_encoded) {
    unit_t unit;
    int rc;

    if (NULL == ret_encoded)
        return SDW_EINVAL;

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    rc = sdwi_encode(&unit);
    if (rc == 0)
        *ret_encoded = strdup(unit.encoded);

    return rc;
}

int sdw_decode(const char *unit_name, char **ret_decoded) {

    unit_t unit;
    int rc;

    if (NULL == unit_name || NULL == ret_decoded)
        return SDW_EINVAL;

    if (strlen(unit_name) >= MAX_UNIT_NAME_LEN) {
        LOG_ERROR("invalid unit name '%s'\n", unit_name);
        return SDW_EINVAL;
    }

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    strncpy(unit.encoded, unit.name, sizeof(unit.name));

    rc = sdwi_decode(&unit);
    if (0 == rc)
        *ret_decoded = strndup(unit.name, sizeof(unit.name));

    return rc;
}

int sdw_get_unit_by_pid(unsigned pid, char **unit_name) {
    char *response = NULL;
    int rc;

    rc = sdwi_get_unit_by_pid(pid, &response);
    if (0 == rc) {
        LOG_INFO("unit '%s' found for PID '%u'\n", response, pid);
        if (strlen(response) <= MAX_RESPONSE_LEN) {
            *unit_name = strdup(response);
        }
    }

    if (NULL != response)
        free(response);

    return rc;
}

int sdw_get_activestate(const char *unit_name, char **ret_state) {
    unit_t unit;
    int rc;

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    rc = sdwi_encode(&unit);
    if (rc != 0)
        return rc;

    return sdwi_get_activestate(unit.encoded, ret_state);
}

int sdw_get_substate(const char *unit_name, char **ret_state) {
    unit_t unit;
    int rc;

    rc = sdwi_set_unit_name(&unit, unit_name);
    if (rc != 0)
        return rc;

    rc = sdwi_encode(&unit);
    if (rc != 0)
        return rc;

    return sdwi_get_substate(unit.encoded, ret_state);
}

int sdw_enable(const char *unit_name) {
    int rc;
    rc = sdwi_enable(unit_name, false, true);

    if (rc != 0)
        return rc;

    return sdw_reload();
}

int sdw_disable(const char *unit_name) {
    int rc;
    rc = sdwi_disable(unit_name, false);

    if (rc != 0)
        return rc;

    return sdw_reload();
}

int sdw_reload(void) {
    int rc = 0;

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *msg = NULL;

    LOG_DEBUG("'%s' '%s' '%s' '%s'\n",
              sdbus_service_contact, sdbus_object_path,
              sdbus_interface_mgr, "Reload");

    rc = FN_SD_BUS_CALL_METHOD(bus, sdbus_service_contact, sdbus_object_path,
                               sdbus_interface_mgr, "Reload", &error, &msg, "");

    if (rc < 0) {
        LOG_ERROR("failed to issue method call: %s\n", error.message);
        goto cleanup;
    }

cleanup:
    FN_SD_BUS_ERROR_FREE(&error);
    FN_SD_BUS_MESSAGE_UNREF(msg);

    if (rc >= 0)
        return 0;

    return SDW_EINVAL;
}

void sdw_set_tracelevel(int trace_level) {
    if (trace_level >= 0 && trace_level <= 2)
        trc_level = trace_level;
}
