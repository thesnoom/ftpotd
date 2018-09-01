#ifndef __CDB_H
#define __CDB_H

#include <string>
#include <unistd.h>
#include <time.h>

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>


using namespace std;


class cdb
{
    public:
        cdb(string host, string user, string pass, string db);
        ~cdb();

	void reinit();

        void log_conn(string ip, string conid, bool close);
        void log_activity(string conid, string data, string vfs, int epoch);
        void log_auth(string conid, string user, string pass, int epoch);
        void log_att(string conid, string user, string pass, int epoch);
        bool ftp_auth(string user, string pass);

    private:
        sql::Driver             *sql_drv;
        sql::Connection         *sql_con;

        sql::PreparedStatement  *con_stmt;
        sql::PreparedStatement  *log_stmt;
        sql::PreparedStatement  *att_log;
        sql::PreparedStatement  *ath_log;
        sql::PreparedStatement  *ath_stmt;

	string db;
};

#endif
