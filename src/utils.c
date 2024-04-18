#define PCRE2_CODE_UNIT_WIDTH 8

#include <fcntl.h>
#include <glib.h>
#include <pcre2.h>
#include <systemd/sd-journal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

static GMutex mutex;

int get_char_key(const char  *key,
                 char       **value)
{
  int out = 0;
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    return out;
  }
  redisReply *reply = redisCommand(context, "GET %s", key);
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error getting key: %s", key, "PRIORITY=%i", LOG_ERR, NULL);
    redisFree(context);
    return out;
  }
  if (reply->type == REDIS_REPLY_STRING) {
    g_free(*value);
    int buffer_size = strlen(reply->str);
    *value = (char *) g_malloc(buffer_size * sizeof(char));
    strcpy(*value, reply->str);
    out = 1;
  }
  freeReplyObject(reply);
  redisFree(context);
  return out;
}

int get_int_key(const char *key,
                int        *value)
{
  int out = 0;
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    return out;
  }
  redisReply *reply = redisCommand(context, "GET %s", key);
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error getting key: %s", key, "PRIORITY=%i", LOG_ERR, NULL);
    redisFree(context);
    return out;
  }
  if (reply->type == REDIS_REPLY_STRING) {
    *value = atoi(reply->str);
    out = 1;
  }
  freeReplyObject(reply);
  redisFree(context);
  return out;
}

int redis_cmd(const char *cmd,
              const char *key,
              const char *x)
{
  int out = 0;
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    return out;
  }
  char *buffer;
  redisReply *reply;
  if (cmd && key && x) {
    buffer = (char *) malloc((strlen(cmd) + strlen(key) + strlen(x) + 3) * sizeof(char));
    sprintf(buffer, "%s %s %s", cmd, key, x);
    reply = redisCommand(context, "%s %s %s", cmd, key, x);
  }
  else if (cmd && key) {
    buffer = (char *) malloc((strlen(cmd) + strlen(key) + 2) * sizeof(char));
    sprintf(buffer, "%s %s", cmd, key);
    reply = redisCommand(context, "%s %s", cmd, key);
  }
  else {
    g_error("Invalid redis command.");
    return out;
  }
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error executing Redis command: %s", buffer, "PRIORITY=%i", LOG_ERR, NULL);
  } else {
    if (reply->type != REDIS_REPLY_ERROR)
      out = 1;
    freeReplyObject(reply);
  }
  free(buffer);
  redisFree(context);
  return out;
}

int send_equipment_status(redisContext *context)
{
  int output = 0;
  redisReply *status_queue = NULL;
  status_queue = redisCommand(context, "LRANGE messages 0 -1");
  if (status_queue == NULL) {
    sd_journal_send("MESSAGE=%s", "Failed to get status queue while attempting to send message.", "PRIORITY=%i", LOG_ERR, NULL);
    output = 1;
  } else {
    if (status_queue->type == REDIS_REPLY_ARRAY) {
      char captured_datetime[20];
      char captured_equipment_status[11];
      char captured_location[10];
      char datetime_substr[20];
      char *messages;
      char new_captured_datetime[20];
      char new_captured_equipment_status[11];
      char new_captured_location[10];
      int buffer_size;
      int counter;
      int total_characters = 0;
      for (counter = 0; counter < status_queue->elements; counter++)
        total_characters += strlen(status_queue->element[counter]->str);
      buffer_size = total_characters + (status_queue->elements * 2);
      messages = (char *) malloc(buffer_size * sizeof(char));
      memset(messages, 0, buffer_size * sizeof(char));
      strcpy(messages, status_queue->element[0]->str);
      strcat(messages, "\n");
      capture_pattern(status_queue->element[0]->str, captured_datetime, captured_equipment_status, captured_location);
      for (counter = 1; counter < status_queue->elements; counter++) {
        capture_pattern(status_queue->element[counter]->str, new_captured_datetime, new_captured_equipment_status, new_captured_location);
        str_difference(captured_datetime, new_captured_datetime, datetime_substr);
        strcat(messages, datetime_substr);
        strcat(messages, " ");
        if (strcmp(captured_equipment_status, new_captured_equipment_status) != 0) {
          strcat(messages, new_captured_equipment_status);
          strcat(messages, " ");
        }
        strcat(messages, new_captured_location);
        strcat(messages, "\n");
        strcpy(captured_datetime, new_captured_datetime);
        strcpy(captured_equipment_status, new_captured_equipment_status);
      }
      // send_sms("All messages:");
      // TODO: return the status of sending messages and delete cache if sending
      // is success.
      send_sms(messages);
      free(messages);

      if (!redis_cmd("DEL", "messages", NULL))
        output = 3;
    } else {
      output = 2;
    }
    freeReplyObject(status_queue);
  }
  return output;
}

int set_hat_time(void)
{
  char *rtc = NULL;
  char time_buffer[32];
  int out = 0;
  time_t current_time;
  time(&current_time);
  struct tm *time_print = localtime(&current_time);
  strftime(time_buffer, sizeof(time_buffer), "AT+CCLK=\"%y/%m/%d,%H:%M:%S+32\"", time_print);
  at_cmd(time_buffer, &rtc, 1);
  const char *success = strstr(rtc, "OK");
  if (success)
    out = 1;
  g_free(rtc);
  return out;
}

void at_cmd(const char    *cmd,
            char         **response,
            unsigned int   timeout)
{
  g_mutex_lock(&mutex);
  char *at_command;
  char at_response[1024];
  int serial_file = -1;
  get_int_key("serial_file", &serial_file);
  if (serial_file < 0) {
    sd_journal_send("MESSAGE=Error establishing serial connection while running AT command: %s", cmd, "PRIORITY=%i", LOG_ERR, NULL);
    return;
  }
  at_command = (char *) g_malloc((strlen(cmd) + 3) * sizeof(char));
  if (at_command == NULL) {
    sd_journal_send("MESSAGE=AT Command allocation error: %s", cmd, "PRIORITY=%i", LOG_ERR, NULL);
    return;
  }
  strcpy(at_command, cmd);
  strcat(at_command, "\r\n");
  write(serial_file, at_command, strlen(at_command));
  sleep(timeout);
  ssize_t num_bytes = read(serial_file, at_response, sizeof(at_response));
  if (num_bytes > 0)
    at_response[num_bytes] = '\0';
  g_free(*response);
  *response = (char *) g_malloc(strlen(at_response) * sizeof(char));
  strcpy(*response, at_response);
  g_free(at_command);
  g_mutex_unlock(&mutex);
  return;
}

void capture_pattern(const char *source,
                     char       *datetime,
                     char       *status,
                     char       *location)
{
  const PCRE2_SPTR pattern = (PCRE2_SPTR) "^(\\d{4}-\\d{2}-\\d{2}-\\d{2}:\\d{2}:\\d{2})\\s+([a-z\\-]+)\\s+(LOCATION).*";
  int error_code;
  int matches;
  pcre2_code *re;
  PCRE2_SIZE error_offset;
  re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL);
  if (re == NULL) {
    PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(error_code, buffer, sizeof(buffer));
    sd_journal_send("MESSAGE=PCRE2 compilation failed at offset %d: %s", (int) error_offset, buffer, "PRIORITY=%i", LOG_ERR, NULL);
    return;
  }
  pcre2_match_data *match_data;
  match_data = pcre2_match_data_create_from_pattern(re, NULL);
  matches = pcre2_match(re, (PCRE2_SPTR) source, strlen(source), 0, 0, match_data, NULL);
  if (matches < 0) {
    switch(matches) {
      case PCRE2_ERROR_NOMATCH:
        sd_journal_send("MESSAGE=%s", "PCRE2 no matches", "PRIORITY=%i", LOG_ERR, NULL);
        break;
      default:
        sd_journal_send("MESSAGE=PCRE2 matching error %d", matches, "PRIORITY=%i", LOG_ERR, NULL);
        break;
    }
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return;
  }
  const PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
  PCRE2_SPTR substring_datetime = (PCRE2_SPTR) source + ovector[2];
  strncpy(datetime, (char *) substring_datetime, ovector[3] - ovector[2]);
  datetime[ovector[3] - ovector[2]] = '\0';
  PCRE2_SPTR substring_status = (PCRE2_SPTR) source + ovector[4];
  strncpy(status, (char *) substring_status, ovector[5] - ovector[4]);
  status[ovector[5] - ovector[4]] = '\0';
  PCRE2_SPTR substring_location = (PCRE2_SPTR) source + ovector[6];
  strncpy(location, (char *) substring_location, ovector[7] - ovector[6]);
  location[ovector[7] - ovector[6]] = '\0';
  pcre2_code_free(re);
  pcre2_match_data_free(match_data);
  return;
}

void initialize_serial_connection(int *serial_file_descriptor)
{
  int baud_rate = B115200;
  *serial_file_descriptor = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
  if (*serial_file_descriptor < 0)
    return;
  struct termios tty;
  if (tcgetattr(*serial_file_descriptor, &tty) != 0) {
    close(*serial_file_descriptor);
    *serial_file_descriptor = -1;
    return;
  }
  cfsetospeed(&tty, baud_rate);
  cfsetispeed(&tty, baud_rate);
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 10;
  if (tcsetattr(*serial_file_descriptor, TCSANOW, &tty) != 0) {
    close(*serial_file_descriptor);
    *serial_file_descriptor = -1;
  }
  return;
}

void send_sms(const char *format,
              ...)
{
  const char ctr_z = '\x1A';
  int serial_file = -1;
  while (serial_file < 0 )
    get_int_key("serial_file", &serial_file);
  char *response = NULL;
  va_list args;
  va_start(args, format);
  int len = vsnprintf(NULL, 0, format, args) + 1;
  va_end(args);
  char *text = (char *) g_malloc(len * sizeof(char));
  va_start(args, format);
  vsnprintf(text, len, format, args);
  va_end(args);
  // TODO: send actual SMS
  printf("%s\n", text);
  if (strlen(text)> 159) {
    sd_journal_send("MESSAGE=%s", "Unsupported SMS length.", "PRIORITY=%i", LOG_ERR, NULL);
    printf("Text too long with length: %i.\n", (int) strlen(text));
  }
  else {
    sleep(1);
    at_cmd("AT+CMGS=\"+639288984366\"", &response, 1);
    write_serial(text, serial_file);
    write_serial(&ctr_z, serial_file);
  }
  g_free(text);
  return;
}

void str_copy(char       **destination,
              const char  *source)
{
  g_free(*destination);
  *destination = (char *) g_malloc(strlen(source) * sizeof(char));
  strcpy(*destination, source);
  return;
}

void str_difference(const char *old,
                    const char *new,
                    char       *holder)
{
  int counter = 0;
  int idx;
  holder[0] = '\0';
  while (1) {
    if (old[counter] != new[counter])
      break;
    counter += 1;
  }
  for (idx = 0; counter < strlen(new); idx++) {
    holder[idx] = new[counter];
    counter += 1;
  }
  holder[idx] = '\0';
  return;
}

void write_serial(const char *buffer,
                  const int   serial_file)
{
  g_mutex_lock(&mutex);
  write(serial_file, buffer, strlen(buffer));
  g_mutex_unlock(&mutex);
  return;
}

