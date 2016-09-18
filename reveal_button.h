#ifndef _REVEAL_BUTTON_H
#define	_REVEAL_BUTTON_H


#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#define REVEAL_BUTTON_CLASS "REVEAL_BUTTON_CLASS";
    
/* Events sent as WM_COMMAND to parent window */
#define RB_REVEAL   0
#define RB_HIDE     1


ATOM RevButtonInit(
    VOID
);

VOID RevButtonDeinit(
    VOID
);


#ifdef __cplusplus
}
#endif

#endif	/* _REVEAL_BUTTON_H */

