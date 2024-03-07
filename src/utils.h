#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>
#include <hiredis/hiredis.h>

typedef struct power_stat {
  GtkWidget *battery;
  GtkWidget *voltage;
} power_stat;

typedef struct pset {
  redisContext *context;
  char *css;
} pset;

/**
 * get_char_key:
 * @key: name of the key in redis where the value will be taken
 * @value: string where the value will be stored
 *
 * Returns: 0 if fail, 1 if success
 */
int get_char_key(const char  *key,
                 char       **value);

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
 * redis_cmd:
 * @cmd: a redis command
 * @key: redis key to modify
 * @x: a value
 *
 * Runs a redis command with 1 optional value.
 *
 * Returns: 0 if fail, 1 if success.
 */
int redis_cmd(const char *cmd,
              const char *key,
              const char *x);

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
 * set_hat_time:
 *
 * This will synchronize the time on the GNSS HAT with the system time.
 *
 * Returns: 1 if success
 *          0 if fail
 */
int set_hat_time(void);

/**
 * at_cmd:
 * @cmd: AT command to run
 * @reponse: the returned response of the AT command
 * @timeout: number of seconds to wait for before reading the response.
 *
 * Runs the AT command @cmd and saves reply to @response.
 */
void at_cmd(const char    *cmd,
            char         **response,
            unsigned int   timeout);

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

/*
 * initialize_serial_connection:
 * @serial_file_descriptor: file descriptor of the opened serial port
 *
 * Initializes and opens a serial port connection. The file descriptor of the
 * opened connection is saved.
 */
void initialize_serial_connection(int *serial_file_descriptor);

/**
 * send_sms:
 * @format: a string format that will be sent to the server
 *
 * Sends an SMS to the server with the contents of @text.
 */
void send_sms(const char *format,
              ...);

/**
 * str_copy:
 * @destination: string
 * @source: string
 *
 * Copies @source to @destination dynamically.
 */
void str_copy(char       **destination,
              const char  *source);

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

/**
 * str_sub:
 * @destination: the substring to be saved
 * @source: a string
 * @start: start index of the substring in @source
 * @end: end index of the substring in @source
 *
 * Gets a substring of @source and save it to @destination.
 */
void str_sub(char       *destination,
             const char *source,
             const int   start,
             const int   end);

#endif // UTILS_H

