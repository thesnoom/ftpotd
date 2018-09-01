#include "virtfs.hpp"


using namespace std;

// Virtual FS class constructor
// Takes in
// Permissions string
// Pointer to parent folder
// Date string,
// Folder name,
// Owner
// Group
cvirtfs::cvirtfs(string perms, cvirtfs *vfs_parent, string date, string name, string owner, string group)
{
	if(name.empty())
		name = string("/");

	this->vfs_name      = name;
    this->vfs_owner     = owner;
    this->vfs_group     = group;
    this->vfs_parent    = vfs_parent;
    this->vfs_date      = date; 
    this->vfs_perms     = perms;

    this->vfs_folders.reserve(10);
    this->vfs_files.reserve(10);
}

// Wrapper to make a call to cvirtfs and apply parent folder.  
// Only if folder doesn't exist.
bool cvirtfs::make_folder(string perms, string name, string date, cvirtfs **out, string owner, string group)
{
	if(!this->folder_exists(name)) 
	{
        cvirtfs *newfolder = new cvirtfs(perms, this, date, name, owner, group);

		this->vfs_folders.push_back(newfolder);

		if(out)
			*out = this->vfs_folders.back();

	} else 
		return false;

	return true;
}


// Recursively remove folders and files.
// Goes top down
bool cvirtfs::remove_folder()
{
	for(auto i = this->vfs_files.begin(); i != this->vfs_files.end(); i++)
		delete (*i);

	for(auto i = this->vfs_folders.begin(); i != this->vfs_folders.end(); i++)
	    (*i)->remove_folder();

	return true;
}


// Rename a folder if it doesn't already exist. 
bool cvirtfs::rename_folder(string name, cvirtfs *folder)
{
    if(!name.empty() && folder)
       return folder->rename_folder(name); 

	if(!name.empty() && !this->folder_exists(name))
		this->vfs_name = name;
	else 
		return false;

	return true;
}


// Return folder string
string cvirtfs::get_folder_name()
{
	return this->vfs_name;
}


// Find a folder, not recursive 
cvirtfs *cvirtfs::find_folder(string name)
{
	cvirtfs *ret = nullptr;

	if(name.empty())
		return ret;

	for(auto &i : this->vfs_folders)
	{
		if(!i->vfs_name.compare(name))
		{
			ret = i;
			break;
		}
	}

	return ret;
}


// Does a folder exist
// I guess it's pointless as we can use the function find_folder...
bool cvirtfs::folder_exists(string name)
{
	bool ret = false;

	if(name.empty())
		return ret;

	for(auto &i : this->vfs_folders)
	{
		if(!i->vfs_name.compare(name))
		{
			ret = true;
			break;
		}
	}

	return ret;
}


// Make a new file and add it to the vector of the parent folder.
bool cvirtfs::make_file(string perms, string filename, int filebytes, string date, string owner, string group, svirtfd **out)
{
	if(!filename.empty() && !this->file_exists(filename)) 
	{
		svirtfd *newfile = new svirtfd;

		newfile->filename   = filename;
        newfile->fileowner  = owner;
        newfile->filegroup  = group;
        newfile->filebytes  = filebytes;
        newfile->filedate   = date; 
		newfile->parent     = (void *)this;
        newfile->fileperm   = perms;

		this->vfs_files.push_back(newfile);

		if(out)
			*out = this->vfs_files.back();
	} else 
		return false;

	return true;
}


// Delete a file. 
bool cvirtfs::remove_file(string filename)
{
	for(auto i = this->vfs_files.begin(); i != this->vfs_files.end();)
	{
		if(!(*i)->filename.compare(filename))
		{
			svirtfd *erase = *i;
			i = this->vfs_files.erase(i);
			delete erase;

			return true;
		} else
			i++;
	}

	return false;
}


// Rename a file 
bool cvirtfs::rename_file(string filename, string new_filename)
{
	bool ret = false;

	if(new_filename.empty() || this->file_exists(new_filename))
		return ret;

	svirtfd *find = nullptr;
	if((find = this->find_file(filename)))
	{
		find->filename = filename;

		ret = true;
	}

	return ret;
}


// Find a file, not recursive.
svirtfd *cvirtfs::find_file(string filename)
{
	svirtfd *ret = nullptr;

	for(auto &i : this->vfs_files)
	{
		if(!i->filename.compare(filename))
		{
			ret = i;
			break;
		}
	}

	return ret;
}


// Again kinda pointless because of find_file...
bool cvirtfs::file_exists(string filename)
{
	bool ret = false;

	for(auto &i : this->vfs_files)
	{
		if(!i->filename.compare(filename))
		{
			ret = true;
			break;
		}
	}

	return ret;
}


// Return the vector of folders. 
vector<cvirtfs *> cvirtfs::get_all_folders()
{
	return this->vfs_folders;
}


// Return the vector of files. 
vector<svirtfd *> cvirtfs::get_all_files()
{
	return this->vfs_files;
}


// Return parent
cvirtfs *cvirtfs::get_parent()
{
	return this->vfs_parent;
}


// Is this the root? Is parent null?
bool cvirtfs::is_root_dir()
{
	return (this->vfs_parent ? false : true);
}


// Recursive until we hit root dir. 
void cvirtfs::get_root(cvirtfs **out)
{
	if(!this->is_root_dir())
		this->get_parent()->get_root(out);
	else
		*out = this;
}


// Recursive addition to path until root.
void cvirtfs::get_full_path(void *item, bool bfolder, string *full_path_out)
{	
	if(!full_path_out || !item)
		return;
	
	cvirtfs *folder = nullptr;
	
	if(bfolder)
		folder = (cvirtfs *)item;
	else {
		folder = (cvirtfs *)(((svirtfd *)item)->parent);
		bfolder = !bfolder;
	}
		
    bool broot = folder->is_root_dir();
    string fol = folder->get_folder_name();

    if(broot) 
	    *full_path_out = (fol + *full_path_out);
    else
	    *full_path_out = (fol + "/" + *full_path_out);

    if(broot)
		return;

	this->get_full_path((void *)folder->get_parent(), bfolder, full_path_out);
}


// Return folder date
string cvirtfs::get_folder_date()
{
    return this->vfs_date;
}


// Return folder permissions
string cvirtfs::get_folder_perm()
{
    return this->vfs_perms;
}


// Return folder owner
string cvirtfs::get_folder_owner()
{
    return this->vfs_owner;
}


// Return folder group
string cvirtfs::get_folder_group()
{
    return this->vfs_group;
}

// Debug function to print the tree, with depth. 
void cvirtfs::printtree(cvirtfs *folder, int depth, bool print_files)
{
	if(folder->is_root_dir())
	{
		cout << "[d] - " << folder->get_folder_name() << endl;
		depth++;
	}

	for(auto &k : folder->get_all_files())
	{
		for(int j = 0; j < depth; j++)
			cout << "\t";

		cout << "[f] - " << k->filename << endl;
	}	


	for(auto &i : folder->get_all_folders())
	{
		for(int j = 0; j < depth; j++)
			cout << "\t";

		cout << "[d] - " << i->get_folder_name() << endl;

		this->printtree(i, (depth + 1));
	}
}


/*void cvirtfs::find(cvirtfs *folder, string match, findparams params, find_callback cbfunc)
{
	if(!folder || !cbfunc || match.empty())
		return;
	
	static vector<cvirtfs *> fnd_folders;
	static vector<svirtfd *> fnd_files;
	
	if(params == FIND_BOTH || params == FIND_FILE)
		for(auto &i : folder->get_all_files())
		{
			if(i->filename.find(match) != string::npos)
				fnd_files.push_back(i);
		}

	if(params == FIND_BOTH || params == FIND_FOLDER)
		for(auto &i : folder->get_all_folders())
		{	
			if(i->get_folder_name().find(match) != string::npos)
				fnd_folders.push_back(i);
		}
	
	cbfunc(fnd_folders, fnd_files);
	
	fnd_folders.clear();
	fnd_files.clear();
	
	for(auto &i : folder->get_all_folders())
		this->find(i, match, params, cbfunc);
}

void find_callbackfunc(vector<cvirtfs *> fnd_folders, vector<cvirtfs::svirtfd *> fnd_files)
{	
	string full_path;

	for(auto &i : fnd_folders)
	{
		full_path.clear();
		i->get_full_path(i, true, &full_path);
		cout << "[x] Found folder: " << full_path << endl;
	}
	
	for(auto &i : fnd_files)
	{
		full_path.clear();
		((cvirtfs *)((cvirtfs::svirtfd *)i)->parent)->get_full_path(i, false, &full_path);
		cout << "[x] Found file: " << (full_path + i->filename + i-> file_ext) << endl;
	}
}
*/
