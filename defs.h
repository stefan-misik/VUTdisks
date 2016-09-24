#ifndef DEFS_H
#define	DEFS_H

/* Project version*/
#define PROJECT_VER_1           1
#define PROJECT_VER_2           0
#define PROJECT_VER_3           0
#ifdef PROJECT_COMMITS
    #define PROJECT_VER_4       PROJECT_COMMITS
#else
    #define PROJECT_VER_4       0
#endif

#ifndef PROJECT_VER
    #define PROJECT_VER         ""
#endif
#define PROJECT_AUTHOR          "Stefan Misik"
#define PROJECT_DESC            "VUTdisks Mapper"
#define PROJECT_CPYR            "Copyright (C) 2016 " PROJECT_AUTHOR

#define PROJECT_WEB "https://github.com/stefan-misik/VUTdisks/releases/latest"



#endif	/* DEFS_H */