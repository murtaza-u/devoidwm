#ifndef TAGS_H
#define TAGS_H

#include "client.h"
#include "key.h"

struct Tag {
    Client *focused;
    bool fullscrlock;
};
extern struct Tag tags[9];

void setup_tags();
void save_fullscrlock(Client *c);
bool get_fullscrlock(unsigned int);
void view(Arg arg);
void toggletag(Arg arg);
void sendclient(Arg arg);

#endif
