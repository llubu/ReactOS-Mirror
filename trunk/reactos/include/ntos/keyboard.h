/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/keyboard.h
 * PURPOSE:      Keyboard declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_KEYBOARD_H
#define __INCLUDE_KEYBOARD_H

#include <ntos/types.h>

/* Virtual Key codes */
#define VK_0	(48)
#define VK_1	(49)
#define VK_2	(50)
#define VK_3	(51)
#define VK_4	(52)
#define VK_5	(53)
#define VK_6	(54)
#define VK_7	(55)
#define VK_8	(56)
#define VK_9	(57)
#define VK_A	(65)
#define VK_B	(66)
#define VK_C	(67)
#define VK_D	(68)
#define VK_E	(69)
#define VK_F	(70)
#define VK_G	(71)
#define VK_H	(72)
#define VK_I	(73)
#define VK_J	(74)
#define VK_K	(75)
#define VK_L	(76)
#define VK_M	(77)
#define VK_N	(78)
#define VK_O	(79)
#define VK_P	(80)
#define VK_Q	(81)
#define VK_R	(82)
#define VK_S	(83)
#define VK_T	(84)
#define VK_U	(85)
#define VK_V	(86)
#define VK_W	(87)
#define VK_X	(88)
#define VK_Y	(89)
#define VK_Z	(90)

#ifndef __USE_W32API

/* KEY_EVENT_RECORD structure */
#define CAPSLOCK_ON	(128)
#define ENHANCED_KEY	(256)
#define LEFT_ALT_PRESSED	(2)
#define LEFT_CTRL_PRESSED	(8)
#define NUMLOCK_ON	(32)
#define RIGHT_ALT_PRESSED	(1)
#define RIGHT_CTRL_PRESSED	(4)
#define SCROLLLOCK_ON	(64)
#define SHIFT_PRESSED	(16)

/* MOUSE_EVENT_RECORD structure */
#define FROM_LEFT_1ST_BUTTON_PRESSED	(1)
#define RIGHTMOST_BUTTON_PRESSED	(2)
#define FROM_LEFT_2ND_BUTTON_PRESSED	(4)
#define FROM_LEFT_3RD_BUTTON_PRESSED	(8)
#define FROM_LEFT_4TH_BUTTON_PRESSED	(16)
#define DOUBLE_CLICK	(2)
#define MOUSE_MOVED	(1)

/* INPUT_RECORD structure */
#define KEY_EVENT	(1)
#define MOUSE_EVENT	(2)
#define WINDOW_BUFFER_SIZE_EVENT	(4)
#define MENU_EVENT	(8)
#define FOCUS_EVENT	(16)


#include <pshpack1.h>

typedef struct _KEY_EVENT_RECORD
{
  BOOL bKeyDown;
  WORD wRepeatCount;
  WORD wVirtualKeyCode;
  WORD wVirtualScanCode;
  union
  {
    WCHAR UnicodeChar;
    CHAR  AsciiChar;
  } uChar;
  DWORD dwControlKeyState;
} KEY_EVENT_RECORD;

#include <poppack.h>


typedef struct _MOUSE_EVENT_RECORD
{
  COORD dwMousePosition;
  DWORD dwButtonState;
  DWORD dwControlKeyState;
  DWORD dwEventFlags;
} MOUSE_EVENT_RECORD;

typedef struct _WINDOW_BUFFER_SIZE_RECORD
{
  COORD dwSize;
} WINDOW_BUFFER_SIZE_RECORD;

typedef struct _MENU_EVENT_RECORD
{
  UINT dwCommandId; 
} MENU_EVENT_RECORD, *PMENU_EVENT_RECORD;

typedef struct _FOCUS_EVENT_RECORD
{
  BOOL bSetFocus;
} FOCUS_EVENT_RECORD;

typedef struct _INPUT_RECORD
{
  WORD EventType;
  union
  {
#ifndef __cplus_plus
    /* this will be the wrong size in c++ */
    KEY_EVENT_RECORD KeyEvent;
#endif
    MOUSE_EVENT_RECORD MouseEvent;
    WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
    MENU_EVENT_RECORD MenuEvent;
    FOCUS_EVENT_RECORD FocusEvent;
  } Event;
} INPUT_RECORD, *PINPUT_RECORD;

/* Virtual Key codes */
#define VK_LBUTTON	(1)
#define VK_RBUTTON	(2)
#define VK_CANCEL	(3)
#define VK_MBUTTON	(4)
#define VK_BACK	(8)
#define VK_TAB	(9)
#define VK_CLEAR	(12)
#define VK_RETURN	(13)
#define VK_SHIFT	(16)
#define VK_CONTROL	(17)
#define VK_MENU	(18)
#define VK_PAUSE	(19)
#define VK_CAPITAL	(20)
#define VK_ESCAPE	(27)
#define VK_SPACE	(32)
#define VK_PRIOR	(33)
#define VK_NEXT	(34)
#define VK_END	(35)
#define VK_HOME	(36)
#define VK_LEFT	(37)
#define VK_UP	(38)
#define VK_RIGHT	(39)
#define VK_DOWN	(40)
#define VK_SELECT	(41)
#define VK_PRINT	(42)
#define VK_EXECUTE	(43)
#define VK_SNAPSHOT	(44)
#define VK_INSERT	(45)
#define VK_DELETE	(46)
#define VK_HELP	    (47)
#define VK_LWIN     (91)
#define VK_RWIN     (92)
#define VK_APPS     (93)
#define VK_SLEEP    (95)
#define VK_NUMPAD0	(96)
#define VK_NUMPAD1	(97)
#define VK_NUMPAD2	(98)
#define VK_NUMPAD3	(99)
#define VK_NUMPAD4	(100)
#define VK_NUMPAD5	(101)
#define VK_NUMPAD6	(102)
#define VK_NUMPAD7	(103)
#define VK_NUMPAD8	(104)
#define VK_NUMPAD9	(105)
#define VK_MULTIPLY	(106)
#define VK_ADD	(107)
#define VK_SEPARATOR	(108)
#define VK_SUBTRACT	(109)
#define VK_DECIMAL	(110)
#define VK_DIVIDE	(111)
#define VK_F1	(112)
#define VK_F2	(113)
#define VK_F3	(114)
#define VK_F4	(115)
#define VK_F5	(116)
#define VK_F6	(117)
#define VK_F7	(118)
#define VK_F8	(119)
#define VK_F9	(120)
#define VK_F10	(121)
#define VK_F11	(122)
#define VK_F12	(123)
#define VK_F13	(124)
#define VK_F14	(125)
#define VK_F15	(126)
#define VK_F16	(127)
#define VK_F17	(128)
#define VK_F18	(129)
#define VK_F19	(130)
#define VK_F20	(131)
#define VK_F21	(132)
#define VK_F22	(133)
#define VK_F23	(134)
#define VK_F24	(135)

/* GetAsyncKeyState */
#define VK_NUMLOCK	(144)
#define VK_SCROLL	(145)
#define VK_LSHIFT	(160)
#define VK_LCONTROL	(162)
#define VK_LMENU	(164)
#define VK_RSHIFT	(161)
#define VK_RCONTROL	(163)
#define VK_RMENU	(165)

#define VK_BROWSER_BACK        (166)
#define VK_BROWSER_FORWARD     (167)
#define VK_BROWSER_REFRESH     (168)
#define VK_BROWSER_STOP        (169)
#define VK_BROWSER_SEARCH      (170)
#define VK_BROWSER_FAVORITES   (171)
#define VK_BROWSER_HOME        (172)
#define VK_VOLUME_MUTE         (173)
#define VK_VOLUME_DOWN         (174)
#define VK_VOLUME_UP           (175)
#define VK_MEDIA_NEXT_TRACK    (176)
#define VK_MEDIA_PREV_TRACK    (177)
#define VK_MEDIA_STOP          (178)
#define VK_MEDIA_PLAY_PAUSE    (179)
#define VK_LAUNCH_MAIL         (180)
#define VK_LAUNCH_MEDIA_SELECT (181)
#define VK_LAUNCH_APP1         (182)
#define VK_LAUNCH_APP2         (183)

#define VK_PROCESSKEY          (229)
#define VK_ICO_CLEAR           (230)

/* Fujitsu/OASYS keys */
#define VK_OEM_FJ_JISHO        (146) /* 'Dictionary' key */
#define VK_OEM_FJ_MASSHOU      (147) /* 'Unregister word' key */
#define VK_OEM_FJ_TOUROKU      (148) /* 'Register word' key */
#define VK_OEM_FJ_LOYA         (149) /* 'Left OYAYUBI' key */
#define VK_OEM_FJ_ROYA         (150) /* 'Right OYAYUBI' key */

/* Nokia/Ericsson keys */
#define VK_OEM_RESET           (223)
#define VK_OEM_JUMP            (234)
#define VK_OEM_PA1             (235)
#define VK_OEM_PA2             (236)
#define VK_OEM_PA3             (237)
#define VK_OEM_WSCTRL          (238)
#define VK_OEM_CUSEL           (239)
#define VK_OEM_ATTN            (240)
#define VK_OEM_FINISH          (241)
#define VK_OEM_COPY            (242)
#define VK_OEM_AUTO            (243)
#define VK_OEM_ENLW            (244)
#define VK_OEM_BACKTAB         (245)
#define VK_ATTN                (246)
#define VK_CRSEL               (247)
#define VK_EXSEL               (248)
#define VK_EREOF               (249)
#define VK_PLAY                (250)
#define VK_ZOOM                (251)
#define VK_NONAME              (252)
#define VK_PA1                 (253)
#define VK_OEM_CLEAR           (254)

#define VK_OEM_NEC_EQUAL       (146) /* = key on numpad */

#define VK_OEM_1               (186) /* ';:' for US */
#define VK_OEM_PLUS            (187) /* '+' any country */
#define VK_OEM_COMMA           (188) /* ',' any country */
#define VK_OEM_MINUS           (189) /* '-' any country */
#define VK_OEM_PERIOD          (190) /* '.' any country */
#define VK_OEM_2               (191) /* '/?' for US */
#define VK_OEM_3               (192) /* '`~' for US */

#define VK_OEM_4               (219) /* '[{' for US */
#define VK_OEM_5               (220) /* '\|' for US */
#define VK_OEM_6               (221) /* ']}' for US */
#define VK_OEM_7               (222) /* ''"' for US */
#define VK_OEM_8               (223)

#define VK_OEM_AX              (225) /* AX key on Japanese AX keyboard */
#define VK_OEM_102             (226) /* <> or | on RT 102-key keyboard */
#define VK_ICO_HELP            (227) /* Help key on ICO */
#define VK_ICO_00              (228) /* 00 key on ICO */

#endif /* !__USE_W32API */

#endif /* __INCLUDE_KEYBOARD_H */



