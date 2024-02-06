#ifndef CHATTER_H
#define CHATTER_H

#include <time.h>

struct Message {
  char *text;
  char *user;
  time_t time;
};

#endif // CHATTER_H
