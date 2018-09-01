/*
 * File: virtfs.hpp
 */

#ifndef __VIRT_FS_H
#define __VIRT_FS_H

#include <iostream>
#include <vector>
#include <memory>
#include <string.h>


using namespace std;


#define VFS_M   0x0007
#define VFS_R   0x0004
#define VFS_W   0x0002
#define VFS_X   0x0001


enum perms {
    VFS_OWNR,
    VFS_GRP,
    VFS_ALL
};

enum findparams {
    FIND_FILE,
	FIND_FOLDER,
	FIND_BOTH
};

struct svirtfd {
    string	filename;
    string  fileowner;
    string  filegroup;
    int     filebytes;
    string  filedate;
    string  fileperm;
    void	*parent;
};


class cvirtfs
{
	public:
		cvirtfs(string perms, cvirtfs *vfs_parent = nullptr, string date = "", string name = "", string owner = "apache", string group = "apache");
		
		bool	make_folder(string perms, string name, string date, cvirtfs **out = nullptr, string owner = "apache", string group = "apache");
		bool	remove_folder();
		bool	rename_folder(string name, cvirtfs *vfs_folder = nullptr);		
		string	get_folder_name();
		cvirtfs *find_folder(string name);
		bool	folder_exists(string name);
		
		bool	make_file(string perms, string filename, int filebytes = 4096, string date = "", string owner = "apache", string group = "apache", svirtfd **out = nullptr);
		bool	remove_file(string name);
		bool	rename_file(string name, string newnamee);
		svirtfd *find_file(string name);
		bool	file_exists(string name);

	    vector<cvirtfs *> get_all_folders();
		vector<svirtfd *> get_all_files();

		cvirtfs *get_parent();
		bool	is_root_dir();

		void	get_root(cvirtfs **out);
		void	get_full_path(void *item, bool bfolder, string *full_path_out);

		void 	printtree(cvirtfs *folder, int depth = 0, bool print_files = true);

        string get_folder_perm();
        string get_folder_owner();
        string get_folder_group();
        string get_folder_date();

		//typedef void (*find_callback)(vector<cvirtfs *> fnd_folders, vector<svirtfd *> fnd_files);
		//void	find(cvirtfs *folder, string match, findparams params, find_callback cbfunc);

	private:
		vector<cvirtfs *>   vfs_folders;
		vector<svirtfd *>   vfs_files;
        cvirtfs             *vfs_parent;

		string				vfs_name;
        string              vfs_owner;
        string              vfs_group;
        string              vfs_date;
        string              vfs_perms;
};

#endif
