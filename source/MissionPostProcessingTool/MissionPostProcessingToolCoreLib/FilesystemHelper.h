#pragma once
#ifndef __FILESYSTEM_HELPER_H__
#include<string>

namespace FileSystemHelper {

	// Return true only if file or directory name without any redirection ..\, .\, A\B etc
	//I.e. this string specifies a single sub directory or a file without folder qualification
	bool NoDirMutation(const std::string& a); //Return !has_parent_path(path)??
}

#endif