#ifndef HANDLE_H
#define HANDLE_H

#include <stdio.h>
#include "common.h"
/*-----------------------------------------------------------------
    process_rq(char *rq, int fd)
    do what the request asks for and write reply to fd
    hadnles request in a new process
    rq is HTTP command: GET /foo.bar.teml HTTP/1.0
------------------------------------------------------------------*/

void *handle_call(void *fdptr);
void process_rq(char *rq, int fd, FILE *fpin); //
void *process_rp(void *fdptr);

void do_404(char *item, int fd);
void do_ls(char *dir, int fd);

/* di_car(filename, fd): sends header then the contents */
void do_cat(char *f, int fd);

void execute_cgi(int fd, struct user_data *ud);

#endif