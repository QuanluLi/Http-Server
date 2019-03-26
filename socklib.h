#pragma once

/* socklib,cpp
 * 
 * This file contains functions used lots when writing internet
 * ckukebt/server programs. The two main functions here are:
 * 
 * int make_server_socket(portnum) returns a server socket
 *                                 or -1 if error
 * int make_server_socket_q(portnum, backlog)
 * 
 * int connect_to_server(char *hostname, int portnum)
 *                      return a connected socket
 *                      or -1 if error
 */

int make_server_socket(int portnum);
int make_server_socket_q(int portnum, int backlog);
int connect_to_server(char *host, int portnum);
