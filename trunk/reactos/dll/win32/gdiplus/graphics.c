/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <math.h>
#include <limits.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/unicode.h"

#define COBJMACROS
#include "objbase.h"
#include "ocidl.h"
#include "olectl.h"
#include "ole2.h"

#include "winreg.h"
#include "shlwapi.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/* looks-right constants */
#define TENSION_CONST (0.3)
#define ANCHOR_WIDTH (2.0)
#define MAX_ITERS (50)

/* Converts angle (in degrees) to x/y coordinates */
static void deg2xy(REAL angle, REAL x_0, REAL y_0, REAL *x, REAL *y)
{
    REAL radAngle, hypotenuse;

    radAngle = deg2rad(angle);
    hypotenuse = 50.0; /* arbitrary */

    *x = x_0 + cos(radAngle) * hypotenuse;
    *y = y_0 + sin(radAngle) * hypotenuse;
}

/* Converts from gdiplus path point type to gdi path point type. */
static BYTE convert_path_point_type(BYTE type)
{
    BYTE ret;

    switch(type & PathPointTypePathTypeMask){
        case PathPointTypeBezier:
            ret = PT_BEZIERTO;
            break;
        case PathPointTypeLine:
            ret = PT_LINETO;
            break;
        case PathPointTypeStart:
            ret = PT_MOVETO;
            break;
        default:
            ERR("Bad point type\n");
            return 0;
    }

    if(type & PathPointTypeCloseSubpath)
        ret |= PT_CLOSEFIGURE;

    return ret;
}

static INT prepare_dc(GpGraphics *graphics, GpPen *pen)
{
    HPEN gdipen;
    REAL width;
    INT save_state = SaveDC(graphics->hdc), i, numdashes;
    GpPointF pt[2];
    DWORD dash_array[MAX_DASHLEN];

    EndPath(graphics->hdc);

    if(pen->unit == UnitPixel){
        width = pen->width;
    }
    else{
        /* Get an estimate for the amount the pen width is affected by the world
         * transform. (This is similar to what some of the wine drivers do.) */
        pt[0].X = 0.0;
        pt[0].Y = 0.0;
        pt[1].X = 1.0;
        pt[1].Y = 1.0;
        GdipTransformMatrixPoints(graphics->worldtrans, pt, 2);
        width = sqrt((pt[1].X - pt[0].X) * (pt[1].X - pt[0].X) +
                     (pt[1].Y - pt[0].Y) * (pt[1].Y - pt[0].Y)) / sqrt(2.0);

        width *= pen->width * convert_unit(graphics->hdc,
                              pen->unit == UnitWorld ? graphics->unit : pen->unit);
    }

    if(pen->dash == DashStyleCustom){
        numdashes = min(pen->numdashes, MAX_DASHLEN);

        TRACE("dashes are: ");
        for(i = 0; i < numdashes; i++){
            dash_array[i] = roundr(width * pen->dashes[i]);
            TRACE("%d, ", dash_array[i]);
        }
        TRACE("\n and the pen style is %x\n", pen->style);

        gdipen = ExtCreatePen(pen->style, roundr(width), &pen->brush->lb,
                              numdashes, dash_array);
    }
    else
        gdipen = ExtCreatePen(pen->style, roundr(width), &pen->brush->lb, 0, NULL);

    SelectObject(graphics->hdc, gdipen);

    return save_state;
}

static void restore_dc(GpGraphics *graphics, INT state)
{
    DeleteObject(SelectObject(graphics->hdc, GetStockObject(NULL_PEN)));
    RestoreDC(graphics->hdc, state);
}

/* This helper applies all the changes that the points listed in ptf need in
 * order to be drawn on the device context.  In the end, this should include at
 * least:
 *  -scaling by page unit
 *  -applying world transformation
 *  -converting from float to int
 * Native gdiplus uses gdi32 to do all this (via SetMapMode, SetViewportExtEx,
 * SetWindowExtEx, SetWorldTransform, etc.) but we cannot because we are using
 * gdi to draw, and these functions would irreparably mess with line widths.
 */
static void transform_and_round_points(GpGraphics *graphics, POINT *pti,
    GpPointF *ptf, INT count)
{
    REAL unitscale;
    GpMatrix *matrix;
    int i;

    unitscale = convert_unit(graphics->hdc, graphics->unit);

    /* apply page scale */
    if(graphics->unit != UnitDisplay)
        unitscale *= graphics->scale;

    GdipCloneMatrix(graphics->worldtrans, &matrix);
    GdipScaleMatrix(matrix, unitscale, unitscale, MatrixOrderAppend);
    GdipTransformMatrixPoints(matrix, ptf, count);
    GdipDeleteMatrix(matrix);

    for(i = 0; i < count; i++){
        pti[i].x = roundr(ptf[i].X);
        pti[i].y = roundr(ptf[i].Y);
    }
}

/* GdipDrawPie/GdipFillPie helper function */
static void draw_pie(GpGraphics *graphics, REAL x, REAL y, REAL width,
    REAL height, REAL startAngle, REAL sweepAngle)
{
    GpPointF ptf[4];
    POINT pti[4];

    ptf[0].X = x;
    ptf[0].Y = y;
    ptf[1].X = x + width;
    ptf[1].Y = y + height;

    deg2xy(startAngle+sweepAngle, x + width / 2.0, y + width / 2.0, &ptf[2].X, &ptf[2].Y);
    deg2xy(startAngle, x + width / 2.0, y + width / 2.0, &ptf[3].X, &ptf[3].Y);

    transform_and_round_points(graphics, pti, ptf, 4);

    Pie(graphics->hdc, pti[0].x, pti[0].y, pti[1].x, pti[1].y, pti[2].x,
        pti[2].y, pti[3].x, pti[3].y);
}

/* GdipDrawCurve helper function.
 * Calculates Bezier points from cardinal spline points. */
static void calc_curve_bezier(CONST GpPointF *pts, REAL tension, REAL *x1,
    REAL *y1, REAL *x2, REAL *y2)
{
    REAL xdiff, ydiff;

    /* calculate tangent */
    xdiff = pts[2].X - pts[0].X;
    ydiff = pts[2].Y - pts[0].Y;

    /* apply tangent to get control points */
    *x1 = pts[1].X - tension * xdiff;
    *y1 = pts[1].Y - tension * ydiff;
    *x2 = pts[1].X + tension * xdiff;
    *y2 = pts[1].Y + tension * ydiff;
}

/* GdipDrawCurve helper function.
 * Calculates Bezier points from cardinal spline endpoints. */
static void calc_curve_bezier_endp(REAL xend, REAL yend, REAL xadj, REAL yadj,
    REAL tension, REAL *x, REAL *y)
{
    /* tangent at endpoints is the line from the endpoint to the adjacent point */
    *x = roundr(tension * (xadj - xend) + xend);
    *y = roundr(tension * (yadj - yend) + yend);
}

/* Draws the linecap the specified color and size on the hdc.  The linecap is in
 * direction of the line from x1, y1 to x2, y2 and is anchored on x2, y2. Probably
 * should not be called on an hdc that has a path you care about. */
static void draw_cap(GpGraphics *graphics, COLORREF color, GpLineCap cap, REAL size,
    const GpCustomLineCap *custom, REAL x1, REAL y1, REAL x2, REAL y2)
{
    HGDIOBJ oldbrush = NULL, oldpen = NULL;
    GpMatrix *matrix = NULL;
    HBRUSH brush = NULL;
    HPEN pen = NULL;
    PointF ptf[4], *custptf = NULL;
    POINT pt[4], *custpt = NULL;
    BYTE *tp = NULL;
    REAL theta, dsmall, dbig, dx, dy = 0.0;
    INT i, count;
    LOGBRUSH lb;
    BOOL customstroke;

    if((x1 == x2) && (y1 == y2))
        return;

    theta = gdiplus_atan2(y2 - y1, x2 - x1);

    customstroke = (cap == LineCapCustom) && custom && (!custom->fill);
    if(!customstroke){
        brush = CreateSolidBrush(color);
        lb.lbStyle = BS_SOLID;
        lb.lbColor = color;
        lb.lbHatch = 0;
        pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT |
                           PS_JOIN_MITER, 1, &lb, 0,
                           NULL);
        oldbrush = SelectObject(graphics->hdc, brush);
        oldpen = SelectObject(graphics->hdc, pen);
    }

    switch(cap){
        case LineCapFlat:
            break;
        case LineCapSquare:
        case LineCapSquareAnchor:
        case LineCapDiamondAnchor:
            size = size * (cap & LineCapNoAnchor ? ANCHOR_WIDTH : 1.0) / 2.0;
            if(cap == LineCapDiamondAnchor){
                dsmall = cos(theta + M_PI_2) * size;
                dbig = sin(theta + M_PI_2) * size;
            }
            else{
                dsmall = cos(theta + M_PI_4) * size;
                dbig = sin(theta + M_PI_4) * size;
            }

            ptf[0].X = x2 - dsmall;
            ptf[1].X = x2 + dbig;

            ptf[0].Y = y2 - dbig;
            ptf[3].Y = y2 + dsmall;

            ptf[1].Y = y2 - dsmall;
            ptf[2].Y = y2 + dbig;

            ptf[3].X = x2 - dbig;
            ptf[2].X = x2 + dsmall;

            transform_and_round_points(graphics, pt, ptf, 4);
            Polygon(graphics->hdc, pt, 4);

            break;
        case LineCapArrowAnchor:
            size = size * 4.0 / sqrt(3.0);

            dx = cos(M_PI / 6.0 + theta) * size;
            dy = sin(M_PI / 6.0 + theta) * size;

            ptf[0].X = x2 - dx;
            ptf[0].Y = y2 - dy;

            dx = cos(- M_PI / 6.0 + theta) * size;
            dy = sin(- M_PI / 6.0 + theta) * size;

            ptf[1].X = x2 - dx;
            ptf[1].Y = y2 - dy;

            ptf[2].X = x2;
            ptf[2].Y = y2;

            transform_and_round_points(graphics, pt, ptf, 3);
            Polygon(graphics->hdc, pt, 3);

            break;
        case LineCapRoundAnchor:
            dx = dy = ANCHOR_WIDTH * size / 2.0;

            ptf[0].X = x2 - dx;
            ptf[0].Y = y2 - dy;
            ptf[1].X = x2 + dx;
            ptf[1].Y = y2 + dy;

            transform_and_round_points(graphics, pt, ptf, 2);
            Ellipse(graphics->hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

            break;
        case LineCapTriangle:
            size = size / 2.0;
            dx = cos(M_PI_2 + theta) * size;
            dy = sin(M_PI_2 + theta) * size;

            ptf[0].X = x2 - dx;
            ptf[0].Y = y2 - dy;
            ptf[1].X = x2 + dx;
            ptf[1].Y = y2 + dy;

            dx = cos(theta) * size;
            dy = sin(theta) * size;

            ptf[2].X = x2 + dx;
            ptf[2].Y = y2 + dy;

            transform_and_round_points(graphics, pt, ptf, 3);
            Polygon(graphics->hdc, pt, 3);

            break;
        case LineCapRound:
            dx = dy = size / 2.0;

            ptf[0].X = x2 - dx;
            ptf[0].Y = y2 - dy;
            ptf[1].X = x2 + dx;
            ptf[1].Y = y2 + dy;

            dx = -cos(M_PI_2 + theta) * size;
            dy = -sin(M_PI_2 + theta) * size;

            ptf[2].X = x2 - dx;
            ptf[2].Y = y2 - dy;
            ptf[3].X = x2 + dx;
            ptf[3].Y = y2 + dy;

            transform_and_round_points(graphics, pt, ptf, 4);
            Pie(graphics->hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y, pt[2].x,
                pt[2].y, pt[3].x, pt[3].y);

            break;
        case LineCapCustom:
            if(!custom)
                break;

            count = custom->pathdata.Count;
            custptf = GdipAlloc(count * sizeof(PointF));
            custpt = GdipAlloc(count * sizeof(POINT));
            tp = GdipAlloc(count);

            if(!custptf || !custpt || !tp || (GdipCreateMatrix(&matrix) != Ok))
                goto custend;

            memcpy(custptf, custom->pathdata.Points, count * sizeof(PointF));

            GdipScaleMatrix(matrix, size, size, MatrixOrderAppend);
            GdipRotateMatrix(matrix, (180.0 / M_PI) * (theta - M_PI_2),
                             MatrixOrderAppend);
            GdipTranslateMatrix(matrix, x2, y2, MatrixOrderAppend);
            GdipTransformMatrixPoints(matrix, custptf, count);

            transform_and_round_points(graphics, custpt, custptf, count);

            for(i = 0; i < count; i++)
                tp[i] = convert_path_point_type(custom->pathdata.Types[i]);

            if(custom->fill){
                BeginPath(graphics->hdc);
                PolyDraw(graphics->hdc, custpt, tp, count);
                EndPath(graphics->hdc);
                StrokeAndFillPath(graphics->hdc);
            }
            else
                PolyDraw(graphics->hdc, custpt, tp, count);

custend:
            GdipFree(custptf);
            GdipFree(custpt);
            GdipFree(tp);
            GdipDeleteMatrix(matrix);
            break;
        default:
            break;
    }

    if(!customstroke){
        SelectObject(graphics->hdc, oldbrush);
        SelectObject(graphics->hdc, oldpen);
        DeleteObject(brush);
        DeleteObject(pen);
    }
}

/* Shortens the line by the given percent by changing x2, y2.
 * If percent is > 1.0 then the line will change direction.
 * If percent is negative it can lengthen the line. */
static void shorten_line_percent(REAL x1, REAL  y1, REAL *x2, REAL *y2, REAL percent)
{
    REAL dist, theta, dx, dy;

    if((y1 == *y2) && (x1 == *x2))
        return;

    dist = sqrt((*x2 - x1) * (*x2 - x1) + (*y2 - y1) * (*y2 - y1)) * -percent;
    theta = gdiplus_atan2((*y2 - y1), (*x2 - x1));
    dx = cos(theta) * dist;
    dy = sin(theta) * dist;

    *x2 = *x2 + dx;
    *y2 = *y2 + dy;
}

/* Shortens the line by the given amount by changing x2, y2.
 * If the amount is greater than the distance, the line will become length 0.
 * If the amount is negative, it can lengthen the line. */
static void shorten_line_amt(REAL x1, REAL y1, REAL *x2, REAL *y2, REAL amt)
{
    REAL dx, dy, percent;

    dx = *x2 - x1;
    dy = *y2 - y1;
    if(dx == 0 && dy == 0)
        return;

    percent = amt / sqrt(dx * dx + dy * dy);
    if(percent >= 1.0){
        *x2 = x1;
        *y2 = y1;
        return;
    }

    shorten_line_percent(x1, y1, x2, y2, percent);
}

/* Draws lines between the given points, and if caps is true then draws an endcap
 * at the end of the last line. */
static GpStatus draw_polyline(GpGraphics *graphics, GpPen *pen,
    GDIPCONST GpPointF * pt, INT count, BOOL caps)
{
    POINT *pti = NULL;
    GpPointF *ptcopy = NULL;
    GpStatus status = GenericError;

    if(!count)
        return Ok;

    pti = GdipAlloc(count * sizeof(POINT));
    ptcopy = GdipAlloc(count * sizeof(GpPointF));

    if(!pti || !ptcopy){
        status = OutOfMemory;
        goto end;
    }

    memcpy(ptcopy, pt, count * sizeof(GpPointF));

    if(caps){
        if(pen->endcap == LineCapArrowAnchor)
            shorten_line_amt(ptcopy[count-2].X, ptcopy[count-2].Y,
                             &ptcopy[count-1].X, &ptcopy[count-1].Y, pen->width);
        else if((pen->endcap == LineCapCustom) && pen->customend)
            shorten_line_amt(ptcopy[count-2].X, ptcopy[count-2].Y,
                             &ptcopy[count-1].X, &ptcopy[count-1].Y,
                             pen->customend->inset * pen->width);

        if(pen->startcap == LineCapArrowAnchor)
            shorten_line_amt(ptcopy[1].X, ptcopy[1].Y,
                             &ptcopy[0].X, &ptcopy[0].Y, pen->width);
        else if((pen->startcap == LineCapCustom) && pen->customstart)
            shorten_line_amt(ptcopy[1].X, ptcopy[1].Y,
                             &ptcopy[0].X, &ptcopy[0].Y,
                             pen->customstart->inset * pen->width);

        draw_cap(graphics, pen->brush->lb.lbColor, pen->endcap, pen->width, pen->customend,
                 pt[count - 2].X, pt[count - 2].Y, pt[count - 1].X, pt[count - 1].Y);
        draw_cap(graphics, pen->brush->lb.lbColor, pen->startcap, pen->width, pen->customstart,
                         pt[1].X, pt[1].Y, pt[0].X, pt[0].Y);
    }

    transform_and_round_points(graphics, pti, ptcopy, count);

    if(Polyline(graphics->hdc, pti, count))
        status = Ok;

end:
    GdipFree(pti);
    GdipFree(ptcopy);

    return status;
}

/* Conducts a linear search to find the bezier points that will back off
 * the endpoint of the curve by a distance of amt. Linear search works
 * better than binary in this case because there are multiple solutions,
 * and binary searches often find a bad one. I don't think this is what
 * Windows does but short of rendering the bezier without GDI's help it's
 * the best we can do. If rev then work from the start of the passed points
 * instead of the end. */
static void shorten_bezier_amt(GpPointF * pt, REAL amt, BOOL rev)
{
    GpPointF origpt[4];
    REAL percent = 0.00, dx, dy, origx, origy, diff = -1.0;
    INT i, first = 0, second = 1, third = 2, fourth = 3;

    if(rev){
        first = 3;
        second = 2;
        third = 1;
        fourth = 0;
    }

    origx = pt[fourth].X;
    origy = pt[fourth].Y;
    memcpy(origpt, pt, sizeof(GpPointF) * 4);

    for(i = 0; (i < MAX_ITERS) && (diff < amt); i++){
        /* reset bezier points to original values */
        memcpy(pt, origpt, sizeof(GpPointF) * 4);
        /* Perform magic on bezier points. Order is important here.*/
        shorten_line_percent(pt[third].X, pt[third].Y, &pt[fourth].X, &pt[fourth].Y, percent);
        shorten_line_percent(pt[second].X, pt[second].Y, &pt[third].X, &pt[third].Y, percent);
        shorten_line_percent(pt[third].X, pt[third].Y, &pt[fourth].X, &pt[fourth].Y, percent);
        shorten_line_percent(pt[first].X, pt[first].Y, &pt[second].X, &pt[second].Y, percent);
        shorten_line_percent(pt[second].X, pt[second].Y, &pt[third].X, &pt[third].Y, percent);
        shorten_line_percent(pt[third].X, pt[third].Y, &pt[fourth].X, &pt[fourth].Y, percent);

        dx = pt[fourth].X - origx;
        dy = pt[fourth].Y - origy;

        diff = sqrt(dx * dx + dy * dy);
        percent += 0.0005 * amt;
    }
}

/* Draws bezier curves between given points, and if caps is true then draws an
 * endcap at the end of the last line. */
static GpStatus draw_polybezier(GpGraphics *graphics, GpPen *pen,
    GDIPCONST GpPointF * pt, INT count, BOOL caps)
{
    POINT *pti;
    GpPointF *ptcopy;
    GpStatus status = GenericError;

    if(!count)
        return Ok;

    pti = GdipAlloc(count * sizeof(POINT));
    ptcopy = GdipAlloc(count * sizeof(GpPointF));

    if(!pti || !ptcopy){
        status = OutOfMemory;
        goto end;
    }

    memcpy(ptcopy, pt, count * sizeof(GpPointF));

    if(caps){
        if(pen->endcap == LineCapArrowAnchor)
            shorten_bezier_amt(&ptcopy[count-4], pen->width, FALSE);
        else if((pen->endcap == LineCapCustom) && pen->customend)
            shorten_bezier_amt(&ptcopy[count-4], pen->width * pen->customend->inset,
                               FALSE);

        if(pen->startcap == LineCapArrowAnchor)
            shorten_bezier_amt(ptcopy, pen->width, TRUE);
        else if((pen->startcap == LineCapCustom) && pen->customstart)
            shorten_bezier_amt(ptcopy, pen->width * pen->customstart->inset, TRUE);

        /* the direction of the line cap is parallel to the direction at the
         * end of the bezier (which, if it has been shortened, is not the same
         * as the direction from pt[count-2] to pt[count-1]) */
        draw_cap(graphics, pen->brush->lb.lbColor, pen->endcap, pen->width, pen->customend,
            pt[count - 1].X - (ptcopy[count - 1].X - ptcopy[count - 2].X),
            pt[count - 1].Y - (ptcopy[count - 1].Y - ptcopy[count - 2].Y),
            pt[count - 1].X, pt[count - 1].Y);

        draw_cap(graphics, pen->brush->lb.lbColor, pen->startcap, pen->width, pen->customstart,
            pt[0].X - (ptcopy[0].X - ptcopy[1].X),
            pt[0].Y - (ptcopy[0].Y - ptcopy[1].Y), pt[0].X, pt[0].Y);
    }

    transform_and_round_points(graphics, pti, ptcopy, count);

    PolyBezier(graphics->hdc, pti, count);

    status = Ok;

end:
    GdipFree(pti);
    GdipFree(ptcopy);

    return status;
}

/* Draws a combination of bezier curves and lines between points. */
static GpStatus draw_poly(GpGraphics *graphics, GpPen *pen, GDIPCONST GpPointF * pt,
    GDIPCONST BYTE * types, INT count, BOOL caps)
{
    POINT *pti = GdipAlloc(count * sizeof(POINT));
    BYTE *tp = GdipAlloc(count);
    GpPointF *ptcopy = GdipAlloc(count * sizeof(GpPointF));
    INT i, j;
    GpStatus status = GenericError;

    if(!count){
        status = Ok;
        goto end;
    }
    if(!pti || !tp || !ptcopy){
        status = OutOfMemory;
        goto end;
    }

    for(i = 1; i < count; i++){
        if((types[i] & PathPointTypePathTypeMask) == PathPointTypeBezier){
            if((i + 2 >= count) || !(types[i + 1] & PathPointTypeBezier)
                || !(types[i + 1] & PathPointTypeBezier)){
                ERR("Bad bezier points\n");
                goto end;
            }
            i += 2;
        }
    }

    memcpy(ptcopy, pt, count * sizeof(GpPointF));

    /* If we are drawing caps, go through the points and adjust them accordingly,
     * and draw the caps. */
    if(caps){
        switch(types[count - 1] & PathPointTypePathTypeMask){
            case PathPointTypeBezier:
                if(pen->endcap == LineCapArrowAnchor)
                    shorten_bezier_amt(&ptcopy[count - 4], pen->width, FALSE);
                else if((pen->endcap == LineCapCustom) && pen->customend)
                    shorten_bezier_amt(&ptcopy[count - 4],
                                       pen->width * pen->customend->inset, FALSE);

                draw_cap(graphics, pen->brush->lb.lbColor, pen->endcap, pen->width, pen->customend,
                    pt[count - 1].X - (ptcopy[count - 1].X - ptcopy[count - 2].X),
                    pt[count - 1].Y - (ptcopy[count - 1].Y - ptcopy[count - 2].Y),
                    pt[count - 1].X, pt[count - 1].Y);

                break;
            case PathPointTypeLine:
                if(pen->endcap == LineCapArrowAnchor)
                    shorten_line_amt(ptcopy[count - 2].X, ptcopy[count - 2].Y,
                                     &ptcopy[count - 1].X, &ptcopy[count - 1].Y,
                                     pen->width);
                else if((pen->endcap == LineCapCustom) && pen->customend)
                    shorten_line_amt(ptcopy[count - 2].X, ptcopy[count - 2].Y,
                                     &ptcopy[count - 1].X, &ptcopy[count - 1].Y,
                                     pen->customend->inset * pen->width);

                draw_cap(graphics, pen->brush->lb.lbColor, pen->endcap, pen->width, pen->customend,
                         pt[count - 2].X, pt[count - 2].Y, pt[count - 1].X,
                         pt[count - 1].Y);

                break;
            default:
                ERR("Bad path last point\n");
                goto end;
        }

        /* Find start of points */
        for(j = 1; j < count && ((types[j] & PathPointTypePathTypeMask)
            == PathPointTypeStart); j++);

        switch(types[j] & PathPointTypePathTypeMask){
            case PathPointTypeBezier:
                if(pen->startcap == LineCapArrowAnchor)
                    shorten_bezier_amt(&ptcopy[j - 1], pen->width, TRUE);
                else if((pen->startcap == LineCapCustom) && pen->customstart)
                    shorten_bezier_amt(&ptcopy[j - 1],
                                       pen->width * pen->customstart->inset, TRUE);

                draw_cap(graphics, pen->brush->lb.lbColor, pen->startcap, pen->width, pen->customstart,
                    pt[j - 1].X - (ptcopy[j - 1].X - ptcopy[j].X),
                    pt[j - 1].Y - (ptcopy[j - 1].Y - ptcopy[j].Y),
                    pt[j - 1].X, pt[j - 1].Y);

                break;
            case PathPointTypeLine:
                if(pen->startcap == LineCapArrowAnchor)
                    shorten_line_amt(ptcopy[j].X, ptcopy[j].Y,
                                     &ptcopy[j - 1].X, &ptcopy[j - 1].Y,
                                     pen->width);
                else if((pen->startcap == LineCapCustom) && pen->customstart)
                    shorten_line_amt(ptcopy[j].X, ptcopy[j].Y,
                                     &ptcopy[j - 1].X, &ptcopy[j - 1].Y,
                                     pen->customstart->inset * pen->width);

                draw_cap(graphics, pen->brush->lb.lbColor, pen->startcap, pen->width, pen->customstart,
                         pt[j].X, pt[j].Y, pt[j - 1].X,
                         pt[j - 1].Y);

                break;
            default:
                ERR("Bad path points\n");
                goto end;
        }
    }

    transform_and_round_points(graphics, pti, ptcopy, count);

    for(i = 0; i < count; i++){
        tp[i] = convert_path_point_type(types[i]);
    }

    PolyDraw(graphics->hdc, pti, tp, count);

    status = Ok;

end:
    GdipFree(pti);
    GdipFree(ptcopy);
    GdipFree(tp);

    return status;
}

GpStatus WINGDIPAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics)
{
    return GdipCreateFromHDC2(hdc, NULL, graphics);
}

GpStatus WINGDIPAPI GdipCreateFromHDC2(HDC hdc, HANDLE hDevice, GpGraphics **graphics)
{
    GpStatus retval;

    if(hDevice != NULL) {
        FIXME("Don't know how to hadle parameter hDevice\n");
        return NotImplemented;
    }

    if(hdc == NULL)
        return OutOfMemory;

    if(graphics == NULL)
        return InvalidParameter;

    *graphics = GdipAlloc(sizeof(GpGraphics));
    if(!*graphics)  return OutOfMemory;

    if((retval = GdipCreateMatrix(&(*graphics)->worldtrans)) != Ok){
        GdipFree(*graphics);
        return retval;
    }

    (*graphics)->hdc = hdc;
    (*graphics)->hwnd = NULL;
    (*graphics)->smoothing = SmoothingModeDefault;
    (*graphics)->compqual = CompositingQualityDefault;
    (*graphics)->interpolation = InterpolationModeDefault;
    (*graphics)->pixeloffset = PixelOffsetModeDefault;
    (*graphics)->compmode = CompositingModeSourceOver;
    (*graphics)->unit = UnitDisplay;
    (*graphics)->scale = 1.0;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFromHWND(HWND hwnd, GpGraphics **graphics)
{
    GpStatus ret;

    if((ret = GdipCreateFromHDC(GetDC(hwnd), graphics)) != Ok)
        return ret;

    (*graphics)->hwnd = hwnd;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromEmf(HENHMETAFILE hemf, BOOL delete,
    GpMetafile **metafile)
{
    static int calls;

    if(!hemf || !metafile)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateMetafileFromWmf(HMETAFILE hwmf, BOOL delete,
    GDIPCONST WmfPlaceableFileHeader * placeable, GpMetafile **metafile)
{
    IStream *stream = NULL;
    UINT read;
    BYTE* copy;
    HENHMETAFILE hemf;
    GpStatus retval = GenericError;

    if(!hwmf || !metafile || !placeable)
        return InvalidParameter;

    *metafile = NULL;
    read = GetMetaFileBitsEx(hwmf, 0, NULL);
    if(!read)
        return GenericError;
    copy = GdipAlloc(read);
    GetMetaFileBitsEx(hwmf, read, copy);

    hemf = SetWinMetaFileBits(read, copy, NULL, NULL);
    GdipFree(copy);

    read = GetEnhMetaFileBits(hemf, 0, NULL);
    copy = GdipAlloc(read);
    GetEnhMetaFileBits(hemf, read, copy);
    DeleteEnhMetaFile(hemf);

    if(CreateStreamOnHGlobal(copy, TRUE, &stream) != S_OK){
        ERR("could not make stream\n");
        GdipFree(copy);
        goto err;
    }

    *metafile = GdipAlloc(sizeof(GpMetafile));
    if(!*metafile){
        retval = OutOfMemory;
        goto err;
    }

    if(OleLoadPicture(stream, 0, FALSE, &IID_IPicture,
        (LPVOID*) &((*metafile)->image.picture)) != S_OK)
        goto err;


    (*metafile)->image.type = ImageTypeMetafile;
    (*metafile)->bounds.X = ((REAL) placeable->BoundingBox.Left) / ((REAL) placeable->Inch);
    (*metafile)->bounds.Y = ((REAL) placeable->BoundingBox.Right) / ((REAL) placeable->Inch);
    (*metafile)->bounds.Width = ((REAL) (placeable->BoundingBox.Right
                    - placeable->BoundingBox.Left)) / ((REAL) placeable->Inch);
    (*metafile)->bounds.Height = ((REAL) (placeable->BoundingBox.Bottom
                   - placeable->BoundingBox.Top)) / ((REAL) placeable->Inch);
    (*metafile)->unit = UnitInch;

    if(delete)
        DeleteMetaFile(hwmf);

    return Ok;

err:
    GdipFree(*metafile);
    IStream_Release(stream);
    return retval;
}

GpStatus WINGDIPAPI GdipCreateStreamOnFile(GDIPCONST WCHAR * filename,
    UINT access, IStream **stream)
{
    DWORD dwMode;
    HRESULT ret;

    if(!stream || !filename)
        return InvalidParameter;

    if(access & GENERIC_WRITE)
        dwMode = STGM_SHARE_DENY_WRITE | STGM_WRITE | STGM_CREATE;
    else if(access & GENERIC_READ)
        dwMode = STGM_SHARE_DENY_WRITE | STGM_READ | STGM_FAILIFTHERE;
    else
        return InvalidParameter;

    ret = SHCreateStreamOnFileW(filename, dwMode, stream);

    return hresult_to_status(ret);
}

GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *graphics)
{
    if(!graphics) return InvalidParameter;
    if(graphics->hwnd)
        ReleaseDC(graphics->hwnd, graphics->hdc);

    GdipDeleteMatrix(graphics->worldtrans);
    HeapFree(GetProcessHeap(), 0, graphics);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawArc(GpGraphics *graphics, GpPen *pen, REAL x,
    REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    INT save_state, num_pts;
    GpPointF points[MAX_ARC_PTS];
    GpStatus retval;

    if(!graphics || !pen || width <= 0 || height <= 0)
        return InvalidParameter;

    num_pts = arc2polybezier(points, x, y, width, height, startAngle, sweepAngle);

    save_state = prepare_dc(graphics, pen);

    retval = draw_polybezier(graphics, pen, points, num_pts, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawArcI(GpGraphics *graphics, GpPen *pen, INT x,
    INT y, INT width, INT height, REAL startAngle, REAL sweepAngle)
{
    INT save_state, num_pts;
    GpPointF points[MAX_ARC_PTS];
    GpStatus retval;

    if(!graphics || !pen || width <= 0 || height <= 0)
        return InvalidParameter;

    num_pts = arc2polybezier(points, x, y, width, height, startAngle, sweepAngle);

    save_state = prepare_dc(graphics, pen);

    retval = draw_polybezier(graphics, pen, points, num_pts, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawBezier(GpGraphics *graphics, GpPen *pen, REAL x1,
    REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
{
    INT save_state;
    GpPointF pt[4];
    GpStatus retval;

    if(!graphics || !pen)
        return InvalidParameter;

    pt[0].X = x1;
    pt[0].Y = y1;
    pt[1].X = x2;
    pt[1].Y = y2;
    pt[2].X = x3;
    pt[2].Y = y3;
    pt[3].X = x4;
    pt[3].Y = y4;

    save_state = prepare_dc(graphics, pen);

    retval = draw_polybezier(graphics, pen, pt, 4, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawBezierI(GpGraphics *graphics, GpPen *pen, INT x1,
    INT y1, INT x2, INT y2, INT x3, INT y3, INT x4, INT y4)
{
    INT save_state;
    GpPointF pt[4];
    GpStatus retval;

    if(!graphics || !pen)
        return InvalidParameter;

    pt[0].X = x1;
    pt[0].Y = y1;
    pt[1].X = x2;
    pt[1].Y = y2;
    pt[2].X = x3;
    pt[2].Y = y3;
    pt[3].X = x4;
    pt[3].Y = y4;

    save_state = prepare_dc(graphics, pen);

    retval = draw_polybezier(graphics, pen, pt, 4, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

/* Approximates cardinal spline with Bezier curves. */
GpStatus WINGDIPAPI GdipDrawCurve2(GpGraphics *graphics, GpPen *pen,
    GDIPCONST GpPointF *points, INT count, REAL tension)
{
    /* PolyBezier expects count*3-2 points. */
    INT i, len_pt = count*3-2, save_state;
    GpPointF *pt;
    REAL x1, x2, y1, y2;
    GpStatus retval;

    if(!graphics || !pen)
        return InvalidParameter;

    pt = GdipAlloc(len_pt * sizeof(GpPointF));
    tension = tension * TENSION_CONST;

    calc_curve_bezier_endp(points[0].X, points[0].Y, points[1].X, points[1].Y,
        tension, &x1, &y1);

    pt[0].X = points[0].X;
    pt[0].Y = points[0].Y;
    pt[1].X = x1;
    pt[1].Y = y1;

    for(i = 0; i < count-2; i++){
        calc_curve_bezier(&(points[i]), tension, &x1, &y1, &x2, &y2);

        pt[3*i+2].X = x1;
        pt[3*i+2].Y = y1;
        pt[3*i+3].X = points[i+1].X;
        pt[3*i+3].Y = points[i+1].Y;
        pt[3*i+4].X = x2;
        pt[3*i+4].Y = y2;
    }

    calc_curve_bezier_endp(points[count-1].X, points[count-1].Y,
        points[count-2].X, points[count-2].Y, tension, &x1, &y1);

    pt[len_pt-2].X = x1;
    pt[len_pt-2].Y = y1;
    pt[len_pt-1].X = points[count-1].X;
    pt[len_pt-1].Y = points[count-1].Y;

    save_state = prepare_dc(graphics, pen);

    retval = draw_polybezier(graphics, pen, pt, len_pt, TRUE);

    GdipFree(pt);
    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawImageI(GpGraphics *graphics, GpImage *image, INT x,
    INT y)
{
    UINT width, height, srcw, srch;

    if(!graphics || !image)
        return InvalidParameter;

    GdipGetImageWidth(image, &width);
    GdipGetImageHeight(image, &height);

    srcw = width * (((REAL) INCH_HIMETRIC) /
            ((REAL) GetDeviceCaps(graphics->hdc, LOGPIXELSX)));
    srch = height * (((REAL) INCH_HIMETRIC) /
            ((REAL) GetDeviceCaps(graphics->hdc, LOGPIXELSY)));

    if(image->type != ImageTypeMetafile){
        y += height;
        height *= -1;
    }

    IPicture_Render(image->picture, graphics->hdc, x, y, width, height,
                    0, 0, srcw, srch, NULL);

    return Ok;
}

/* FIXME: partially implemented (only works for rectangular parallelograms) */
GpStatus WINGDIPAPI GdipDrawImagePointsRect(GpGraphics *graphics, GpImage *image,
     GDIPCONST GpPointF *points, INT count, REAL srcx, REAL srcy, REAL srcwidth,
     REAL srcheight, GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes,
     DrawImageAbort callback, VOID * callbackData)
{
    GpPointF ptf[3];
    POINT pti[3];
    REAL dx, dy;

    TRACE("%p %p %p %d %f %f %f %f %d %p %p %p\n", graphics, image, points, count,
          srcx, srcy, srcwidth, srcheight, srcUnit, imageAttributes, callback,
          callbackData);

    if(!graphics || !image || !points || !imageAttributes || count != 3)
         return InvalidParameter;

    if(srcUnit == UnitInch)
        dx = dy = (REAL) INCH_HIMETRIC;
    else if(srcUnit == UnitPixel){
        dx = ((REAL) INCH_HIMETRIC) /
             ((REAL) GetDeviceCaps(graphics->hdc, LOGPIXELSX));
        dy = ((REAL) INCH_HIMETRIC) /
             ((REAL) GetDeviceCaps(graphics->hdc, LOGPIXELSY));
    }
    else
        return NotImplemented;

    memcpy(ptf, points, 3 * sizeof(GpPointF));
    transform_and_round_points(graphics, pti, ptf, 3);

    /* IPicture renders bitmaps with the y-axis reversed
     * FIXME: flipping for unknown image type might not be correct. */
    if(image->type != ImageTypeMetafile){
        INT temp;
        temp = pti[0].y;
        pti[0].y = pti[2].y;
        pti[2].y = temp;
    }

    if(IPicture_Render(image->picture, graphics->hdc,
        pti[0].x, pti[0].y, pti[1].x - pti[0].x, pti[2].y - pti[0].y,
        srcx * dx, srcy * dy,
        srcwidth * dx, srcheight * dy,
        NULL) != S_OK){
        if(callback)
            callback(callbackData);
        return GenericError;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawImageRectRect(GpGraphics *graphics, GpImage *image,
    REAL dstx, REAL dsty, REAL dstwidth, REAL dstheight, REAL srcx, REAL srcy,
    REAL srcwidth, REAL srcheight, GpUnit srcUnit,
    GDIPCONST GpImageAttributes* imageattr, DrawImageAbort callback,
    VOID * callbackData)
{
    GpPointF points[3];

    points[0].X = dstx;
    points[0].Y = dsty;
    points[1].X = dstx + dstwidth;
    points[1].Y = dsty;
    points[2].X = dstx;
    points[2].Y = dsty + dstheight;

    return GdipDrawImagePointsRect(graphics, image, points, 3, srcx, srcy,
               srcwidth, srcheight, srcUnit, imageattr, callback, callbackData);
}

GpStatus WINGDIPAPI GdipDrawImageRectRectI(GpGraphics *graphics, GpImage *image,
	INT dstx, INT dsty, INT dstwidth, INT dstheight, INT srcx, INT srcy,
	INT srcwidth, INT srcheight, GpUnit srcUnit,
	GDIPCONST GpImageAttributes* imageAttributes, DrawImageAbort callback,
	VOID * callbackData)
{
   GpPointF points[3];

    points[0].X = dstx;
    points[0].Y = dsty;
    points[1].X = dstx + dstwidth;
    points[1].Y = dsty;
    points[2].X = dstx;
    points[2].Y = dsty + dstheight;

    return GdipDrawImagePointsRect(graphics, image, points, 3, srcx, srcy,
               srcwidth, srcheight, srcUnit, imageAttributes, callback, callbackData);
}

GpStatus WINGDIPAPI GdipDrawLine(GpGraphics *graphics, GpPen *pen, REAL x1,
    REAL y1, REAL x2, REAL y2)
{
    INT save_state;
    GpPointF pt[2];
    GpStatus retval;

    if(!pen || !graphics)
        return InvalidParameter;

    pt[0].X = x1;
    pt[0].Y = y1;
    pt[1].X = x2;
    pt[1].Y = y2;

    save_state = prepare_dc(graphics, pen);

    retval = draw_polyline(graphics, pen, pt, 2, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawLineI(GpGraphics *graphics, GpPen *pen, INT x1,
    INT y1, INT x2, INT y2)
{
    INT save_state;
    GpPointF pt[2];
    GpStatus retval;

    if(!pen || !graphics)
        return InvalidParameter;

    pt[0].X = (REAL)x1;
    pt[0].Y = (REAL)y1;
    pt[1].X = (REAL)x2;
    pt[1].Y = (REAL)y2;

    save_state = prepare_dc(graphics, pen);

    retval = draw_polyline(graphics, pen, pt, 2, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawLines(GpGraphics *graphics, GpPen *pen, GDIPCONST
    GpPointF *points, INT count)
{
    INT save_state;
    GpStatus retval;

    if(!pen || !graphics || (count < 2))
        return InvalidParameter;

    save_state = prepare_dc(graphics, pen);

    retval = draw_polyline(graphics, pen, points, count, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawLinesI(GpGraphics *graphics, GpPen *pen, GDIPCONST
    GpPoint *points, INT count)
{
    INT save_state;
    GpStatus retval;
    GpPointF *ptf = NULL;
    int i;

    if(!pen || !graphics || (count < 2))
        return InvalidParameter;

    ptf = GdipAlloc(count * sizeof(GpPointF));
    if(!ptf) return OutOfMemory;

    for(i = 0; i < count; i ++){
        ptf[i].X = (REAL) points[i].X;
        ptf[i].Y = (REAL) points[i].Y;
    }

    save_state = prepare_dc(graphics, pen);

    retval = draw_polyline(graphics, pen, ptf, count, TRUE);

    restore_dc(graphics, save_state);

    GdipFree(ptf);
    return retval;
}

GpStatus WINGDIPAPI GdipDrawPath(GpGraphics *graphics, GpPen *pen, GpPath *path)
{
    INT save_state;
    GpStatus retval;

    if(!pen || !graphics)
        return InvalidParameter;

    save_state = prepare_dc(graphics, pen);

    retval = draw_poly(graphics, pen, path->pathdata.Points,
                       path->pathdata.Types, path->pathdata.Count, TRUE);

    restore_dc(graphics, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawPie(GpGraphics *graphics, GpPen *pen, REAL x,
    REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    INT save_state;

    if(!graphics || !pen)
        return InvalidParameter;

    save_state = prepare_dc(graphics, pen);
    SelectObject(graphics->hdc, GetStockObject(NULL_BRUSH));

    draw_pie(graphics, x, y, width, height, startAngle, sweepAngle);

    restore_dc(graphics, save_state);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawRectangleI(GpGraphics *graphics, GpPen *pen, INT x,
    INT y, INT width, INT height)
{
    INT save_state;
    GpPointF ptf[4];
    POINT pti[4];

    if(!pen || !graphics)
        return InvalidParameter;

    ptf[0].X = x;
    ptf[0].Y = y;
    ptf[1].X = x + width;
    ptf[1].Y = y;
    ptf[2].X = x + width;
    ptf[2].Y = y + height;
    ptf[3].X = x;
    ptf[3].Y = y + height;

    save_state = prepare_dc(graphics, pen);
    SelectObject(graphics->hdc, GetStockObject(NULL_BRUSH));

    transform_and_round_points(graphics, pti, ptf, 4);
    Polygon(graphics->hdc, pti, 4);

    restore_dc(graphics, save_state);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawRectangles(GpGraphics *graphics, GpPen *pen,
    GDIPCONST GpRectF* rects, INT count)
{
    GpPointF *ptf;
    POINT *pti;
    INT save_state, i;

    if(!graphics || !pen || !rects || count < 1)
        return InvalidParameter;

    ptf = GdipAlloc(4 * count * sizeof(GpPointF));
    pti = GdipAlloc(4 * count * sizeof(POINT));

    if(!ptf || !pti){
        GdipFree(ptf);
        GdipFree(pti);
        return OutOfMemory;
    }

    for(i = 0; i < count; i++){
        ptf[4 * i + 3].X = ptf[4 * i].X = rects[i].X;
        ptf[4 * i + 1].Y = ptf[4 * i].Y = rects[i].Y;
        ptf[4 * i + 2].X = ptf[4 * i + 1].X = rects[i].X + rects[i].Width;
        ptf[4 * i + 3].Y = ptf[4 * i + 2].Y = rects[i].Y + rects[i].Height;
    }

    save_state = prepare_dc(graphics, pen);
    SelectObject(graphics->hdc, GetStockObject(NULL_BRUSH));

    transform_and_round_points(graphics, pti, ptf, 4 * count);

    for(i = 0; i < count; i++)
        Polygon(graphics->hdc, &pti[4 * i], 4);

    restore_dc(graphics, save_state);

    GdipFree(ptf);
    GdipFree(pti);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawString(GpGraphics *graphics, GDIPCONST WCHAR *string,
    INT length, GDIPCONST GpFont *font, GDIPCONST RectF *rect,
    GDIPCONST GpStringFormat *format, GDIPCONST GpBrush *brush)
{
    HRGN rgn = NULL;
    HFONT gdifont;
    LOGFONTW lfw;
    TEXTMETRICW textmet;
    GpPointF pt[2], rectcpy[4];
    POINT corners[4];
    WCHAR* stringdup;
    REAL angle, ang_cos, ang_sin, rel_width, rel_height;
    INT sum = 0, height = 0, fit, fitcpy, save_state, i, j, lret, nwidth,
        nheight;
    SIZE size;
    RECT drawcoord;

    if(!graphics || !string || !font || !brush || !rect)
        return InvalidParameter;

    if((brush->bt != BrushTypeSolidColor)){
        FIXME("not implemented for given parameters\n");
        return NotImplemented;
    }

    if(format)
        TRACE("may be ignoring some format flags: attr %x\n", format->attr);

    if(length == -1) length = lstrlenW(string);

    stringdup = GdipAlloc(length * sizeof(WCHAR));
    if(!stringdup) return OutOfMemory;

    save_state = SaveDC(graphics->hdc);
    SetBkMode(graphics->hdc, TRANSPARENT);
    SetTextColor(graphics->hdc, brush->lb.lbColor);

    rectcpy[3].X = rectcpy[0].X = rect->X;
    rectcpy[1].Y = rectcpy[0].Y = rect->Y;
    rectcpy[2].X = rectcpy[1].X = rect->X + rect->Width;
    rectcpy[3].Y = rectcpy[2].Y = rect->Y + rect->Height;
    transform_and_round_points(graphics, corners, rectcpy, 4);

    if(roundr(rect->Width) == 0 && roundr(rect->Height) == 0){
        rel_width = rel_height = 1.0;
        nwidth = nheight = INT_MAX;
    }
    else{
        rel_width = sqrt((corners[1].x - corners[0].x) * (corners[1].x - corners[0].x) +
                         (corners[1].y - corners[0].y) * (corners[1].y - corners[0].y))
                         / rect->Width;
        rel_height = sqrt((corners[2].x - corners[1].x) * (corners[2].x - corners[1].x) +
                          (corners[2].y - corners[1].y) * (corners[2].y - corners[1].y))
                          / rect->Height;

        nwidth = roundr(rel_width * rect->Width);
        nheight = roundr(rel_height * rect->Height);
        rgn = CreatePolygonRgn(corners, 4, ALTERNATE);
        SelectClipRgn(graphics->hdc, rgn);
    }

    /* Use gdi to find the font, then perform transformations on it (height,
     * width, angle). */
    SelectObject(graphics->hdc, CreateFontIndirectW(&font->lfw));
    GetTextMetricsW(graphics->hdc, &textmet);
    lfw = font->lfw;

    lfw.lfHeight = roundr(((REAL)lfw.lfHeight) * rel_height);
    lfw.lfWidth = roundr(textmet.tmAveCharWidth * rel_width);

    pt[0].X = 0.0;
    pt[0].Y = 0.0;
    pt[1].X = 1.0;
    pt[1].Y = 0.0;
    GdipTransformMatrixPoints(graphics->worldtrans, pt, 2);
    angle = gdiplus_atan2((pt[1].Y - pt[0].Y), (pt[1].X - pt[0].X));
    ang_cos = cos(angle);
    ang_sin = sin(angle);
    lfw.lfEscapement = lfw.lfOrientation = -roundr((angle / M_PI) * 1800.0);

    gdifont = CreateFontIndirectW(&lfw);
    DeleteObject(SelectObject(graphics->hdc, CreateFontIndirectW(&lfw)));

    for(i = 0, j = 0; i < length; i++){
        if(!isprint(string[i]) && (string[i] != '\n'))
            continue;

        stringdup[j] = string[i];
        j++;
    }

    stringdup[j] = 0;
    length = j;

    while(sum < length){
        drawcoord.left = corners[0].x + roundr(ang_sin * (REAL) height);
        drawcoord.top = corners[0].y + roundr(ang_cos * (REAL) height);

        GetTextExtentExPointW(graphics->hdc, stringdup + sum, length - sum,
                              nwidth, &fit, NULL, &size);
        fitcpy = fit;

        if(fit == 0){
            DrawTextW(graphics->hdc, stringdup + sum, 1, &drawcoord, DT_NOCLIP |
                      DT_EXPANDTABS);
            break;
        }

        for(lret = 0; lret < fit; lret++)
            if(*(stringdup + sum + lret) == '\n')
                break;

        /* Line break code (may look strange, but it imitates windows). */
        if(lret < fit)
            fit = lret;    /* this is not an off-by-one error */
        else if(fit < (length - sum)){
            if(*(stringdup + sum + fit) == ' ')
                while(*(stringdup + sum + fit) == ' ')
                    fit++;
            else
                while(*(stringdup + sum + fit - 1) != ' '){
                    fit--;

                    if(*(stringdup + sum + fit) == '\t')
                        break;

                    if(fit == 0){
                        fit = fitcpy;
                        break;
                    }
                }
        }
        DrawTextW(graphics->hdc, stringdup + sum, min(length - sum, fit),
                  &drawcoord, DT_NOCLIP | DT_EXPANDTABS);

        sum += fit + (lret < fitcpy ? 1 : 0);
        height += size.cy;

        if(height > nheight)
            break;

        /* Stop if this was a linewrap (but not if it was a linebreak). */
        if((lret == fitcpy) && format && (format->attr & StringFormatFlagsNoWrap))
            break;
    }

    GdipFree(stringdup);
    DeleteObject(rgn);
    DeleteObject(gdifont);

    RestoreDC(graphics->hdc, save_state);

    return Ok;
}

GpStatus WINGDIPAPI GdipFillPath(GpGraphics *graphics, GpBrush *brush, GpPath *path)
{
    INT save_state;
    GpStatus retval;

    if(!brush || !graphics || !path)
        return InvalidParameter;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, brush->gdibrush);
    SetPolyFillMode(graphics->hdc, (path->fill == FillModeAlternate ? ALTERNATE
                                                                    : WINDING));

    BeginPath(graphics->hdc);
    retval = draw_poly(graphics, NULL, path->pathdata.Points,
                       path->pathdata.Types, path->pathdata.Count, FALSE);

    if(retval != Ok)
        goto end;

    EndPath(graphics->hdc);
    FillPath(graphics->hdc);

    retval = Ok;

end:
    RestoreDC(graphics->hdc, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipFillPie(GpGraphics *graphics, GpBrush *brush, REAL x,
    REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    INT save_state;

    if(!graphics || !brush)
        return InvalidParameter;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, brush->gdibrush);
    SelectObject(graphics->hdc, GetStockObject(NULL_PEN));

    draw_pie(graphics, x, y, width, height, startAngle, sweepAngle);

    RestoreDC(graphics->hdc, save_state);

    return Ok;
}

GpStatus WINGDIPAPI GdipFillPolygon(GpGraphics *graphics, GpBrush *brush,
    GDIPCONST GpPointF *points, INT count, GpFillMode fillMode)
{
    INT save_state;
    GpPointF *ptf = NULL;
    POINT *pti = NULL;
    GpStatus retval = Ok;

    if(!graphics || !brush || !points || !count)
        return InvalidParameter;

    ptf = GdipAlloc(count * sizeof(GpPointF));
    pti = GdipAlloc(count * sizeof(POINT));
    if(!ptf || !pti){
        retval = OutOfMemory;
        goto end;
    }

    memcpy(ptf, points, count * sizeof(GpPointF));

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, brush->gdibrush);
    SelectObject(graphics->hdc, GetStockObject(NULL_PEN));
    SetPolyFillMode(graphics->hdc, (fillMode == FillModeAlternate ? ALTERNATE
                                                                  : WINDING));

    transform_and_round_points(graphics, pti, ptf, count);
    Polygon(graphics->hdc, pti, count);

    RestoreDC(graphics->hdc, save_state);

end:
    GdipFree(ptf);
    GdipFree(pti);

    return retval;
}

GpStatus WINGDIPAPI GdipFillPolygonI(GpGraphics *graphics, GpBrush *brush,
    GDIPCONST GpPoint *points, INT count, GpFillMode fillMode)
{
    INT save_state, i;
    GpPointF *ptf = NULL;
    POINT *pti = NULL;
    GpStatus retval = Ok;

    if(!graphics || !brush || !points || !count)
        return InvalidParameter;

    ptf = GdipAlloc(count * sizeof(GpPointF));
    pti = GdipAlloc(count * sizeof(POINT));
    if(!ptf || !pti){
        retval = OutOfMemory;
        goto end;
    }

    for(i = 0; i < count; i ++){
        ptf[i].X = (REAL) points[i].X;
        ptf[i].Y = (REAL) points[i].Y;
    }

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, brush->gdibrush);
    SelectObject(graphics->hdc, GetStockObject(NULL_PEN));
    SetPolyFillMode(graphics->hdc, (fillMode == FillModeAlternate ? ALTERNATE
                                                                  : WINDING));

    transform_and_round_points(graphics, pti, ptf, count);
    Polygon(graphics->hdc, pti, count);

    RestoreDC(graphics->hdc, save_state);

end:
    GdipFree(ptf);
    GdipFree(pti);

    return retval;
}

GpStatus WINGDIPAPI GdipFillRectangle(GpGraphics *graphics, GpBrush *brush,
    REAL x, REAL y, REAL width, REAL height)
{
    INT save_state;
    GpPointF ptf[4];
    POINT pti[4];

    if(!graphics || !brush)
        return InvalidParameter;

    ptf[0].X = x;
    ptf[0].Y = y;
    ptf[1].X = x + width;
    ptf[1].Y = y;
    ptf[2].X = x + width;
    ptf[2].Y = y + height;
    ptf[3].X = x;
    ptf[3].Y = y + height;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, brush->gdibrush);
    SelectObject(graphics->hdc, GetStockObject(NULL_PEN));

    transform_and_round_points(graphics, pti, ptf, 4);

    Polygon(graphics->hdc, pti, 4);

    RestoreDC(graphics->hdc, save_state);

    return Ok;
}

GpStatus WINGDIPAPI GdipFillRectangleI(GpGraphics *graphics, GpBrush *brush,
    INT x, INT y, INT width, INT height)
{
    INT save_state;
    GpPointF ptf[4];
    POINT pti[4];

    if(!graphics || !brush)
        return InvalidParameter;

    ptf[0].X = x;
    ptf[0].Y = y;
    ptf[1].X = x + width;
    ptf[1].Y = y;
    ptf[2].X = x + width;
    ptf[2].Y = y + height;
    ptf[3].X = x;
    ptf[3].Y = y + height;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, brush->gdibrush);
    SelectObject(graphics->hdc, GetStockObject(NULL_PEN));

    transform_and_round_points(graphics, pti, ptf, 4);

    Polygon(graphics->hdc, pti, 4);

    RestoreDC(graphics->hdc, save_state);

    return Ok;
}

/* FIXME: Compositing mode is not used anywhere except the getter/setter. */
GpStatus WINGDIPAPI GdipGetCompositingMode(GpGraphics *graphics,
    CompositingMode *mode)
{
    if(!graphics || !mode)
        return InvalidParameter;

    *mode = graphics->compmode;

    return Ok;
}

/* FIXME: Compositing quality is not used anywhere except the getter/setter. */
GpStatus WINGDIPAPI GdipGetCompositingQuality(GpGraphics *graphics,
    CompositingQuality *quality)
{
    if(!graphics || !quality)
        return InvalidParameter;

    *quality = graphics->compqual;

    return Ok;
}

/* FIXME: Interpolation mode is not used anywhere except the getter/setter. */
GpStatus WINGDIPAPI GdipGetInterpolationMode(GpGraphics *graphics,
    InterpolationMode *mode)
{
    if(!graphics || !mode)
        return InvalidParameter;

    *mode = graphics->interpolation;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPageScale(GpGraphics *graphics, REAL *scale)
{
    if(!graphics || !scale)
        return InvalidParameter;

    *scale = graphics->scale;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPageUnit(GpGraphics *graphics, GpUnit *unit)
{
    if(!graphics || !unit)
        return InvalidParameter;

    *unit = graphics->unit;

    return Ok;
}

/* FIXME: Pixel offset mode is not used anywhere except the getter/setter. */
GpStatus WINGDIPAPI GdipGetPixelOffsetMode(GpGraphics *graphics, PixelOffsetMode
    *mode)
{
    if(!graphics || !mode)
        return InvalidParameter;

    *mode = graphics->pixeloffset;

    return Ok;
}

/* FIXME: Smoothing mode is not used anywhere except the getter/setter. */
GpStatus WINGDIPAPI GdipGetSmoothingMode(GpGraphics *graphics, SmoothingMode *mode)
{
    if(!graphics || !mode)
        return InvalidParameter;

    *mode = graphics->smoothing;

    return Ok;
}

/* FIXME: Text rendering hint is not used anywhere except the getter/setter. */
GpStatus WINGDIPAPI GdipGetTextRenderingHint(GpGraphics *graphics,
    TextRenderingHint *hint)
{
    if(!graphics || !hint)
        return InvalidParameter;

    *hint = graphics->texthint;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetWorldTransform(GpGraphics *graphics, GpMatrix *matrix)
{
    if(!graphics || !matrix)
        return InvalidParameter;

    *matrix = *graphics->worldtrans;
    return Ok;
}

/* Find the smallest rectangle that bounds the text when it is printed in rect
 * according to the format options listed in format. If rect has 0 width and
 * height, then just find the smallest rectangle that bounds the text when it's
 * printed at location (rect->X, rect-Y). */
GpStatus WINGDIPAPI GdipMeasureString(GpGraphics *graphics,
    GDIPCONST WCHAR *string, INT length, GDIPCONST GpFont *font,
    GDIPCONST RectF *rect, GDIPCONST GpStringFormat *format, RectF *bounds,
    INT *codepointsfitted, INT *linesfilled)
{
    HFONT oldfont;
    WCHAR* stringdup;
    INT sum = 0, height = 0, fit, fitcpy, max_width = 0, i, j, lret, nwidth,
        nheight;
    SIZE size;

    if(!graphics || !string || !font || !rect)
        return InvalidParameter;

    if(codepointsfitted || linesfilled){
        FIXME("not implemented for given parameters\n");
        return NotImplemented;
    }

    if(format)
        TRACE("may be ignoring some format flags: attr %x\n", format->attr);

    if(length == -1) length = lstrlenW(string);

    stringdup = GdipAlloc(length * sizeof(WCHAR));
    if(!stringdup) return OutOfMemory;

    oldfont = SelectObject(graphics->hdc, CreateFontIndirectW(&font->lfw));
    nwidth = roundr(rect->Width);
    nheight = roundr(rect->Height);

    if((nwidth == 0) && (nheight == 0))
        nwidth = nheight = INT_MAX;

    for(i = 0, j = 0; i < length; i++){
        if(!isprint(string[i]) && (string[i] != '\n'))
            continue;

        stringdup[j] = string[i];
        j++;
    }

    stringdup[j] = 0;
    length = j;

    while(sum < length){
        GetTextExtentExPointW(graphics->hdc, stringdup + sum, length - sum,
                              nwidth, &fit, NULL, &size);
        fitcpy = fit;

        if(fit == 0)
            break;

        for(lret = 0; lret < fit; lret++)
            if(*(stringdup + sum + lret) == '\n')
                break;

        /* Line break code (may look strange, but it imitates windows). */
        if(lret < fit)
            fit = lret;    /* this is not an off-by-one error */
        else if(fit < (length - sum)){
            if(*(stringdup + sum + fit) == ' ')
                while(*(stringdup + sum + fit) == ' ')
                    fit++;
            else
                while(*(stringdup + sum + fit - 1) != ' '){
                    fit--;

                    if(*(stringdup + sum + fit) == '\t')
                        break;

                    if(fit == 0){
                        fit = fitcpy;
                        break;
                    }
                }
        }

        GetTextExtentExPointW(graphics->hdc, stringdup + sum, fit,
                              nwidth, &j, NULL, &size);

        sum += fit + (lret < fitcpy ? 1 : 0);
        height += size.cy;
        max_width = max(max_width, size.cx);

        if(height > nheight)
            break;

        /* Stop if this was a linewrap (but not if it was a linebreak). */
        if((lret == fitcpy) && format && (format->attr & StringFormatFlagsNoWrap))
            break;
    }

    bounds->X = rect->X;
    bounds->Y = rect->Y;
    bounds->Width = (REAL)max_width;
    bounds->Height = (REAL) min(height, nheight);

    GdipFree(stringdup);
    DeleteObject(SelectObject(graphics->hdc, oldfont));

    return Ok;
}

GpStatus WINGDIPAPI GdipRestoreGraphics(GpGraphics *graphics, GraphicsState state)
{
    static int calls;

    if(!graphics)
        return InvalidParameter;

    if(!(calls++))
        FIXME("graphics state not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipRotateWorldTransform(GpGraphics *graphics, REAL angle,
    GpMatrixOrder order)
{
    if(!graphics)
        return InvalidParameter;

    return GdipRotateMatrix(graphics->worldtrans, angle, order);
}

GpStatus WINGDIPAPI GdipSaveGraphics(GpGraphics *graphics, GraphicsState *state)
{
    static int calls;

    if(!graphics || !state)
        return InvalidParameter;

    if(!(calls++))
        FIXME("graphics state not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipScaleWorldTransform(GpGraphics *graphics, REAL sx,
    REAL sy, GpMatrixOrder order)
{
    if(!graphics)
        return InvalidParameter;

    return GdipScaleMatrix(graphics->worldtrans, sx, sy, order);
}

GpStatus WINGDIPAPI GdipSetCompositingMode(GpGraphics *graphics,
    CompositingMode mode)
{
    if(!graphics)
        return InvalidParameter;

    graphics->compmode = mode;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetCompositingQuality(GpGraphics *graphics,
    CompositingQuality quality)
{
    if(!graphics)
        return InvalidParameter;

    graphics->compqual = quality;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetInterpolationMode(GpGraphics *graphics,
    InterpolationMode mode)
{
    if(!graphics)
        return InvalidParameter;

    graphics->interpolation = mode;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPageScale(GpGraphics *graphics, REAL scale)
{
    if(!graphics || (scale <= 0.0))
        return InvalidParameter;

    graphics->scale = scale;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPageUnit(GpGraphics *graphics, GpUnit unit)
{
    if(!graphics || (unit == UnitWorld))
        return InvalidParameter;

    graphics->unit = unit;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPixelOffsetMode(GpGraphics *graphics, PixelOffsetMode
    mode)
{
    if(!graphics)
        return InvalidParameter;

    graphics->pixeloffset = mode;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetSmoothingMode(GpGraphics *graphics, SmoothingMode mode)
{
    if(!graphics)
        return InvalidParameter;

    graphics->smoothing = mode;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetTextRenderingHint(GpGraphics *graphics,
    TextRenderingHint hint)
{
    if(!graphics)
        return InvalidParameter;

    graphics->texthint = hint;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetWorldTransform(GpGraphics *graphics, GpMatrix *matrix)
{
    if(!graphics || !matrix)
        return InvalidParameter;

    GdipDeleteMatrix(graphics->worldtrans);
    return GdipCloneMatrix(matrix, &graphics->worldtrans);
}

GpStatus WINGDIPAPI GdipTranslateWorldTransform(GpGraphics *graphics, REAL dx,
    REAL dy, GpMatrixOrder order)
{
    if(!graphics)
        return InvalidParameter;

    return GdipTranslateMatrix(graphics->worldtrans, dx, dy, order);
}
