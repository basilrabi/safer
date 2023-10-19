/**
 * This contains functions which are constantly running throughout the life
 * of the main process. All of these functions will terminate when the shutdown
 * status is fetched from redis.
 */

#ifndef WORKER_H
#define WORKER_H

#include <gtk/gtk.h>

/**
 * shutdown_trigger:
 *
 * Triggers the actual system shutdown after all equipment status are sent. The
 * following redis keys are triggered in order:
 * pre_shutdown: triggered by ignition off
 * shutdown: triggered if ignition is still off within a certain duration
 * proceed_shutdown: triggered when all messages are sent
 */
void shutdown_trigger();

/**
 * shutdown_watcher:
 *
 * A separate thread for watching the pre_shutdown key which will be triggered
 * when the ignition is off. Should the pre_shutdown key remains on for the set
 * period of time, the shutdown key will be set to TRUE. The pre_shutdown key is
 * needed to account for temporary disconnection as experienced in the devices
 * from Manila GPS.
 *
 * TODO: When shutting down, the time should be recorded so when booting up
 * again, the time difference will be analyzed to check if the real-time clock
 * is still functioning properly.
 */
void shutdown_watcher();

/**
 * status_sender:
 *
 * A background worker which will send the cached equipment status in redis to
 * the server. This will also queues equipment statuses to redis at the defined
 * time interval in the function.
 */
void status_sender();

/* sms_receiver:
 *
 * Required use cases:
 * 1. updating operators
 * 2. updating supervisors
 * 3. pms notification
 *
 * Nice to have use cases:
 * 1. habitual over speeding notification
 * 2. deployment/reassignment notification
 */
void sms_receiver();

#endif // WORKER_H
