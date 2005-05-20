/*
 * NTOSKRNL Tag names.
 * License GPL
 *
 * FIXME: Replace with standard GPL Header.
 * FIXME: Add notes as needed
 */

#ifndef _NTOSKRNL_TAG_H
#define _NTOSKRNL_TAG_H

/* formerly located in cc/view.c */
#define TAG_CSEG  TAG('C', 'S', 'E', 'G')
#define TAG_BCB   TAG('B', 'C', 'B', ' ')
#define TAG_IBCB  TAG('i', 'B', 'C', 'B')

/* formerly located in ex/resource.c */
#define TAG_OWNER_TABLE     TAG('R', 'O', 'W', 'N')
#define TAG_EXCLUSIVE_LOCK  TAG('E', 'R', 'E', 'L')
#define TAG_SHARED_SEM      TAG('E', 'R', 'S', 'S')

/* formerly located in fs/notify.c */
#define FSRTL_NOTIFY_TAG TAG('N','O','T','I')

/* formerly located in io/device.c */
#define TAG_DEVICE_EXTENSION   TAG('D', 'E', 'X', 'T')
#define TAG_SHUTDOWN_ENTRY    TAG('S', 'H', 'U', 'T')
#define TAG_IO_TIMER      TAG('I', 'O', 'T', 'M')

/* formerly located in io/driver.c */
#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')

/* formerly located in io/file.c */
#define TAG_SYSB        TAG('S', 'Y', 'S', 'B')
#define TAG_LOCK        TAG('F','l','c','k')
#define TAG_FILE_NAME   TAG('F', 'N', 'A', 'M')

/* formerly located in io/fs.c */
#define TAG_FILE_SYSTEM       TAG('F', 'S', 'Y', 'S')
#define TAG_FS_CHANGE_NOTIFY  TAG('F', 'S', 'C', 'N')

/* formerly located in io/iocomp.c */
#define IOC_TAG   TAG('I', 'O', 'C', 'T')

/* formerly located in io/iomgr.c */
#define TAG_DEVICE_TYPE     TAG('D', 'E', 'V', 'T')
#define TAG_FILE_TYPE       TAG('F', 'I', 'L', 'E')
#define TAG_ADAPTER_TYPE    TAG('A', 'D', 'P', 'T')
#define IO_LARGEIRP         TAG('I', 'r', 'p', 'l')
#define IO_SMALLIRP         TAG('I', 'r', 'p', 's')
#define IO_LARGEIRP_CPU     TAG('I', 'r', 'p', 'L')
#define IO_SMALLIRP_CPU     TAG('I', 'r', 'p', 'S')
#define IOC_TAG1             TAG('I', 'p', 'c', ' ')
#define IOC_CPU             TAG('I', 'p', 'c', 'P')

/* formerly located in io/work.c */
#define TAG_IOWI TAG('I', 'O', 'W', 'I')

/* formerly located in io/irp.c */
#define TAG_IRP      TAG('I', 'R', 'P', ' ')
#define TAG_SYS_BUF  TAG('S', 'Y', 'S' , 'B')

/* formerly located in io/irq.c */
#define TAG_KINTERRUPT   TAG('K', 'I', 'S', 'R')

/* formerly located in io/mdl.c */
#define TAG_MDL    TAG('M', 'D', 'L', ' ')

/* formerly located in io/pnpnotify.c */
#define TAG_PNP_NOTIFY  TAG('P', 'n', 'P', 'N')

/* formerly located in io/resource.c */
#define TAG_IO_RESOURCE    TAG('R', 'S', 'R', 'C')

/* formerly located in io/timer.c */
#define TAG_IO_TIMER      TAG('I', 'O', 'T', 'M')

/* formerly located in io/vpb.c */
#define TAG_VPB    TAG('V', 'P', 'B', ' ')
#define TAG_SYSB   TAG('S', 'Y', 'S', 'B')

/* formerly located in kdbg/kdb_symbols.c */
#define TAG_KDBS TAG('K', 'D', 'B', 'S')

/* formerly located in ldr/loader.c */
#define TAG_DRIVER_MEM  TAG('D', 'R', 'V', 'M') /* drvm */
#define TAG_MODULE_OBJECT TAG('k', 'l', 'm', 'o') /* klmo - kernel ldr module object */
#define TAG_LDR_WSTR TAG('k', 'l', 'w', 's') /* klws - kernel ldr wide string */
#define TAG_MODULE_TEXT_SECTION TAG('k', 'l', 'm', 't') /* klmt - kernel ldr module text */

/* formerly located in lpc/connect */
#define TAG_LPC_CONNECT_MESSAGE   TAG('L', 'P', 'C', 'C')

/* formerly located in mm/aspace.c */
#define TAG_PTRC      TAG('P', 'T', 'R', 'C')

/* formerly located in mm/marea.c */
#define TAG_MAREA   TAG('M', 'A', 'R', 'E')

/* formerly located in mm/pageop.c */
#define TAG_MM_PAGEOP   TAG('M', 'P', 'O', 'P')

/* formerly located in mm/pool.c */
#define TAG_NONE TAG('N', 'o', 'n', 'e')

/* formerly located in mm/region.c */
#define TAG_MM_REGION    TAG('M', 'R', 'G', 'N')

/* formerly located in mm/rmap.c */
#define TAG_RMAP    TAG('R', 'M', 'A', 'P')

/* formerly located in mm/section.c */
#define TAG_MM_SECTION_SEGMENT   TAG('M', 'M', 'S', 'S')
#define TAG_SECTION_PAGE_TABLE   TAG('M', 'S', 'P', 'T')

/* formerly located in ob/symlink.c */
#define TAG_SYMLINK_TTARGET     TAG('S', 'Y', 'T', 'T')
#define TAG_SYMLINK_TARGET      TAG('S', 'Y', 'M', 'T')

/* formerly located in ps/cid.c */
#define TAG_CIDOBJECT TAG('C', 'I', 'D', 'O')

/* formerly located in ps/job.c */
#define TAG_EJOB TAG('E', 'J', 'O', 'B') /* EJOB */

/* formerly located in ps/kill.c */
#define TAG_TERMINATE_APC   TAG('T', 'A', 'P', 'C')

/* formerly located in ps/notify.c */
#define TAG_KAPC TAG('k','p','a','p') /* kpap - kernel ps apc */

/* formerly located in rtl/handle.c */
#define TAG_HDTB  TAG('H', 'D', 'T', 'B')

/* formerly located in se/acl.c */
#define TAG_ACL    TAG('A', 'C', 'L', 'T')

/* formerly located in se/semgr.c */
#define TAG_SXPT   TAG('S', 'X', 'P', 'T')

/* formerly located in se/sid.c */
#define TAG_SID    TAG('S', 'I', 'D', 'T')

#endif /* _NTOSKRNL_TAG_H */
