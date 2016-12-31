#include <cstdlib>
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

using std::cout;
using std::endl;

int main(int argc, char** argv) {
	if(argc != 2) {
		return EXIT_FAILURE;
	}

	DIR *dp = opendir(argv[1]);
	if(dp != NULL) {
		struct dirent *ep = NULL;
		while(ep = readdir(dp)) {
			cout << ep->d_name << endl;

			struct stat s;
			if(stat(ep->d_name, &s) == 0) {
				time_t& t = s.st_atime;

				cout << t << "\t" << ep->d_name << endl;
			} else {
				cout << "woomp whomp" << endl;
			}

		}
		closedir(dp);
	} else {

	}

	return EXIT_SUCCESS;
}
