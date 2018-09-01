#include "sockets.hpp"
#include "db.hpp"


#define SRV_BANNER "220 ftpotd\r\n"


// Asynchronous socket constructor
// Takes in database and file system object, and port to bind to. 
asyncsock::asyncsock(cdb *db, cvirtfs *vfs, int port)
{
	this->port	= port;
	this->db	= db;
    this->vfs   = vfs;
}


// Destructor, delete the db class. 
asyncsock::~asyncsock()
{
   delete this->db; 
}


// Create an event base
// Fill out sock information
// Start an asynchronous bind, callback to function accept_cb
// Set our error callback
// Enter the event loop
void asyncsock::start_socket()
{
	struct evconnlistener *ftp_listen = nullptr;
	struct sockaddr_in ftp_saddr = { 0 };

	this->evbase = event_base_new();
	if(!this->evbase)
	{
    	// ERROR
	}
   
	memset(&ftp_saddr, 0, sizeof(ftp_saddr));
	ftp_saddr.sin_family        = AF_INET;
	ftp_saddr.sin_addr.s_addr   = htonl(0);
	ftp_saddr.sin_port          = htons(this->port);

	ftp_listen = evconnlistener_new_bind(this->evbase, asyncsock::accept_cb, (void *)this, (LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE), -1, (struct sockaddr *)&ftp_saddr, sizeof(ftp_saddr));
	if(!ftp_listen)
	{
		// ERROR LISTENER
	}
    
	evconnlistener_set_error_cb(ftp_listen, asyncsock::accept_error_cb);

	event_base_dispatch(this->evbase);
}


// Callback function executed when a connection is established to us.
// Here we take in the remote IP and determine local IP
// Make a new buffer for the socket and set the callbacks
// We also pass through a pointer to the new client object made for this client. 
// Lastly we send the FTP banner. 
void asyncsock::accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ptr)
{
	asyncsock *sockbase = (asyncsock *)ptr;

    struct sockaddr_storage _addr;
    struct sockaddr_in *addr;
    socklen_t slen = sizeof(_addr);
    getpeername(fd, (struct sockaddr *)&_addr, &slen);
    addr = (struct sockaddr_in *)&_addr;

    char ip[16] = { 0 };
    string rip, lip = string(inet_ntoa(((struct sockaddr_in *)address)->sin_addr));
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

    rip.assign(ip);

    // Buffer event for the socket
    struct bufferevent *evbuff = bufferevent_socket_new(sockbase->evbase, fd, BEV_OPT_CLOSE_ON_FREE);

    // New client, sockbase, event and IPs passed through.
    ftpc *client = new ftpc(sockbase, evbuff, lip, rip);

    // Set callbacks.
    bufferevent_setcb(evbuff, asyncsock::ftp_read_cb, nullptr, asyncsock::ftp_event_cb, (void *)client);
    bufferevent_enable(evbuff, (EV_READ|EV_WRITE));

    // Banner send
    bufferevent_write(evbuff, SRV_BANNER, strlen(SRV_BANNER));
}
 

// Error on our listener socket.
void asyncsock::accept_error_cb(struct evconnlistener *listener, void *ptr)
{
    event_base_loopexit(((asyncsock *)ptr)->evbase, nullptr);
}


// We have some data!  Use our ftpc client object passed through to handle the request. 
void asyncsock::ftp_read_cb(struct bufferevent *evbuff, void *ptr)
{
    ((ftpc *)ptr)->handle_req();
}


//  Some error has occured with a client, delete it.
void asyncsock::ftp_event_cb(struct bufferevent *evbuff, short events, void *ptr)
{
    delete (ftpc *)ptr;
}


// Client constructor.  
// Takes in remote and local IP, our asyncsock parent object 
// Create a state struct for our client
// Generate a connection ID
// Log the connection
ftpc::ftpc(asyncsock *sockbase, struct bufferevent *evbuff, string lip, string rip)
{
    this->sockbase  = sockbase;
    this->evbuff    = evbuff;
    this->local_ip  = lip;
    this->remote_ip = rip;
    this->state     = new ftp_state();
    this->vfs_curr  = sockbase->vfs;
    
    this->state->auth       = false;
    this->state->data       = false;
    this->state->usercmd    = false;

    this->gen_conid();
	
    this->sockbase->db->log_conn(this->remote_ip, this->conid, false);

    cout << "[" << this->conid << "] Client connected (" << this->remote_ip << ")" << endl;
}


// Client destructor,
// free our event buffer for this client, free the state of the client
// log the disconnection
ftpc::~ftpc()
{
	bufferevent_free(this->evbuff);

    if(this->state->data)
        bufferevent_free(this->evdata);

    this->sockbase->db->log_conn(this->remote_ip, this->conid, true);

    cout << "[" << this->conid << "] Client disconnected (" << this->remote_ip << ")" << endl;

    delete this->state;
}



// Generate a random 15 character string from our character set A-z0-9 
void ftpc::gen_conid()
{
    static char id_charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    srand(time(0));

    for(int i = 0; i < 15; i++)
        this->conid += id_charset[rand() % (strlen(id_charset) - 1)];

}


// Function to parse commands sent to the server. 
void ftpc::handle_req()
{
    uint8_t req[4096] = { 0 };
    this->split.clear();

    // Read into the 4kb buffer
    bufferevent_read(this->evbuff, &req, sizeof(req));

    // Split by space, string terminate too.
    // Push each token into our vector of chars
    char *tok = strtok((char *)req, " ");
    while(tok)
    {
        if(!strncmp((tok + (strlen(tok) - 2)), "\r\n", strlen(tok)))
            tok[(strlen(tok) - 2)] = '\0';
        else if(!strncmp((tok + (strlen(tok) - 1)), "\n", strlen(tok)))
            tok[(strlen(tok) - 1)] = '\0';

        this->split.push_back(tok);

        tok = strtok(nullptr, " ");
    }

    char *cmd       = this->split.front(); // command to edit to lowercase
    string log;

    for(size_t i = 0; i < this->split.size(); i++)
        log += (this->split[i] + string(" "));

    for(size_t i = 0; i < strlen(cmd); i++)
        cmd[i] = tolower(cmd[i]);

    string cur_dir;     
    this->vfs_curr->get_full_path(this->vfs_curr, true, &cur_dir);

    this->sockbase->db->log_activity(this->conid, log, cur_dir, time(nullptr));

    if(strlen(cmd) > 4)
    {
        bufferevent_write(this->evbuff, "500 Unknown command\r\n", strlen("500 Unknown command\r\n"));

        return; // Unknown command
    }

    // Exit this client. 
    if(!strcmp(cmd, "quit"))
    {
        int sock = bufferevent_getfd(this->evbuff);
        send(sock, "221 Goodbye.\r\n", strlen("221 Goodbye.\r\n"), 0);

        delete this;

        return;
    }

    // Must be authed. 
    if(!this->state->auth)
    {
        // If we don't have a valid USER command, go to USER handler 
        if(!this->state->usercmd)
        {
            this->handle_user();

            return;
        } else { // We have a user, go to pass.  
            this->handle_pass();
            
            return;
        }
    } else {
        if(!strcmp(cmd, "list")) {
            this->handle_list();

            return;
        } else if(!strcmp(cmd, "pasv")) {
            this->handle_pasv();

            return;
        } else if(!strcmp(cmd, "port")) {
            this->handle_port();

            return;
        } else if(!strcmp(cmd, "cdup")) {
            this->handle_cdup();

            return;
        } else if(!strcmp(cmd, "cwd")) {
            this->handle_cwd();

            return;
        } else if(!strcmp(cmd, "pwd")) {
            this->handle_pwd();

            return;
        } else if(!strcmp(cmd, "syst")) {
            this->handle_syst();

            return;
        } else {
            bufferevent_write(this->evbuff, "500 Unknown command\r\n", strlen("500 Unknown command\r\n"));

            // log activity. 

            return;
        }
    }
}


// USER command
void ftpc::handle_user()
{
    if(strcmp(this->split.front(), "user"))
    {
        bufferevent_write(this->evbuff, "530 Please login with USER and PASS\r\n", strlen("530 Please login with USER and PASS\r\n"));
        
        // LOG split.front()
    } else {
        if(this->state->attempts == 3)
        {
            bufferevent_write(this->evbuff, "421 Service not available\r\n", strlen("421 Service not available\r\n"));
            delete this;

            return;
        }

        if(this->split.size() >= 1)
        {
            this->state->user       = this->split[1];
            this->state->usercmd    = true;
        }

        bufferevent_write(this->evbuff, "331 Please specify the password\r\n", strlen("331 Please specify the password\r\n"));
    }
}


// PASS command
// Set auth state if login is succesful.
// Up attempts if it's wrong
void ftpc::handle_pass()
{
    this->state->usercmd = false;

    if(strcmp(this->split.front(), "pass"))
    {
        bufferevent_write(this->evbuff, "530 Please login with USER and PASS\r\n", strlen("530 Please login with USER and PASS\r\n"));

        // LOG split.front()
    } else {
        if(this->split.size() > 1 && this->sockbase->db->ftp_auth(this->state->user, this->split[1]))
        {
            this->state->auth       = true;
            this->state->attempts   = 0;

            this->sockbase->db->log_auth(this->conid, this->state->user, this->split[1], time(nullptr));

            bufferevent_write(this->evbuff, "230 Login successful\r\n", strlen("230 Login successful\r\n"));
        } else {
            this->state->attempts++;

            this->sockbase->db->log_att(this->conid, this->state->user, this->split[1], time(nullptr));

            bufferevent_write(this->evbuff, "530 Login incorrect\r\n", strlen("530 Login incorrect\r\n"));
        }
    }
}

 
// PASV command
// We find a port suitable to bind and
// initialise a new asynchronous bind on said port.
// More callbacks for the new listener socket.
void ftpc::handle_pasv()
{
	struct evconnlistener *pasv_listen = nullptr;
	struct sockaddr_in pasv_saddr = { 0 };

    unsigned short port = find_pasv_port();

	memset(&pasv_saddr, 0, sizeof(pasv_saddr));
	pasv_saddr.sin_family        = AF_INET;
	pasv_saddr.sin_addr.s_addr   = htonl(0);
	pasv_saddr.sin_port          = htons(port);

	pasv_listen = evconnlistener_new_bind(this->sockbase->evbase, ftpc::pasv_accept_cb, (void *)this, (LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE), -1, (struct sockaddr *)&pasv_saddr, sizeof(pasv_saddr));
	if(!pasv_listen)
	{

        cout << "E: " << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR()) << endl;

		// ERROR HERE
        return; 
	}
   
	evconnlistener_set_error_cb(pasv_listen, ftpc::pasv_error_cb);

    char buf[48] = { 0 };

    for(size_t i = 0; i < this->local_ip.size(); i++)
        if(this->local_ip[i] == '.')
            this->local_ip[i] = ',';

    // Construct Passive string
    snprintf(buf, sizeof(buf), "227 Entering Passive Mode (%s,%u,%u)\r\n", this->local_ip.c_str(), port >> 8, port & 0xFF);

    bufferevent_write(this->evbuff, buf, strlen(buf));
}


// PASV accept callback
// If we have a data channel for this client already, close it. 
// Make a new one and set the callbacks, set the data channel state to true. 
void ftpc::pasv_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ptr)
{
	ftpc *client = (ftpc *)ptr;

    if(client->state->data)
        bufferevent_free(client->evdata);
    
    client->evdata = bufferevent_socket_new(client->sockbase->evbase, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(client->evdata, nullptr, nullptr, ftpc::pasv_event_cb, (void *)client);
    bufferevent_enable(client->evdata, (EV_READ|EV_WRITE));

    client->state->data = true;
}


// Error on PASV socket
// Close the data socket and set the state to false
void ftpc::pasv_error_cb(struct evconnlistener *listener, void *ptr)
{
	ftpc *client = (ftpc *)ptr;

    bufferevent_free(client->evdata);
    client->state->data = false;
}


// PASV disconnect
// Same as above. 
void ftpc::pasv_event_cb(struct bufferevent *evbuff, short events, void *ptr)
{
	ftpc *client = (ftpc *)ptr;

    bufferevent_free(client->evdata);
    client->state->data = false;
}


// PORT command
// Parse our received PORT command and try to connect to it. 
void ftpc::handle_port()
{
    if(this->split.size() < 1)
    {
        // No argument
        
        return;
    } else {
        char ch, *port = strdup(this->split[1]);

        if(strlen(port) > 23) // 111,111,111,111,111,111 max... 23 char
        {
            // Incorrect syntax, log
        } else {
            int c = 0, v = 1;

            for(size_t i = 0; i < strlen(port); i++)
            {
                ch = port[i];
                if(ch == ',')
                    c++;
                else if(!isdigit(ch)) {
                    v = 0;
                    break;
                }
            }

            if(v && c == 5)
            {
                char *tok = strtok(port, ",");
                int pdata[6], i = 0;

                while(tok != nullptr)
                {
                    pdata[i++] = atoi(tok);
                    tok = strtok(nullptr, ",");
                }

                char r_ip[64] = { 0 };
                snprintf(r_ip, sizeof(r_ip), "%d.%d.%d.%d", pdata[0], pdata[1], pdata[2], pdata[3]);

                // cout << "IP: " << r_ip << endl; 

                struct sockaddr_in s_addr;
                inet_pton(AF_INET, r_ip, &s_addr.sin_addr); 
                s_addr.sin_family = AF_INET;
                s_addr.sin_port = htons((pdata[4] * 256) + pdata[5]);

                if(this->state->data)
                    bufferevent_free(this->evdata);

                this->evdata = bufferevent_socket_new(this->sockbase->evbase, -1, BEV_OPT_CLOSE_ON_FREE);
                bufferevent_setcb(this->evdata, nullptr, nullptr, ftpc::port_event_cb, (void *)this);

                if(bufferevent_socket_connect(this->evdata, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
                {
                    bufferevent_free(this->evdata);
                    bufferevent_write(this->evbuff, "421 Service not available\r\n", strlen("421 Service not available\r\n"));
                }
            } else {
                // Incorrect port, LOG
            }
        }

        free(port);
    }
}


// PORT event callback.
// We receive an event if we've connected, or an error has occured otherwise
void ftpc::port_event_cb(struct bufferevent *evbuff, short events, void *ptr)
{
    ftpc *client = (ftpc *)ptr;

    if(events & BEV_EVENT_CONNECTED)
    {
        client->state->data = true;

        bufferevent_write(client->evbuff, "200 Command okay\r\n", strlen("200 Command okay\r\n"));
    } else {
        bufferevent_free(client->evdata);

        client->state->data = false;
    }
}


// LIST command 
// Grab all files and folders and send them over.
// They aren't in alphabetical order and doesn't take a folder as argument. 
// Close the channel after
void ftpc::handle_list()
{
    if(!this->state->data)
        bufferevent_write(this->evbuff, "425 Use PORT or PASV first\r\n", strlen("425 Use PORT or PASV first\r\n"));
    else {
        bufferevent_write(this->evbuff, "125 Data connection already open; transfer starting\r\n", strlen("125 Data connection already open; transfer starting\r\n"));
        
        int sock = bufferevent_getfd(this->evdata);

        vector<svirtfd *> ptr_files     = this->vfs_curr->get_all_files();
        vector<cvirtfs *> ptr_folders   = this->vfs_curr->get_all_folders();

        char list_buff[2048];

        for(auto &i : ptr_folders)
        {
            memset(&list_buff, 0, sizeof(list_buff));

            snprintf(list_buff, sizeof(list_buff), "d%s %6s %6s %6d %s %s\r\n", i->get_folder_perm().c_str(), i->get_folder_owner().c_str(), i->get_folder_group().c_str(), 4096, i->get_folder_date().c_str(), i->get_folder_name().c_str()); 

            send(sock, list_buff, strlen(list_buff), 0);
        }

        for(auto &i : ptr_files)
        {
            memset(&list_buff, 0, sizeof(list_buff));

            snprintf(list_buff, sizeof(list_buff), "-%s %6s %6s %6d %s %s\r\n", i->fileperm.c_str(), i->fileowner.c_str(), i->filegroup.c_str(), i->filebytes, i->filedate.c_str(), i->filename.c_str()); 

            send(sock, list_buff, strlen(list_buff), 0);
        }

        bufferevent_free(this->evdata);
        this->state->data = false;

        bufferevent_write(this->evbuff, "226 Transfer completed\r\n", strlen("226 Transfer completed\r\n"));
    }
}


// PWD command
// Grab the full path from the current folder pointer and send it.
void ftpc::handle_pwd()
{
    string cur_dir; 
    
    this->vfs_curr->get_full_path(this->vfs_curr, true, &cur_dir);

    if(cur_dir.empty())
    {
        bufferevent_write(this->evbuff, "421 Service closing\r\n", strlen("421 Service closing\r\n"));

        delete this;

        return;
    } else {
        string resp = string("257 " + cur_dir + "\r\n");
        
        bufferevent_write(this->evbuff, resp.c_str(), resp.length());
    }
}


// CWD command
// Parse the argument and see if the folder exists
// Unfortunately doesnn't parse ../../folder/folder2/ etc.
void ftpc::handle_cwd()
{
    if(this->split.size() < 1)
    {
        // No argument
        
        return;
    } else {
        string folder;

        for(size_t i = 1; i < this->split.size(); ++i)
            if(i == (this->split.size() - 1))
                folder += this->split[i];
            else
                folder += (this->split[i] + string(" "));

        if(!folder.compare(".."))
        {
            this->handle_cdup();

            return;
        } else if(!folder.compare(".")) {
            bufferevent_write(this->evbuff, "200 Directory changed\r\n", strlen("200 Directory changed\r\n"));
            
            return;
        }

        cvirtfs *vfs_folder = this->vfs_curr->find_folder(folder);

        if(vfs_folder)
        {
            this->vfs_curr = vfs_folder;

            bufferevent_write(this->evbuff, "200 Directory changed\r\n", strlen("200 Directory changed\r\n"));
        } else {
            bufferevent_write(this->evbuff, "431 No such directory\r\n", strlen("431 No such directory\r\n"));
        }
    }
}


// CDUP command
// Change to parent folder if it exists. 
void ftpc::handle_cdup()
{
    cvirtfs *vfs_parent = this->vfs_curr->get_parent();

    if(vfs_parent)
        this->vfs_curr = vfs_parent;

    bufferevent_write(this->evbuff, "200 Directory changed\r\n", strlen("200 Directory changed\r\n"));
}


// SYST command
void ftpc::handle_syst()
{
    bufferevent_write(this->evbuff, "215 UNIX Type: L8\r\n", strlen("215 UNIX Type: L8\r\n")); 
}


// Find a port for pasv, just try to bind at random til we hit one. 
unsigned short ftpc::find_pasv_port()
{
    unsigned short retport = 0;

    srand(time(0));

    while(true)
    {
        retport = (rand() % (35565 - 1025) + 1025);

        int sock = 0;
        struct sockaddr_in s_addr;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if(!sock)
            return 0;

        memset(&s_addr, 0, sizeof(struct sockaddr_in));
        s_addr.sin_addr.s_addr  = htonl(0);
        s_addr.sin_family       = AF_INET;
        s_addr.sin_port         = htons(retport);

        if(bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr)) != -1)
        {
            close(sock);

            break;
        }

        close(sock);
    }

    return retport;
}
