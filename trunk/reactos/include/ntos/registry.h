/* $Id: registry.h,v 1.3 2002/06/19 22:30:29 ekohl Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/registry.h
 * PURPOSE:      Registry declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   Eric Kohl <ekohl@rz-online.de>
 * UPDATE HISTORY: 
 *               25/01/2001: Created
 */

#ifndef __INCLUDE_NTOS_REGISTRY_H
#define __INCLUDE_NTOS_REGISTRY_H

/* Key access rights */
#define KEY_QUERY_VALUE			(1)
#define KEY_SET_VALUE			(2)
#define KEY_CREATE_SUB_KEY		(4)
#define KEY_ENUMERATE_SUB_KEYS		(8)
#define KEY_NOTIFY			(16)
#define KEY_CREATE_LINK			(32)

#define KEY_READ			(0x20019L)
#define KEY_WRITE			(0x20006L)
#define KEY_EXECUTE			(0x20019L)
#define KEY_ALL_ACCESS			(0xf003fL)

/* Key create options */
#define REG_OPTION_NON_VOLATILE		(0x0L)
#define REG_OPTION_VOLATILE		(0x1L)
#define REG_OPTION_CREATE_LINK		(0x2L)
#define REG_OPTION_BACKUP_RESTORE	(0x8L)
#define REG_OPTION_OPEN_LINK		(0x8L)

/* Key create/open disposition */
#define REG_CREATED_NEW_KEY		(0x1L)
#define REG_OPENED_EXISTING_KEY		(0x2L)

/* Value types */
#define REG_NONE			(0)
#define REG_SZ				(1)
#define REG_EXPAND_SZ			(2)
#define REG_BINARY			(3)
#define REG_DWORD			(4)
#define REG_DWORD_LITTLE_ENDIAN		(4)
#define REG_DWORD_BIG_ENDIAN		(5)
#define REG_LINK			(6)
#define REG_MULTI_SZ			(7)
#define REG_RESOURCE_LIST		(8)
#define REG_FULL_RESOURCE_DESCRIPTOR	(9)
#define REG_RESOURCE_REQUIREMENTS_LIST	(10)


#endif /* __INCLUDE_NTOS_REGISTRY_H */
