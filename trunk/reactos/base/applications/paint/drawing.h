/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        drawing.h
 * PURPOSE:     The drawing functions used by the tools
 * PROGRAMMERS: Benedikt Freisen
 */

void Line(HDC hdc, short x1, short y1, short x2, short y2, int color, int thickness);

void Rect(HDC hdc, short x1, short y1, short x2, short y2, int fg, int bg, int thickness, BOOL filled);

void Ellp(HDC hdc, short x1, short y1, short x2, short y2, int fg, int bg, int thickness, BOOL filled);

void RRect(HDC hdc, short x1, short y1, short x2, short y2, int fg, int bg, int thickness, BOOL filled);

void Fill(HDC hdc, int x, int y, int color);

void Erase(HDC hdc, short x1, short y1, short x2, short y2, int color, int radius);

void Airbrush(HDC hdc, short x, short y, int color, int r);

void Brush(HDC hdc, short x1, short y1, short x2, short y2, int color, int style);

void RectSel(HDC hdc, short x1, short y1, short x2, short y2);

void SelectionFrame(HDC hdc, int x1, int y1, int x2, int y2);
