#ifndef UTILS_H
#define UTILS_H

#include <hiredis/hiredis.h>

typedef struct pset {
  redisContext *context;
  char *css;
} pset;

/**
 * get_int_key:
 * @key: name of the key in redis where the value will be taken
 * @value: integer where the value will be stored
 *
 * Returns: 0 if fail, 1 if success
 */
int get_int_key(const char *key,
                int        *value);

/**
 * push_message:
 * @context: redisContext to be used
 * @message: the message to be pushed
 *
 * Pushes @message to the messages list in redis.
 *
 * Returns: 0 if fail, 1 if success.
 */
int push_message(redisContext *context,
                 const char   *message);

/**
 * redis_cmd:
 * @format: a string format which will be used as redis command
 *
 * Runs a redis command.
 *
 * Returns: 0 if fail, 1 if success.
 */
int redis_cmd(const char *format,
              ...);

/**
 * send_equipment_status:
 * @context: redisContext to be used for connection
 *
 * Using the cached list of equipment status in redis, send it to the server.
 *
 * Returns: 0 if no issues
 *          1 if failed to get status queue
 *          2 if querying the status queue does not return an array
 *          3 if deleting the sent equipment status fails
 */
int send_equipment_status(redisContext *context);

/**
 * capture_pattern:
 * @source: a string that contains the full text to be analyzed
 * @datetime: a string where the datetime substring will be stored
 * @status: a string where the equipment status substring will be stored
 * @location: a string where the equipment location will be stored
 *
 * Extracts the datetime, equipment status, and equipment location regex
 * patterns.
 */
void capture_pattern(const char *source,
                     char       *datetime,
                     char       *status,
                     char       *location);

/**
 * send_sms:
 * @text: a string that will be sent to the server
 *
 * Sends an SMS to the server with the contents of @text.
 */
void send_sms(const char *text);

/**
 * set_system_time:
 *
 * This will synchronize the system using the real-time clock.
 */
void set_system_time(void);

/**
 * str_difference:
 * @old_string: a string
 * @new_string: a string where the first few characters may be the same with
 *   @old_string
 * @holder: a string where the difference will be saved
 *
 * Compares @old and @new. Starting at the array index where the character  in
 * @new is different from @old up to the last index of @new, that substring will
 * be stored to @holder.
 */
void str_difference(const char *old_string,
                    const char *new_string,
                    char       *holder);

#endif // UTILS_H

