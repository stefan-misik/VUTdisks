#ifndef _REVEAL_BUTTON_H
#define	_REVEAL_BUTTON_H


#include "common.h"

#define REVEAL_BUTTON_CLASS "REVEAL_BUTTON_CLASS";

/* Events sent as WM_COMMAND to parent window */
#define RB_REVEAL   0
#define RB_HIDE     1


ATOM RevButtonRegisterClass(
    VOID
);

#endif	/* _REVEAL_BUTTON_H */

