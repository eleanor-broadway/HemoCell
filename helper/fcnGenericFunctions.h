#ifndef FCN_GENERIC_FUNCTIONS_H
#define FCN_GENERIC_FUNCTIONS_H

#include <sys/stat.h>
#include <vector>
#include <string>

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
typedef struct stat Stat;


using namespace std;

/* Checks if a file exists */
// http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
inline bool file_exists (const std::string& name);

int renameFileToDotOld(std::string fName);



/*
@(#)File:           $RCSfile: mkpath.c,v $
@(#)Version:        $Revision: 1.13 $
@(#)Last changed:   $Date: 2012/07/15 00:40:37 $
@(#)Purpose:        Create all directories in path
@(#)Author:         J Leffler
@(#)Copyright:      (C) JLSS 1990-91,1997-98,2001,2005,2008,2012
*/
// http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux

static int do_mkdir(const char *path, mode_t mode);

/**
** mkpath - ensure all directories in path exist
** Algorithm takes the pessimistic view and works top-down to ensure
** each directory in path exists, rather than optimistically creating
** the last element and working backwards.
*/
// mkpath(argv[i], 0777);
int mkpath(const char *path, mode_t mode);



#include "fcnGenericFunctions.hh"
#endif // FCN_GENERIC_FUNCTIONS_HH