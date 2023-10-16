#include <hiredis/hiredis.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include "worker.h"
#include "utils.h"

void status_sender(gpointer data)
{
  redisContext *context = (redisContext *) data;

  // Number of messages to be queued before sending
  const int message_limit = 10;
  // Number of seconds before the new equipment status is recorded to allow making mistakes
  const int seconds_refresh_cutoff = 5;
  // Number of seconds between location queries
  const int seconds_location_period = 30;

  char time_buffer[20];
  char *location = "LOCATION"; // Temporary placeholder for GNSS query
  char *message;
  char *last_status = "off";
  int buffer_size;
  int push_message_status = 0;
  int refresh_status = 0;
  int shutdown = 0;
  redisReply *equipment_status = NULL;
  redisReply *previous_equipment_status = NULL;
  redisReply *status_queue = NULL;
  struct tm *time_print;
  time_t current_time;
  time_t refresh_time;
  time(&refresh_time);

  while (1) {
    sleep(1);
    time(&current_time);
    equipment_status = redisCommand(context, "GET equipment_status");
    previous_equipment_status = redisCommand(context, "GET previous_equipment_status");
    status_queue = redisCommand(context, "LRANGE messages 0 -1");
    if (equipment_status == NULL ||
        previous_equipment_status == NULL ||
        status_queue == NULL) {
      sd_journal_send("MESSAGE=%s", "Failed to get redis response.", "PRIORITY=%i", LOG_ERR, NULL);
      continue;
    }
    get_int_key(context, "status_refresh", &refresh_status);
    get_int_key(context, "shutdown", &shutdown);
    if (shutdown) {
      time_print = localtime(&current_time);
      strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
      buffer_size = strlen(time_buffer) + strlen(last_status) + strlen(location) + 3;
      message = (char *) malloc(buffer_size * sizeof(char));
      memset(message, 0, buffer_size * sizeof(char));
      strcpy(message, time_buffer);
      strcat(message, " ");
      strcat(message, last_status);
      strcat(message, " ");
      strcat(message, location);
      push_message_status = push_message(context, message);
      free(message);
      if (!push_message_status)
        continue;
    }

    // TODO: when the new operator is different from the previous, update
    // TODO: when the new supervisor is different from the previous, update
    // TODO: GNSS query

    // Send SMS when the message queue limit is reached or there is shutdown signal.
    if (status_queue->type == REDIS_REPLY_ARRAY && (status_queue->elements >= message_limit || shutdown))
      send_equipment_status(context);

    if (equipment_status->type == REDIS_REPLY_STRING && !shutdown) {
      if (refresh_status) {
        if (previous_equipment_status->type == REDIS_REPLY_STRING && strcmp(previous_equipment_status->str, equipment_status->str) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            if (!redis_cmd(context, "SET status_refresh 0"))
              continue;
            time_print = localtime(&current_time);
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
            buffer_size = strlen(time_buffer) + strlen(equipment_status->str) + strlen(location) + 3;
            message = (char *) malloc(buffer_size * sizeof(char));
            memset(message, 0, buffer_size * sizeof(char));
            strcpy(message, time_buffer);
            strcat(message, " ");
            strcat(message, equipment_status->str);
            strcat(message, " ");
            strcat(message, location);
            push_message_status = push_message(context, message);
            free(message);
            if (!push_message_status)
              continue;
            refresh_time = current_time;
          }
        } else {
          if (!redis_cmd(context, "SET previous_equipment_status %s", equipment_status->str)) {
            continue;
          }
          refresh_time = current_time;
        }
      } else {
        if ((refresh_time + seconds_location_period) <= current_time) {
          // TODO: GNSS query
          time_print = localtime(&current_time);
          strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
          buffer_size = strlen(time_buffer) + strlen(equipment_status->str) + strlen(location) + 3;
          message = (char *) malloc(buffer_size * sizeof(char));
          memset(message, 0, buffer_size * sizeof(char));
          strcpy(message, time_buffer);
          strcat(message, " ");
          strcat(message, equipment_status->str);
          strcat(message, " ");
          strcat(message, location);
          push_message_status = push_message(context, message);
          free(message);
          if (!push_message_status)
            continue;
          refresh_time = current_time;
        }
      }
    }
    freeReplyObject(equipment_status);
    freeReplyObject(previous_equipment_status);
    freeReplyObject(status_queue);

    if (shutdown)
      break;
  }
  return;
}
