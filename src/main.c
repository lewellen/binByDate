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

struct Path {
	char str[PATH_MAX];
};

bool initPath(struct Path* dest, const char* src) {
	assert(dest != NULL);
	assert(src != NULL);

	size_t srcLen = strlen(src);
	if(srcLen >= sizeof(dest->str)) {
		return false;
	}

	memset(dest->str, '\0', sizeof(dest->str));
	memcpy(dest->str, src, srcLen);
	return true;
}

void copyPath(struct Path* dest, const struct Path* src) {
	assert(dest != NULL);
	assert(src != NULL);

	memset(dest->str, '\0', sizeof(dest->str));
	memcpy(dest->str, src->str, sizeof(src->str));
}

bool pathExists(const struct Path* path) {
	assert(path != NULL);

	struct stat s;
	return stat(path->str, &s) == 0;
}

bool makeDirIfNecessary(const struct Path* path) {
	assert(path != NULL);
	
	struct stat s;
	int statResult = stat(path->str, &s);
	if(statResult == 0) {
		return true;
	}

	if(errno != ENOENT) {
		perror(path->str);
		return false;
	}

	int mkdirResult = mkdir(path->str, 0700);
	if(mkdirResult == 0) {
		return true;
	}

	perror(path->str);
	return false;
}

bool addForwardSlashIfNecessary(struct Path* path) {
	assert(path != NULL);

	size_t len = strlen(path->str);
	if(len + 1 >= sizeof(path->str)) {
		return false;
	}

	if(path->str[len - 1] == '/') {
		return true;
	}

	path->str[len] = '/';
	path->str[len + 1] = '\0';
	return true;
}

bool concatPath(struct Path* base, const char* addendum) {
	assert(base != NULL);
	assert(addendum != NULL);

	addForwardSlashIfNecessary(base);

	size_t baseLen = strlen(base->str);
	size_t addendumLen = strlen(addendum);
	if(baseLen + addendumLen + 1 >= sizeof(base->str)) {
		return false;
	}

	memcpy(&(base->str[baseLen]), addendum, addendumLen);
	base->str[baseLen + addendumLen] = '\0';
	return true;
}

bool formatInt(char* dest, size_t destLen, int leadingZeros, int value) {
	assert(dest != NULL);
	if( (leadingZeros < 0) || (leadingZeros >= destLen) ) {
		return false;
	}

	if(value < 0) {
		return false;
	}	

	char format[5];
	if(snprintf(format, sizeof(format), "%%0%dd", leadingZeros) < 0) {
		perror("Failed to format format string.\n");
		return false;
	}

	memset(dest, '\0', destLen);
	if(snprintf(dest, destLen, format, value) < 0) {
		perror("Failed to format string.\n");
		return false;
	}

	return true;
}

void processFilePath(const struct Path* baseDir, const char* fileName) {
	struct Path srcPath;
	copyPath(&srcPath, baseDir);
	concatPath(&srcPath, fileName);

	struct stat s;
	if(stat(srcPath.str, &s) != 0)  {
		perror(srcPath.str);
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

	// Put together the dest directory
	struct Path destPath;
	copyPath(&destPath, baseDir);

	char year[5] = { '\0' };
	if(
		!formatInt(year, sizeof(year), 4, m->tm_year + 1900) ||
		!concatPath(&destPath, year)
	) {
		perror(destPath.str);
		return;
	}

	if(!makeDirIfNecessary(&destPath)) {
		fprintf(stderr, "Failed to create path: '%s'.\n", destPath.str);
	} else {
		char month[3] = { '\0' };
		if(
			!formatInt(month, sizeof(month), 2, m->tm_mon + 1) ||
			!concatPath(&destPath, month)
		) {
			perror(destPath.str);
			return;
		}
	
		if(!makeDirIfNecessary(&destPath)) {
			fprintf(stderr, "Failed to create path: '%s'.\n", destPath.str);
		} else {
			// Put together full dest file path
			if(!concatPath(&destPath, fileName)) {
				perror(destPath.str);
				return;
			}

			// Move the file.
			if( rename(srcPath.str, destPath.str) == 0 ) {
				fprintf(stdout, "%s\n", destPath.str);
			} else {
				perror(destPath.str);
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

	struct Path dirPath;
	if(!initPath(&dirPath, argv[1])) {
		fprintf(stderr, "Copy error.\n");
		return EXIT_FAILURE;
	}

	if(!pathExists(&dirPath)) {
		fprintf(stderr, "Directory '%s' does not exist.\n", 
			dirPath.str);
		return EXIT_FAILURE;
	}

	// Query the directory
	DIR* dp = opendir(dirPath.str);
	if(dp == NULL) {
		return EXIT_FAILURE;
	}

	// For each entry in the directory move it into it's year-month 
	// subdirectory.
	struct dirent *ep = NULL;
	while( (ep = readdir(dp)) ) {
		processFilePath(&dirPath, ep->d_name);
	}

	closedir(dp);

	return EXIT_SUCCESS;
}
