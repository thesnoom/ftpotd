#include <string>
#include <string.h>

#include "sockets.hpp"
#include "db.hpp"


using namespace std;


int main(int argc, char **argv)
{	
    string root_folders[] = { "www", "backup", "keys" };
    string date_strings[] = { "Feb 26  2017", "Mar 17  2017" };

    cvirtfs virtual_fs("rwxr-xr-x", nullptr, date_strings[0], "/srv/", "root", "root");

    virtual_fs.make_folder("rwxr-xr-x", root_folders[0], date_strings[0]);

    virtual_fs.make_folder("r--r-----", root_folders[2], date_strings[1], nullptr, "root", "root");
    virtual_fs.make_folder("rwxr-----", root_folders[1], date_strings[0], nullptr, "root", "root");

    vector<cvirtfs *> ptr_root_folders = virtual_fs.get_all_folders();
    
    int j = 0;

    for(auto &i : ptr_root_folders)
    {
        switch(j)
        {
            // www
            case 0:
            {
                // www subfolders
                string www_folders[] = { "db", "assets", "e1f60072ab7cfaed6581de7e7808fd9b"};
                
                // www files
                string www_files[] = { "index.php", "about.php", "store.php", "checkout.php", "login.php", "profile.php", "admin.php", ".htaccess" };

                // folder creation
                i->make_folder("rwxr-xr-x", www_folders[0], date_strings[0]);
                i->make_folder("rwxr-xr--", www_folders[1], date_strings[0]);
                i->make_folder("rwxr-x---", www_folders[2], date_strings[0]);

                // file creation
                i->make_file("rwxr-x---", www_files[0], 3285, date_strings[0]);
                i->make_file("rwxr-x---", www_files[1], 1801, date_strings[0]);
                i->make_file("rwxr-x---", www_files[2], 5966, date_strings[0]);
                i->make_file("rwxr-x---", www_files[3], 2092, date_strings[0]);
                i->make_file("rwxr-x---", www_files[4], 500, date_strings[0]);
                i->make_file("rwxr-x---", www_files[5], 4099, date_strings[0]);
                i->make_file("rwxr-x---", www_files[6], 7022, date_strings[0]);
                i->make_file("rwxr-x---", www_files[7], 59, date_strings[0]);

                // Same again to do sub folder files..
                
                vector<cvirtfs *> ptr_www_folders = i->get_all_folders();
                int k = 0;

                for(auto &w : ptr_www_folders)
                {
                    switch(k)
                    {
                        case 0: // db
                        {
                            string db_files[] = { "db_con.php", "db_backup.php", "db_creds.php" };

                            w->make_file("rwxr-x---", db_files[0], 800, date_strings[0]);
                            w->make_file("rwxr-x---", db_files[1], 6300, date_strings[0]);
                            w->make_file("rwxr-x---", db_files[2], 102, date_strings[0]);

                            break;
                        }

                        case 1: // assets
                        {
                            string asset_files[] = { "css.css", "store.css", "admin.css", "global.js" }; 

                            w->make_file("rwxr-x---", asset_files[0], 200, date_strings[0]);
                            w->make_file("rwxr-x---", asset_files[1], 230, date_strings[0]);
                            w->make_file("rwxr-x---", asset_files[2], 230, date_strings[0]);
                            w->make_file("rwxr-x---", asset_files[3], 9025, date_strings[0]);

                            break;
                        }
                    }

                    k++;
                }

                break;
            }

            // keys
            case 1:
            {
                i->make_file("r--r-----", "gpgkey",     7326, date_strings[1], "root", "root");
                i->make_file("r--r-----", "gpgkey.pub", 5006, date_strings[1], "root", "root");
                i->make_file("r--r-----", "sshkey",     3250, date_strings[1], "root", "root");
                i->make_file("r--r-----", "sshkey.pub",  757, date_strings[1], "root", "root");
                break;
            }

            /// backup
            case 2:
            {
                i->make_file("r--r-----", "schema_sql.bak", 5322,  date_strings[1], "root", "root");
                i->make_file("r--r-----", "latest_sql.bak", 60204, "Aug 22  2017" , "root", "root");
                break;
            }
        }

        j++;
    }

    asyncsock *asock = new asyncsock(new cdb("127.0.0.1", "user", "pass", "db"), &virtual_fs, 21);
    asock->start_socket();

    delete asock;

    return 0;
}
