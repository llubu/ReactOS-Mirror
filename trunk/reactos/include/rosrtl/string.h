/* $Id: string.h,v 1.1 2003/11/17 02:12:52 hyperion Exp $
 */

#ifndef ROSRTL_STRING_H__
#define ROSRTL_STRING_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define RosInitializeString( \
 __PDEST_STRING__, \
 __LENGTH__, \
 __MAXLENGTH__, \
 __BUFFER__ \
) \
{ \
 (__PDEST_STRING__)->Length = (__LENGTH__); \
 (__PDEST_STRING__)->MaximumLength = (__MAXLENGTH__); \
 (__PDEST_STRING__)->Buffer = (__BUFFER__); \
}

#define RtlRosInitStringFromLiteral( \
 __PDEST_STRING__, __SOURCE_STRING__) \
 RosInitializeString( \
  (__PDEST_STRING__), \
  sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
  sizeof(__SOURCE_STRING__), \
  (__SOURCE_STRING__) \
 )
 
#define RtlRosInitUnicodeStringFromLiteral \
 RtlRosInitStringFromLiteral

#define ROS_STRING_INITIALIZER(__SOURCE_STRING__) \
{ \
 sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
 sizeof(__SOURCE_STRING__), \
 (__SOURCE_STRING__) \
}

#define ROS_EMPTY_STRING {0, 0, NULL}

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
