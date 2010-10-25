#pragma once

#include <include/winsta.h>

INT  INTERNAL_CALL MouseSafetyOnDrawStart(PPDEVOBJ ppdev, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2);
INT  INTERNAL_CALL MouseSafetyOnDrawEnd(PPDEVOBJ ppdev);

#ifndef XBUTTON1
#define XBUTTON1	(0x01)
#endif
#ifndef XBUTTON2
#define XBUTTON2	(0x02)
#endif
#ifndef MOUSEEVENTF_XDOWN
#define MOUSEEVENTF_XDOWN	(0x80)
#endif
#ifndef MOUSEEVENTF_XUP
#define MOUSEEVENTF_XUP	(0x100)
#endif
