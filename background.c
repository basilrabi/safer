#include <hiredis/hiredis.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>
#include <time.h>

int main(int argc, char **argv)
{
  int message_limit = 10;           // Number of messages to be queued before sending
  int seconds_refresh_cutoff = 10;  // Number of seconds before the new equipment status is recorded to allow making mistakes
  int seconds_location_period = 30; // Number of seconds between location queries

  char time_buffer[20];
  char *location = "LOCATION"; // Temporary placeholder for GNSS query
  char *message;
  char *messages;
  int buffer_size;
  int connected = 0;
  int counter;
  int total_characters;
  redisContext *context = NULL;
  redisReply *equipment_status = NULL;
  redisReply *previous_equipment_status = NULL;
  redisReply *refresh_status = NULL;
  redisReply *set_command = NULL;
  redisReply *status_queue = NULL;
  struct tm *time_print;
  time_t current_time;
  time_t refresh_time;

  // Connection Loop
  while (connected == 0) {
    context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
        redisFree(context);
      } else {
        sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
      }
    } else {
      connected = 1;
    }
  }

  // TODO: Connect via serial connection
  // TODO: Real time clock sync

  time(&refresh_time);

  // Main loop
  while (connected == 1) {
    time(&current_time);
    equipment_status = redisCommand(context, "GET equipment_status");
    previous_equipment_status = redisCommand(context, "GET previous_equipment_status");
    refresh_status = redisCommand(context, "GET status_refresh");
    status_queue = redisCommand(context, "LRANGE messages 0 -1");
    if (equipment_status == NULL ||
        previous_equipment_status == NULL ||
        refresh_status == NULL ||
        status_queue == NULL) {
      sd_journal_send("MESSAGE=%s", "Failed to get redis response.", "PRIORITY=%i", LOG_ERR, NULL);
      continue;
    }

    // Send SMS when the message queue limit is reached.
    if (status_queue->type == REDIS_REPLY_ARRAY &&
        status_queue->elements >= message_limit) {
      total_characters = 0;
      for (counter = 0; counter < status_queue->elements; counter++) {
        total_characters += strlen(status_queue->element[counter]->str);
      }
      if (total_characters < 10 ) {
        sd_journal_send("MESSAGE=%s", "Invalid character count.", "PRIORITY=%i", LOG_ERR, NULL);
        continue;
      }
      buffer_size = total_characters + status_queue->elements;
      messages = (char *) malloc(buffer_size * sizeof(char));
      memset(messages, 0, buffer_size * sizeof(char));
      strcpy(messages, status_queue->element[0]->str);
      strcat(messages, "\n");
      for (counter = 1; counter < status_queue->elements; counter++) {
        strcat(messages, status_queue->element[counter]->str);
        strcat(messages, "\n");
      }
      printf("All messages:\n");
      printf("%s\n", messages);
      set_command = redisCommand(context, "DEL messages");
      freeReplyObject(set_command);
      free(messages);
    }

    if (equipment_status->type == REDIS_REPLY_STRING) {
      if (refresh_status->type == REDIS_REPLY_STRING &&
          strcmp(refresh_status->str, "1") == 0) {
        if (previous_equipment_status->type == REDIS_REPLY_STRING &&
            strcmp(previous_equipment_status->str, equipment_status->str) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            set_command = redisCommand(context, "SET status_refresh 0");
            freeReplyObject(set_command);
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
            free(message);
            freeReplyObject(set_command);
            refresh_time = current_time;
          }
        } else {
          set_command = redisCommand(context, "SET previous_equipment_status %s", equipment_status->str);
          freeReplyObject(set_command);
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
          free(message);
          freeReplyObject(set_command);
          refresh_time = current_time;
        }
      }
    }

    freeReplyObject(equipment_status);
    freeReplyObject(previous_equipment_status);
    freeReplyObject(refresh_status);
    freeReplyObject(status_queue);
  }
  return 0;
}
