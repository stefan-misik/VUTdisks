#ifndef DEFS_H
#define	DEFS_H

#include "common.h"

/* Project version*/
#define PROJECT_VER_1           1
#define PROJECT_VER_2           0
#define PROJECT_VER_3           1
#ifdef PROJECT_COMMITS
    #define PROJECT_VER_4       PROJECT_COMMITS
#else
    #define PROJECT_VER_4       0
#endif
#ifndef PROJECT_LAST_COMMIT
    #define PROJECT_LAST_COMMIT 0
#endif

#ifndef PROJECT_LAST_RELEASE
    #define PROJECT_LAST_RELEASE         ""
#endif

#ifndef PROJECT_GIT
    #define PROJECT_GIT         ""
#endif

#define PROJECT_VER             "v" STRINGIFY(PROJECT_VER_1) "."     \
                                    STRINGIFY(PROJECT_VER_2) "."     \
                                    STRINGIFY(PROJECT_VER_3) "."     \
                                    STRINGIFY(PROJECT_VER_4)
#define PROJECT_NAME            "VUTdisks"
#define PROJECT_AUTHOR          "Stefan Misik"
#define PROJECT_DESC            "VUTdisks Mapper"
#define PROJECT_CPYR            "Copyright (C) 2016 " PROJECT_AUTHOR

#define PROJECT_WEB "https://github.com/stefan-misik/VUTdisks/releases/latest"

#define PROJECT_MAIL "stefan.misik@phd.feec.vutbr.cz"


extern const TCHAR lpProjectName[];
extern const TCHAR lpProjectAuthor[];
extern const TCHAR lpProjectVer[];
extern const TCHAR lpProjectLastRelease[];
extern const TCHAR lpProjectGit[];
extern const TCHAR lpProjectWeb[];
extern const TCHAR lpProjectMail[];
extern const TCHAR lpProjectLic[];



#endif	/* DEFS_H */