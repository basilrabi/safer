#include <hiredis/hiredis.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include "worker.h"
#include "utils.h"

void status_sender(gpointer data) {
  // Number of messages to be queued before sending
  const int message_limit = 10;
  // Number of seconds before the new equipment status is recorded to allow making mistakes
  const int seconds_refresh_cutoff = 10;
  // Number of seconds between location queries
  const int seconds_location_period = 30;

  char time_buffer[20];
  char *location = "LOCATION"; // Temporary placeholder for GNSS query
  char *message;
  char *last_message;
  char *last_status = "off";
  int buffer_size;
  int shutdown = 0;
  redisContext *context = (redisContext *) data;
  redisReply *equipment_status = NULL;
  redisReply *previous_equipment_status = NULL;
  redisReply *refresh_status = NULL;
  redisReply *set_command = NULL;
  redisReply *shutdown_status = NULL;
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
    refresh_status = redisCommand(context, "GET status_refresh");
    shutdown_status = redisCommand(context, "GET shutdown");
    status_queue = redisCommand(context, "LRANGE messages 0 -1");
    if (equipment_status == NULL ||
        previous_equipment_status == NULL ||
        refresh_status == NULL ||
        shutdown_status == NULL ||
        status_queue == NULL) {
      sd_journal_send("MESSAGE=%s", "Failed to get redis response.", "PRIORITY=%i", LOG_ERR, NULL);
      continue;
    }

    if (shutdown_status->type == REDIS_REPLY_STRING && strcmp(shutdown_status->str, "1") == 0) {
      shutdown = 1;
      time_print = localtime(&current_time);
      strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
      buffer_size = strlen(time_buffer) + strlen(last_status) + strlen(location) + 3;
      last_message = (char *) malloc(buffer_size * sizeof(char));
      memset(last_message, 0, buffer_size * sizeof(char));
      strcpy(last_message, time_buffer);
      strcat(last_message, " ");
      strcat(last_message, last_status);
      strcat(last_message, " ");
      strcat(last_message, location);
      set_command = redisCommand(context, "RPUSH messages %s", last_message);
      free(last_message);
      if (set_command == NULL) {
        sd_journal_send("MESSAGE=%s", "Failed to get redis response while pushing update.", "PRIORITY=%i", LOG_ERR, NULL);
        continue;
      } else {
        freeReplyObject(set_command);
      }
    }

    // TODO: when the new operator is different from the previous, update
    // TODO: when the new supervisor is different from the previous, update
    // TODO: GNSS query

    // Send SMS when the message queue limit is reached.
    if (status_queue->type == REDIS_REPLY_ARRAY && (status_queue->elements >= message_limit || shutdown == 1)) {
      send_equipment_status(context);
    }

    if (equipment_status->type == REDIS_REPLY_STRING && shutdown == 0) {
      if (refresh_status->type == REDIS_REPLY_STRING && strcmp(refresh_status->str, "1") == 0) {
        if (previous_equipment_status->type == REDIS_REPLY_STRING && strcmp(previous_equipment_status->str, equipment_status->str) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            set_command = redisCommand(context, "SET status_refresh 0");
            if (set_command == NULL) {
              sd_journal_send("MESSAGE=%s", "Failed to set status_refresh.", "PRIORITY=%i", LOG_ERR, NULL);
              continue;
            } else {
              freeReplyObject(set_command);
            }
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
            set_command = redisCommand(context, "RPUSH messages %s", message);
            if (set_command == NULL) {
              sd_journal_send("MESSAGE=%s", "Failed to get redis response while pushing update.", "PRIORITY=%i", LOG_ERR, NULL);
              continue;
            } else {
              freeReplyObject(set_command);
            }
            free(message);
            refresh_time = current_time;
          }
        } else {
          set_command = redisCommand(context, "SET previous_equipment_status %s", equipment_status->str);
          if (set_command == NULL) {
            sd_journal_send("MESSAGE=%s", "Failed to set previous_equipment_status.", "PRIORITY=%i", LOG_ERR, NULL);
            continue;
          } else {
            freeReplyObject(set_command);
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
          set_command = redisCommand(context, "RPUSH messages %s", message);
          if (set_command == NULL) {
            sd_journal_send("MESSAGE=%s", "Failed to get redis response while pushing location update.", "PRIORITY=%i", LOG_ERR, NULL);
            continue;
          } else {
            freeReplyObject(set_command);
          }
          free(message);
          refresh_time = current_time;
        }
      }
    }
    freeReplyObject(equipment_status);
    freeReplyObject(previous_equipment_status);
    freeReplyObject(refresh_status);
    freeReplyObject(shutdown_status);
    freeReplyObject(status_queue);

    if (shutdown) {
      break;
    }
  }
  return;
}
