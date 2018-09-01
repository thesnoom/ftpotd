#include "db.hpp"


using namespace std;


// Database constructor
// 1. Get driver instance
// 2. Connect
// 3. Change database schema
// 4. Initialise prepared statements
cdb::cdb(string host, string user, string pass, string db)
{
    try 
    {
        this->sql_drv = get_driver_instance();
        if(!this->sql_drv)
        {
            // Error
        }

        this->sql_con = this->sql_drv->connect(host, user, pass);
        if(!this->sql_con->isValid())
        {
            // Error
        }

		this->db = db;
        this->sql_con->setSchema(db);

        this->con_stmt  = this->sql_con->prepareStatement("INSERT INTO connections(ip_string, conid_string, close, epoch) VALUES(?, ?, ?, ?)");
        this->log_stmt  = this->sql_con->prepareStatement("INSERT INTO activity(conid_string, data, vfs, epoch) VALUES (?, ?, ?, ?)");
        this->ath_log   = this->sql_con->prepareStatement("INSERT INTO authed(conid_string, user, pass, epoch) VALUES (?, ?, ?, ?)");
        this->att_log   = this->sql_con->prepareStatement("INSERT INTO attempts(conid_string, user, pass, epoch) VALUES (?, ?, ?, ?)");
        this->ath_stmt  = this->sql_con->prepareStatement("SELECT EXISTS (SELECT * FROM logins WHERE BINARY user = ? AND BINARY pass = ?)");

    } catch(sql::SQLException &e) {
        cout << string("Error: ") + string(e.what()) << endl;
		exit(0);
    }
}


// Database destructor
// Delete prepared statements
// Delete connection
cdb::~cdb()
{
    if(this->con_stmt)
        delete this->con_stmt;

    if(this->log_stmt)
        delete this->log_stmt;

    if(this->ath_stmt)
        delete this->ath_stmt;

    if(this->att_log)
        delete this->att_log;

    if(this->sql_con)
        delete this->sql_con;
}


void cdb::reinit()
{
	if(this->sql_con->isValid())
		return;

    if(this->con_stmt)
		delete this->con_stmt;

    if(this->log_stmt)
		delete this->log_stmt;

    if(this->ath_stmt)
		delete this->ath_stmt;

    if(this->att_log)
		delete this->att_log;

	if(!this->sql_con->reconnect())
	{
		cout << "Error reconnecting to database" << endl;
		exit(-1);
	}

	this->sql_con->setSchema(this->db);

	this->con_stmt  = this->sql_con->prepareStatement("INSERT INTO connections(ip_string, conid_string, close, epoch) VALUES(?, ?, ?, ?)");
	this->log_stmt  = this->sql_con->prepareStatement("INSERT INTO activity(conid_string, data, vfs, epoch) VALUES (?, ?, ?, ?)");
	this->ath_log   = this->sql_con->prepareStatement("INSERT INTO authed(conid_string, user, pass, epoch) VALUES (?, ?, ?, ?)");
	this->att_log   = this->sql_con->prepareStatement("INSERT INTO attempts(conid_string, user, pass, epoch) VALUES (?, ?, ?, ?)");
	this->ath_stmt  = this->sql_con->prepareStatement("SELECT EXISTS (SELECT * FROM logins WHERE BINARY user = ? AND BINARY pass = ?)");
}


// Function to log a connection
// Set prepared statement variables
// Execute statement
void cdb::log_conn(string ip, string conid, bool close)
{
    try
    {
		this->reinit();
        
        this->con_stmt->setString (1, ip);
        this->con_stmt->setString (2, conid);
        this->con_stmt->setInt    (3, close);
        this->con_stmt->setInt    (4, time(0));

        this->con_stmt->execute();
    } catch(sql::SQLException &e) {
        cout << string("Error: ") + string(e.what()) << endl;
    }
}


// Function to log general activity
// Set prepared statement variables
// Execute statement
void cdb::log_activity(string conid, string data, string vfs, int epoch)
{
    try
    {
		this->reinit();
        
        this->log_stmt->setString   (1, conid);
        this->log_stmt->setString   (2, data);
        this->log_stmt->setString   (3, vfs);
        this->log_stmt->setInt      (4, epoch);

        this->log_stmt->execute();
    } catch(sql::SQLException &e) {
        cout << string("Error: ") + string(e.what()) << endl;
    }
}


//
void cdb::log_auth(string conid, string user, string pass, int epoch)
{
    try
    {
		this->reinit();

        this->ath_log->setString    (1, conid);
        this->ath_log->setString    (2, user);
        this->ath_log->setString    (3, pass);
        this->ath_log->setInt       (4, epoch);

        this->ath_log->execute();
    } catch(sql::SQLException &e) {
        cout << string("Error: ") + string(e.what()) << endl;
    }
}


//
void cdb::log_att(string conid, string user, string pass, int epoch)
{
    try
    {
		this->reinit();

        this->att_log->setString    (1, conid);
        this->att_log->setString    (2, user);
        this->att_log->setString    (3, pass);
        this->att_log->setInt       (4, epoch);

        this->att_log->execute();
    } catch(sql::SQLException &e) {
        cout << string("Error: ") + string(e.what()) << endl;
    }
}


// Function to determine valid authentication
// Set prepared statement variables
// Execute statement
// Test response, valid login or not
bool cdb::ftp_auth(string user, string pass)
{
    bool bret = false;

    if(user.empty() || pass.empty())
        return bret;

    this->reinit();
    
    sql::ResultSet *res;

    this->ath_stmt->setString(1, user);
    this->ath_stmt->setString(2, pass);

    res = this->ath_stmt->executeQuery();
    res->next();
    
    res->getInt(1) ? bret = true : bret = false;

    delete res;

    return bret;
}
