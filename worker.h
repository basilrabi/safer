/**
 * This contains functions which are constantly running throughout the life
 * of the main process. All of these functions will terminate when the shutdown
 * status is fetched from redis.
 */

#ifndef WORKER_H
#define WORKER_H

#include <gtk/gtk.h>

/**
 * sms_receiver:
 * @data: redisContext to be used for connection
 *
 * A background worker which will send the cached equipment status in redis to
 * the server. This will also queues equipment statuses to redis at the defined
 * time interval in the function.
 */
void status_sender(gpointer data);

/* sms_receiver:
 * @data: redisContext to be used for connection
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
void sms_receiver(gpointer data);

#endif // WORKER_H
