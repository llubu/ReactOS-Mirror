#ifndef __INCLUDE_DDK_LDRTYPES_H
#define __INCLUDE_DDK_LDRTYPES_H
/* $Id: ldrtypes.h,v 1.1 2001/06/22 12:39:47 ekohl Exp $ */

typedef struct _LDR_RESOURCE_INFO
{
    ULONG Type;
    ULONG Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

#define RESOURCE_TYPE_LEVEL      0
#define RESOURCE_NAME_LEVEL      1
#define RESOURCE_LANGUAGE_LEVEL  2
#define RESOURCE_DATA_LEVEL      3

#endif /* __INCLUDE_DDK_LDRTYPES_H */
