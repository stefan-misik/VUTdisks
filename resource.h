#ifndef _RESOURCE_H
#define _RESOURCE_H

#include "common.h"

#define IDC_STATIC -1

#define IDB_LOGO                        102
#define IDI_MAIN                        103
#define IDI_WARN                        104
#define IDI_LOGO_IMAGE                  105
#define IDI_REVEAL                      106

#define IDR_MENU                        40000
    #define ID_REMOVEREGISTRYDATA           40001
    #define ID_DISKMAPPER_RUNATSTARTUP      40002
    #define ID_RUNATSTARTUP                 40003
    #define ID_VISUALMANIFEST               40004
    #define ID_EXIT                         40005
    #define ID_MENU_ABOUT                   40100

#define IDD_MAIN_WND                    1000
    #define IDC_LOGIN                       1001
    #define IDC_PASSWD                      1002
    #define IDC_PROGRESS                    1003
    #define IDC_CPRGHT                      1004
    #define IDC_ID                          1005
    #define IDC_SHOWP                       1006
    #define IDC_LOGO                        1007
    #define IDC_SAVE_PASS                   1009
    #define IDC_ERR_LIST                    1010
    #define IDC_WARNS                       1011

#define IDD_ABOUT       1030
    #define IDC_APP_ICON                    1031
    #define IDC_APP_NAME                    1032
    #define IDC_APP_VER                     1033
    #define IDC_APP_GIT                     1034
    #define IDC_APP_AUTHOR                  1035
    #define IDC_LICTEXT                     1036
    #define IDC_GET_LATEST                  1037
    #define IDC_MAILTO                      1038

#endif /* _RESOURCE_H */

