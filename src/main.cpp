#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

using std::cout;
using std::endl;
using std::string;
using std::ostringstream;

int main(int argc, char** argv) {
	if(argc != 2) {
		return EXIT_FAILURE;
	}

	const char* dirPath = argv[1];
	const size_t dirPathLen = strlen(argv[1]);
	if(dirPathLen <= 1) {
		return EXIT_FAILURE;
	}

	// Query the directory
	DIR* dp = opendir(dirPath);
	if(dp == NULL) {
		return EXIT_FAILURE;
	}

	// For each entry in the directory move it into it's year-month 
	// subdirectory.
	struct dirent *ep = NULL;
	while( (ep = readdir(dp)) ) {

		// Put together full source file path
		ostringstream ss;
		ss << dirPath << ep->d_name;
		string str = ss.str();
		const char* filePath = str.c_str();	

		struct stat s;
		if(stat(filePath, &s) == 0) {
			// Make sure we're not looking at a directory
			if(S_ISREG(s.st_mode)) {
				// Seconds since linux epoch
				time_t& t = s.st_mtime;

				// Year, month, day 
				struct tm* m = localtime(&t);

				// Put together the destination directory
				ostringstream ss;
				ss << dirPath << (m->tm_year + 1900) << "/";
				if( (m->tm_mon + 1) < 10 ) {
					ss << "0";
				}
				ss << (m->tm_mon + 1) << "/";

				string newPathStr = ss.str();
				const char* newPath = newPathStr.c_str();

				// Make that directory if it's missing
				struct stat se;
				if( stat( newPath, &se ) == -1 ) {
					mkdir(newPath, 0700);
				}

				// Put together full destination file path
				ss << ep->d_name;
				string newFilePathStr = ss.str();
				const char* newFilePath = newFilePathStr.c_str();

				// Move the file.
				if( rename(filePath, newFilePath) == 0 ) {
					cout << newFilePath << endl;
				} else {
					perror(newFilePath);
				}
			}
		} else {
			perror(filePath);
		}

	}

	closedir(dp);

	return EXIT_SUCCESS;
}
