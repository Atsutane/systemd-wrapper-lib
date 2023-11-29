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

#ifndef _LIBSDW_H_
#define _LIBSDW_H_

/*--------------------------------------------------------------------*/
/** @file    sdw.h
 *
 *  @brief   wrapper for systemd communication
 *
 *                                                                    */
/*--------------------------------------------------------------------*/


enum {
    SDW_EINIT       = -1,                           /**< systemdlib initialization failed   */
    SDW_EVERSION    = -2,                           /**< invalid systemd version            */
    SDW_EINVAL      = -3,                           /**< invalid value                      */
    SDW_ENOTIFYSOCK = -4,                           /**< sd_notify socket not available     */
    SDW_ETIMEOUT    = -5,                           /**< timeout of synchronous call        */

    SDW_UNIT_FILE_STAT_ENABLED              = 11,   /**< Unit FileState is enabled          */
    SDW_UNIT_FILE_STAT_DISABLED             = 12,   /**< Unit FileState is disabled         */

    SDW_UNIT_ACTIVE_STAT_UNKNOWN            = 20,   /**< Unit ActiveState is unknown        */
    SDW_UNIT_ACTIVE_STAT_ACTIVATING         = 21,   /**< Unit ActiveState is activating     */
    SDW_UNIT_ACTIVE_STAT_ACTIVE             = 22,   /**< Unit ActiveState is active         */
    SDW_UNIT_ACTIVE_STAT_RELOADING          = 23,   /**< Unit ActiveState is reloading      */
    SDW_UNIT_ACTIVE_STAT_DEACTIVATING       = 24,   /**< Unit ActiveState is deactivating   */
    SDW_UNIT_ACTIVE_STAT_INACTIVE           = 25,   /**< Unit ActiveState is inactive       */
    SDW_UNIT_ACTIVE_STAT_FAILED             = 26,   /**< Unit ActiveState is failed         */

    SDW_UNIT_SUB_STAT_UNKNOWN               = 30,   /**< Unit SubState is unknown           */
    SDW_UNIT_SUB_STAT_START                 = 31,   /**< Unit SubState is start             */
    SDW_UNIT_SUB_STAT_RUNNING               = 32,   /**< Unit SubState is running           */
    SDW_UNIT_SUB_STAT_STOP_SIGTERM          = 33,   /**< Unit SubState is stop-sigterm      */
    SDW_UNIT_SUB_STAT_DEAD                  = 34,   /**< Unit SubState is dead              */
    SDW_UNIT_SUB_STAT_FAILED                = 35    /**< Unit SubState is failed            */
};


/*--------------------------------------------------------------------*/
/* sdw_check_pid ()                                                   */
/*                                                                    */
/** Check if the PID of the calling process is started from systemd
 *  and it's own unit name matches unit_name
 *
 * @param  unit_name       unit_name of service
 * @param  pid             optional PID,
 *                         the default value '0' will be replaced by
 *                         getpid()
 *
 * @return
 *     - #0             successful, unit of pid found and matched
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *     - #SDW_EINVAL    not started from systemd or
 *                          invalid unit name
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_check_pid(const char *unit_name,
                  unsigned pid = 0);


/*--------------------------------------------------------------------*/
/* sdw_check_controlpid ()                                            */
/*                                                                    */
/** Compare the ControlPID of the unit with the value of pid
 *
 * @param  unit_name       unit_name of service
 * @param  pid             expected control PID
 *
 * @return
 *     - #0             successful, (pid == ControlPID)
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *     - #SDW_EINVAL    failed, pid != ControlPID
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_check_controlpid(const char *unit_name,
                         unsigned pid);


/*--------------------------------------------------------------------*/
/* sdw_get_mainpid ()                                                 */
/*                                                                    */
/** Get the MainPID for the service 'unit_name'
 *
 * @param  unit_name       unit_name of service
 * @param  pid             pid as out parameter
 *
 * @retval pid             pid of systemd service unit_name
 *
 * @return
 *     - #0             successful, pid of active instance found
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_get_mainpid(const char *unit_name,
                    unsigned *pid);


/*--------------------------------------------------------------------*/
/* sdw_start ()                                                       */
/*                                                                    */
/** Start the service 'unit_name'
 *
 * @param  unit_name       unit name of service
 * @param  wait_sec        wait_sec = 0 runs an async unit start
 *                         and does not return the final return code of
 *                         the startup process.
 *                         wait_sec > 0 runs a sync unit start
 *                         and waits 'wait_sec' for the return code of
 *                         the startup process.
 *
 * @return
 *     - #0             successful, instance started
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_start(const char *unit_name,
              unsigned wait_sec = 0);


/*--------------------------------------------------------------------*/
/* sdw_stop ()                                                        */
/*                                                                    */
/** Stop the service 'unit_name'
 *
 * @param  unit_name       unit name of service
 * @param  wait_sec        poll wait_sec for unit SubState 'dead'
 * @param  wait_sec        wait_sec = 0 runs an async unit stop
 *                         and does not return the final return code of
 *                         the shutdown process.
 *                         wait_sec > 0 runs a sync unit stop
 *                         and waits 'wait_sec' for the return code of
 *                         the shutdown process.
 *
 * @return
 *     - #0             successful, instance stopped
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_stop(const char *unit_name,
             unsigned wait_sec = 0);


/*--------------------------------------------------------------------*/
/* sdw_restart ()                                                     */
/*                                                                    */
/** Restart the service 'unit_name'
 *
 * @param  unit_name       unit name of service
 * @param  wait_sec        wait_sec = 0 runs an async unit restart
 *                         and does not return the final return code of
 *                         the restart process.
 *                         wait_sec > 0 runs a sync unit restart
 *                         and waits 'wait_sec' for the return code of
 *                         the restart process.
 *
 * @return
 *     - #0             successful, instance restarted
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_restart(const char *unit_name,
                unsigned wait_sec = 0);


/*--------------------------------------------------------------------*/
/* sdw_enable ()                                                      */
/*                                                                    */
/** Enable the service 'unit_name'
 *
 * @param  unit_name        unit name of service
 *
 * @return
 *     - #0             successful, service enabled
 *     - #SDW_EINVAL    failed, service not enabled
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_enable(const char *unit_name);


/*--------------------------------------------------------------------*/
/* sdw_disable ()                                                     */
/*                                                                    */
/** Disable the service 'unit_name'
 *
 * @param  unit_name        unit name of service
 *
 * @return
 *     - #0             successful, service disabled
 *     - #SDW_EINVAL    failed, service not disabled
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_disable(const char *unit_name);


/*--------------------------------------------------------------------*/
/* sdw_reload ()                                                      */
/*                                                                    */
/** Reload all units
 *
 * @param
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINVAL    reload failed
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_reload(void);


/*--------------------------------------------------------------------*/
/* sdw_get_version ()                                                 */
/*                                                                    */
/** Read the systemd version
 *
 * @param  ret_version      systemd version as out parameter
 *
 * @retval ret_version      caller must release the memory with free()
 *
 * @return
 *     - #0             successful, systemd library initialized,
 *                      version info is available
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_get_version(char **ret_version);


/*--------------------------------------------------------------------*/
/* sdw_notify_ready ()                                                */
/*                                                                    */
/** Notify service manager that service startup is finished
 *  For details see man sd_notify, READY=1
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_notify_ready(void);


/*--------------------------------------------------------------------*/
/* sdw_notify_stopping ()                                             */
/*                                                                    */
/** Notify service manager that service is stopping
 *  For details see man sd_notify, STOPPING=1
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_notify_stopping(void);


/*--------------------------------------------------------------------*/
/* sdw_notify_mainpid ()                                              */
/*                                                                    */
/** Update the mainpid of the service.
 *  For details see man sd_notify, MAINPID=%u
 *
 * @param  pid              new mainpid of service
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_notify_mainpid(unsigned pid);


/*--------------------------------------------------------------------*/
/* sdw_get_error_message ()                                           */
/*                                                                    */
/** Get last error message
 *
 * @return      pointer to last error message
 *              memory must not be released from the caller
 *                                                                    */
/*--------------------------------------------------------------------*/
const char *sdw_get_error_message(void);


/*--------------------------------------------------------------------*/
/* sdw_is_supported ()                                                */
/*                                                                    */
/** check systemd version for defined minimum version number
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_is_supported(void);


/*--------------------------------------------------------------------*/
/* sdw_encode ()                                                      */
/*                                                                    */
/** Encode a unit name, for details see 'man sd_bus_path_encode'
 *
 * @param  unit_name        unit name
 * @param  ret_encoded      pointer to the encoded unit name
 *
 * @retval ret_encoded      caller must release the memory with free()
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINVAL    failed
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_encode(const char *unit_name,
               char **ret_encoded);


/*--------------------------------------------------------------------*/
/* sdw_decode ()                                                      */
/*                                                                    */
/** Decode a unit name, for details see 'man sd_bus_path_decode'
 *
 * @param  unit_name        encoded unit name
 * @param  ret_decoded      pointer to the decoded unit name
 *
 * @retval ret_decoded      caller must release the memory with free()
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINVAL    failed
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_decode(const char *unit_name,
               char **ret_decoded);


/*--------------------------------------------------------------------*/
/* sdw_get_unit_by_pid ()                                             */
/*                                                                    */
/** Lookup the unit name for a running process
 *
 * @param  pid              pid of the process
 * @param  ret_unit_name    pointer to the unit name of the process
 *
 * @retval ret_unit_name    caller must release the memory with free()
 *
 * @return
 *     - #0             successful, unit found
 *     - #SDW_EINVAL    failed, no unit found
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_get_unit_by_pid(unsigned pid,
                        char **ret_unit_name);


/*--------------------------------------------------------------------*/
/* sdw_get_activestate ()                                             */
/*                                                                    */
/** Read the property 'ActiveState' of a unit
 *
 * @param  unit_name        unit name
 * @param  ret_state        pointer to the unit active state
 *
 * @retval ret_state        caller must release the memory with free()
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINVAL    failed
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_get_activestate(const char *unit_name,
                        char **ret_state);


/*--------------------------------------------------------------------*/
/* sdw_get_substate ()                                                */
/*                                                                    */
/** Read the property 'SubState' of a unit
 *
 * @param  unit_name        unit name
 * @param  ret_state        pointer to the unit sub state
 *
 * @retval ret_state        caller must release the memory with free()
 *
 * @return
 *     - #0             successful
 *     - #SDW_EINVAL    failed
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_get_substate(const char *unit_name,
                     char **ret_state);


/*--------------------------------------------------------------------*/
/* sdw_get_unitfilestate ()                                           */
/*                                                                    */
/** Read the unit file state with 'GetUnitFileState'
 *
 * @param  unit_name        unit name of service
 * @param  ret_state        pointer to the unit file state
 *
 * @retval ret_state        caller must release the memory with free()
 *
 * @return
 *     - #0           'GetUnitFileState' successful but unknown state
 *     - #SDW_UNIT_FILE_STAT_ENABLED   unit is enabled
 *     - #SDW_UNIT_FILE_STAT_DISABLED  unit is disabled
 *     - #SDW_EINVAL    failed
 *     - #SDW_EINIT     sdbus library initialization failed
 *     - #SDW_EVERSION  invalid systemd version detected
 *                                                                    */
/*--------------------------------------------------------------------*/
int sdw_get_unitfilestate(const char *unit_name,
                          char **ret_state);

/*--------------------------------------------------------------------*/
/* sdw_set_tracelevel ()                                              */
/*                                                                    */
/** Set internal tracelevel
 *
 * @param  level            traclevel (0-4)
 *                                                                    */
/*--------------------------------------------------------------------*/
void sdw_set_tracelevel(int level);

#endif
