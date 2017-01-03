#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef enum { false, true = !false } bool;

bool copyPath(char* dest, const char* source) {
	assert(dest != NULL);
	assert(source != NULL);

	size_t len = strlen(source);
	if(len >= PATH_MAX) {
		return false;
	}

	memset(dest, '\0', PATH_MAX);
	memcpy(dest, source, len);
	return true;
}

bool directoryExists(const char* path) {
	assert(path != NULL);

	struct stat s;
	int statResult = stat(path, &s);
	return statResult == 0;
}

bool makeDirIfNecessary(const char* path) {
	assert(path != NULL);
	
	struct stat s;
	int statResult = stat(path, &s);
	if(statResult == 0) {
		return true;
	}

	if(errno != ENOENT) {
		perror(path);
		return false;
	}
	
	int mkdirResult = mkdir(path, 0700);
	if(mkdirResult == 0) {
		return true;
	}

	perror(path);
	return false;
}

bool addForwardSlashIfNecessary(char* path) {
	assert(path != NULL);

	size_t len = strlen(path);
	if(len + 1 >= PATH_MAX) {
		return false;
	}

	if(path[len - 1] == '/') {
		return true;
	}

	path[len] = '/';
	path[len + 1] = '\0';
	return true;
}

bool concatPath(char* base, const char* addendum) {
	assert(base != NULL);
	assert(addendum != NULL);

	addForwardSlashIfNecessary(base);

	size_t baseLen = strlen(base);
	size_t addendumLen = strlen(addendum);

	if(baseLen + addendumLen + 1 >= PATH_MAX) {
		return false;
	}

	memcpy(&base[baseLen], addendum, addendumLen);

	base[baseLen + addendumLen] = '\0';

	return true;
}

bool formatInt(char* dest, int leadingZeros, int value) {
	assert(dest != NULL);
	assert((leadingZeros == 2) || (leadingZeros == 4));
	
	char format[5];
	if(sprintf(format, "%%0%dd", leadingZeros) < 0) {
		return false;
	}

	memset(dest, '\0', PATH_MAX);

	if(sprintf(dest, format, value) < 0) {
		return false;
	}

	return true;
}

void processFilePath(const char* baseDir, const char* fileName) {
	char sourceFilePath[PATH_MAX];
	copyPath(sourceFilePath, baseDir);
	concatPath(sourceFilePath, fileName);

	struct stat s;
	if(stat(sourceFilePath, &s) != 0)  {
		perror(sourceFilePath);
		return;
	}

	if(! S_ISREG(s.st_mode)) {
		// filePath does not point to a directory
		return;
	}

	// Get the last modified time in seconds since the linux epoch
	time_t t = s.st_mtime;

	// Year, month, day 
	struct tm* m = localtime(&t);

	// Put together the destination directory

	char newPathWithYear[PATH_MAX];
		
	char newPath[PATH_MAX];
	char year[5] = { '\0' };

	copyPath(newPath, baseDir);
	formatInt(year, 4, m->tm_year + 1900);
	concatPath(newPath, year);

	if(!makeDirIfNecessary(newPath)) {
		fprintf(stderr, "Failed to create path: '%s'.\n", newPath);
	} else {
		char month[3] = { '\0' };
		formatInt(month, 2, m->tm_mon + 1);
		concatPath(newPath, month);
	
		if(!makeDirIfNecessary(newPath)) {
			fprintf(stderr, "Failed to create path: '%s'.\n", newPathWithYear);
		} else {
			// Put together full destination file path
			concatPath(newPath, fileName);

			// Move the file.
			if( rename(sourceFilePath, newPath) == 0 ) {
				fprintf(stdout, "%s\n", newPath);
			} else {
				perror(newPath);
			}
		}

	}

}

int main(int argc, char** argv) {
	if(argc == 0) {
		fprintf(stderr, "program dirPath\n");
		return EXIT_FAILURE;
	}

	if(argc == 1) {
		fprintf(stderr, "%s dirPath\n", argv[0]);
		return EXIT_FAILURE;
	}

	const size_t pathLen = strlen(argv[1]);
	if(pathLen <= 1 || pathLen >= PATH_MAX ) {
		fprintf(stderr, "Invalid path length %zu >= %d.\n", 
			pathLen, PATH_MAX);
		return EXIT_FAILURE;
	}

	char dirPath[PATH_MAX];
	if(!copyPath(dirPath, argv[1])) {
		fprintf(stderr, "Copy error.\n");
		return EXIT_FAILURE;
	}

	if(!directoryExists(dirPath)) {
		fprintf(stderr, "Directory '%s' does not exist.\n", 
			dirPath);
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
		processFilePath(dirPath, ep->d_name);
	}

	closedir(dp);

	return EXIT_SUCCESS;
}
