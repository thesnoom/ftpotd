#ifndef __SOCKETS_H
#define __SOCKETS_H

#include <string>
#include <vector>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/buffer.h>

#include "db.hpp"
#include "virtfs.hpp"


using namespace std;

class ftpc;

class asyncsock
{
    public:
        asyncsock(cdb *db, cvirtfs *vfs, int port = 21);
        ~asyncsock();

        cdb *db;
        cvirtfs *vfs;

        struct event_base *evbase = nullptr;

        void start_socket();


    private:
        int port;

        static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ptr);
        static void accept_error_cb(struct evconnlistener *listener, void *ptr);

        static void ftp_read_cb(struct bufferevent *evbuff, void *ptr);
        static void ftp_event_cb(struct bufferevent *evbuff, short events, void *ptr);
};


class ftpc
{
    public:
        ftpc(asyncsock *sockbase, struct bufferevent *evbuff, string lip, string rip);
        ~ftpc();

        void handle_req();


    private:
        asyncsock *sockbase;
        string remote_ip, local_ip;
        string conid;
        struct bufferevent *evbuff;
        struct bufferevent *evdata;

        cvirtfs *vfs_curr;

        void gen_conid();
        
        vector<char *> split;

        typedef struct _ftp_state {
            bool    auth;
            int     attempts;
            bool    data;
            bool    usercmd;
            string  user;            
        } ftp_state, *pftp_state;

        pftp_state state;

        void handle_user();
        void handle_pass();
        void handle_pasv();
        void handle_port();
        void handle_cwd();
        void handle_pwd();
        void handle_cdup();
        void handle_list();
        void handle_syst();

        unsigned short find_pasv_port();

        static void pasv_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ptr);
        static void pasv_error_cb(struct evconnlistener *listener, void *ptr);
        static void pasv_event_cb(struct bufferevent *evbuff, short events, void *ptr);
        static void port_event_cb(struct bufferevent *evbuff, short events, void *ptr);
};

#endif
