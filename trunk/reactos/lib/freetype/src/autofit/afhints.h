#ifndef __AFHINTS_H__
#define __AFHINTS_H__

#include "aftypes.h"

FT_BEGIN_HEADER

 /*
  *  The definition of outline glyph hints. These are shared by all
  *  script analysis routines (until now)
  *
  */

  typedef enum
  {
    AF_DIMENSION_HORZ = 0,  /* x coordinates, i.e. vertical segments & edges   */
    AF_DIMENSION_VERT = 1,  /* y coordinates, i.e. horizontal segments & edges */

    AF_DIMENSION_MAX  /* do not remove */

  } AF_Dimension;


  /* hint directions -- the values are computed so that two vectors are */
  /* in opposite directions iff `dir1+dir2 == 0'                        */
  typedef enum
  {
    AF_DIR_NONE  =  4,
    AF_DIR_RIGHT =  1,
    AF_DIR_LEFT  = -1,
    AF_DIR_UP    =  2,
    AF_DIR_DOWN  = -2

  } AF_Direction;


  /* point hint flags */
  typedef enum
  {
    AF_FLAG_NONE    = 0,

   /* point type flags */
    AF_FLAG_CONIC   = (1 << 0),
    AF_FLAG_CUBIC   = (1 << 1),
    AF_FLAG_CONTROL = AF_FLAG_CONIC | AF_FLAG_CUBIC,

   /* point extremum flags */
    AF_FLAG_EXTREMA_X = (1 << 2),
    AF_FLAG_EXTREMA_Y = (1 << 3),

   /* point roundness flags */
    AF_FLAG_ROUND_X = (1 << 4),
    AF_FLAG_ROUND_Y = (1 << 5),

   /* point touch flags */
    AF_FLAG_TOUCH_X = (1 << 6),
    AF_FLAG_TOUCH_Y = (1 << 7),

   /* candidates for weak interpolation have this flag set */
    AF_FLAG_WEAK_INTERPOLATION = (1 << 8),

   /* all inflection points in the outline have this flag set */
    AF_FLAG_INFLECTION         = (1 << 9)

  } AF_Flags;


  /* edge hint flags */
  typedef enum
  {
    AF_EDGE_NORMAL = 0,
    AF_EDGE_ROUND  = (1 << 0),
    AF_EDGE_SERIF  = (1 << 1),
    AF_EDGE_DONE   = (1 << 2)

  } AF_Edge_Flags;



  typedef struct AF_PointRec_*    AF_Point;
  typedef struct AF_SegmentRec_*  AF_Segment;
  typedef struct AF_EdgeRec_*     AF_Edge;


  typedef struct  AF_PointRec_
  {
    AF_Flags      flags;    /* point flags used by hinter */
    FT_Pos        ox, oy;   /* original, scaled position  */
    FT_Pos        fx, fy;   /* original, unscaled position (font units) */
    FT_Pos        x,  y;    /* current position */
    FT_Pos        u,  v;    /* current (x,y) or (y,x) depending on context */

    AF_Direction  in_dir;   /* direction of inwards vector  */
    AF_Direction  out_dir;  /* direction of outwards vector */

    AF_Point      next;     /* next point in contour     */
    AF_Point      prev;     /* previous point in contour */

  } AF_PointRec;


  typedef struct  AF_SegmentRec_
  {
    AF_Edge_Flags  flags;       /* edge/segment flags for this segment */
    AF_Direction   dir;         /* segment direction                   */
    FT_Pos         pos;         /* position of segment                 */
    FT_Pos         min_coord;   /* minimum coordinate of segment       */
    FT_Pos         max_coord;   /* maximum coordinate of segment       */

    AF_Edge        edge;        /* the segment's parent edge */
    AF_Segment     edge_next;   /* link to next segment in parent edge */

    AF_Segment     link;        /* (stem) link segment        */
    AF_Segment     serif;       /* primary segment for serifs */
    FT_Pos         num_linked;  /* number of linked segments  */
    FT_Pos         score;       /* used during stem matching  */

    AF_Point       first;       /* first point in edge segment             */
    AF_Point       last;        /* last point in edge segment              */
    AF_Point*      contour;     /* ptr to first point of segment's contour */

  } AF_SegmentRec;


  typedef struct  AF_EdgeRec_
  {
    FT_Pos         fpos;       /* original, unscaled position (font units) */
    FT_Pos         opos;       /* original, scaled position                */
    FT_Pos         pos;        /* current position                         */

    AF_Edge_Flags  flags;      /* edge flags */
    AF_Direction   dir;        /* edge direction */
    FT_Fixed       scale;      /* used to speed up interpolation between edges */
    AF_Width       blue_edge;  /* non-NULL if this is a blue edge              */

    AF_Edge        link;
    AF_Edge        serif;
    FT_Int         num_linked;

    FT_Int         score;

    AF_Segment     first;
    AF_Segment     last;

  } AF_EdgeRec;


  typedef struct AF_AxisHintsRec_
  {
    FT_Int        num_segments;
    AF_Segment    segments;

    FT_Int        num_edges;
    AF_Edge       edges;

    AF_Direction  major_dir;

  } AF_AxisHintsRec, *AF_AxisHints;


  typedef struct AF_GlyphHintsRec_
  {
    FT_Memory     memory;

    FT_Fixed      x_scale;
    FT_Pos        x_delta;

    FT_Fixed      y_scale;
    FT_Pos        y_delta;

    FT_Pos        edge_distance_threshold;

    FT_Int        max_points;
    FT_Int        num_points;
    AF_Point      points;

    FT_Int        max_contours;
    FT_Int        num_contours;
    AF_Point*     contours;

    AF_AxisHintsRec  axis[ AF_DIMENSION_MAX ];
    
    FT_UInt32         scaler_flags;  /* copy of scaler flags */
    FT_UInt32         other_flags;   /* free for script-specific implementations */
    AF_ScriptMetrics  metrics;

  } AF_GlyphHintsRec;


#define  AF_HINTS_TEST_SCALER(h,f)  ( (h)->scaler_flags & (f) )
#define  AF_HINTS_TEST_OTHER(h,f)   ( (h)->other_flags  & (f) )

#define  AF_HINTS_DO_HORIZONTAL(h)  \
            !AF_HINTS_TEST_SCALER(h,AF_SCALER_FLAG_NO_HORIZONTAL)

#define  AF_HINTS_DO_VERTICAL(h)    \
            !AF_HINTS_TEST_SCALER(h,AF_SCALER_FLAG_NO_VERTICAL)

#define  AF_HINTS_DO_ADVANCE(h)     \
            !AF_HINTS_TEST_SCALER(h,AF_SCALER_FLAG_NO_ADVANCE)


  FT_LOCAL( AF_Direction )
  af_direction_compute( FT_Pos  dx,
                        FT_Pos  dy );


  FT_LOCAL( void )
  af_glyph_hints_init( AF_GlyphHints  hints,
                       FT_Memory      memory );



 /*  recomputes all AF_Point in a AF_GlyphHints from the definitions
  *  in a source outline
  */
  FT_LOCAL( FT_Error )
  af_glyph_hints_reset( AF_GlyphHints  hints,
                        AF_Scaler      scaler,
                        FT_Outline*    outline );

  FT_LOCAL( void )
  af_glyph_hints_align_edge_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim );

  FT_LOCAL( void )
  af_glyph_hints_align_strong_points( AF_GlyphHints  hints,
                                      AF_Dimension   dim );

  FT_LOCAL( void )
  af_glyph_hints_align_weak_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim );

  FT_LOCAL( void )
  af_glyph_hints_done( AF_GlyphHints  hints );

/* */

FT_END_HEADER

#endif /* __AFHINTS_H__ */
