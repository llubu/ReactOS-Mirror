#ifndef __INCLUDE_DDK_IODEF_H
#define __INCLUDE_DDK_IODEF_H

typedef enum _IO_QUERY_DEVICE_DESCRIPTION
{
   IoQueryDeviceIdentifier = 0,
   IoQueryDeviceConfigurationData,
   IoQueryDeviceComponentInformation,
   IoQueryDeviceDataFormatMaximum,
} IO_QUERY_DEVICE_DESCRIPTION, *PIO_QUERY_DEVICE_DESCRIPTION;

typedef enum _CONFIGURATION_TYPE
{
   DiskController,
   ParallelController,
   MaximumType,
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

typedef enum _CM_RESOURCE_TYPE
{
   CmResourceTypePort = 1,
   CmResourceTypeInterrupt,
   CmResourceTypeMemory,
   CmResourceTypeDma,
   CmResourceTypeDeviceSpecific,
   CmResourceTypeMaximum,
} CM_RESOURCE_TYPE;

typedef enum _CM_SHARE_DISPOSITION
{
   CmResourceShareDeviceExclusive = 1,
   CmResourceShareDriverExclusive,
   CmResourceShareShared,
   CmResourceShareMaximum,
} CM_SHARE_DISPOSITION;

enum
{
   CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE,
   CM_RESOURCE_INTERRUPT_LATCHED,
};

enum
{
   CM_RESOURCE_PORT_MEMORY,
   CM_RESOURCE_PORT_IO,
};

/*
 * PURPOSE: Irp flags
 */
enum
{
   IRP_NOCACHE = 0x1,
   IRP_PAGING_IO = 0x2,     
   IRP_MOUNT_COMPLETION = 0x4,     
   IRP_SYNCHRONOUS_API = 0x8,     
   IRP_ASSOCIATED_IRP = 0x10,
   IRP_BUFFERED_IO = 0x20,     
   IRP_DEALLOCATE_BUFFER = 0x40,     
   IRP_INPUT_OPERATION = 0x80,     
   IRP_SYNCHRONOUS_PAGING_IO = 0x100,     
   IRP_CREATE_OPERATION = 0x200,     
   IRP_READ_OPERATION = 0x400,     
   IRP_WRITE_OPERATION = 0x800,          
   IRP_CLOSE_OPERATION = 0x1000,
   IRP_DEFER_IO_COMPLETION = 0x2000,
};

#define SL_FORCE_ACCESS_CHECK      (0x1)
#define SL_OPEN_PAGING_FILE        (0x2)
#define SL_OPEN_TARGET_DIRECTORY   (0x4)
#define SL_CASE_SENSITIVE          (0x80)

#define SL_KEY_SPECIFIED           (0x1)
#define SL_OVERRIDE_VERIFY_VOLUME  (0x2)
#define SL_WRITE_THROUGHT          (0x4)
#define SL_FT_SEQUENTIAL_WRITE     (0x8)

#define SL_FAIL_IMMEDIATELY        (0x1)
#define SL_EXCLUSIVE_LOCK          (0x2)

#define SL_WATCH_TREE              (0x1)

#define SL_RESTART_SCAN        (0x1)
#define SL_RETURN_SINGLE_ENTRY (0x2)
#define SL_INDEX_SPECIFIED     (0x4)

#define SL_ALLOW_RAW_MOUNT  (0x1)

#define SL_PENDING_RETURNED             0x01
#define SL_INVOKE_ON_CANCEL             0x20
#define SL_INVOKE_ON_SUCCESS            0x40
#define SL_INVOKE_ON_ERROR              0x80

/*
 * Possible flags for the device object flags
 */
#define DO_UNLOAD_PENDING		0x00000001
#define DO_VERIFY_VOLUME		0x00000002
#define DO_BUFFERED_IO			0x00000004
#define DO_EXCLUSIVE			0x00000008
#define DO_DIRECT_IO			0x00000010
#define DO_MAP_IO_BUFFER		0x00000020
#define DO_DEVICE_HAS_NAME		0x00000040
#define DO_DEVICE_INITIALIZING		0x00000080
#define DO_SYSTEM_BOOT_PARTITION	0x00000100
#define DO_LONG_TERM_REQUESTS		0x00000200
#define DO_NEVER_LAST_DEVICE		0x00000400
#define DO_SHUTDOWN_REGISTERED		0x00000800

/*
 * Possible device types
 */
#define	FILE_DEVICE_BEEP		0x00000001
#define	FILE_DEVICE_CD_ROM           	0x00000002
#define	FILE_DEVICE_CD_ROM_FILE_SYSTEM	0x00000003
#define	FILE_DEVICE_CONTROLLER       	0x00000004
#define	FILE_DEVICE_DATALINK         	0x00000005
#define	FILE_DEVICE_DFS              	0x00000006
#define	FILE_DEVICE_DISK             	0x00000007
#define	FILE_DEVICE_DISK_FILE_SYSTEM 	0x00000008
#define	FILE_DEVICE_FILE_SYSTEM      	0x00000009
#define	FILE_DEVICE_INPORT_PORT      	0x0000000a
#define	FILE_DEVICE_KEYBOARD         	0x0000000b
#define	FILE_DEVICE_MAILSLOT         	0x0000000c
#define	FILE_DEVICE_MIDI_IN          	0x0000000d
#define	FILE_DEVICE_MIDI_OUT         	0x0000000e
#define	FILE_DEVICE_MOUSE            	0x0000000f
#define	FILE_DEVICE_MULTI_UNC_PROVIDER 	0x00000010
#define	FILE_DEVICE_NAMED_PIPE       	0x00000011
#define	FILE_DEVICE_NETWORK          	0x00000012
#define	FILE_DEVICE_NETWORK_BROWSER  	0x00000013
#define	FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define	FILE_DEVICE_NULL             	0x00000015
#define	FILE_DEVICE_PARALLEL_PORT    	0x00000016
#define	FILE_DEVICE_PHYSICAL_NETCARD 	0x00000017
#define	FILE_DEVICE_PRINTER          	0x00000018
#define	FILE_DEVICE_SCANNER          	0x00000019
#define	FILE_DEVICE_SERIAL_MOUSE_PORT 	0x0000001a
#define	FILE_DEVICE_SERIAL_PORT      	0x0000001b
#define	FILE_DEVICE_SCREEN           	0x0000001c
#define	FILE_DEVICE_SOUND            	0x0000001d
#define	FILE_DEVICE_STREAMS          	0x0000001e
#define	FILE_DEVICE_TAPE             	0x0000001f
#define	FILE_DEVICE_TAPE_FILE_SYSTEM 	0x00000020
#define	FILE_DEVICE_TRANSPORT        	0x00000021
#define	FILE_DEVICE_UNKNOWN          	0x00000022
#define	FILE_DEVICE_VIDEO            	0x00000023
#define	FILE_DEVICE_VIRTUAL_DISK     	0x00000024
#define	FILE_DEVICE_WAVE_IN          	0x00000025
#define	FILE_DEVICE_WAVE_OUT         	0x00000026
#define	FILE_DEVICE_8042_PORT        	0x00000027
#define	FILE_DEVICE_NETWORK_REDIRECTOR	0x00000028
#define	FILE_DEVICE_BATTERY          	0x00000029
#define	FILE_DEVICE_BUS_EXTENDER     	0x0000002a
#define	FILE_DEVICE_MODEM            	0x0000002b
#define	FILE_DEVICE_VDM              	0x0000002c

#define	FILE_REMOVABLE_MEDIA            0x00000001
#define FILE_READ_ONLY_DEVICE           0x00000002
#define FILE_FLOPPY_DISKETTE            0x00000004
#define FILE_WRITE_ONCE_MEDIA           0x00000008
#define FILE_REMOTE_DEVICE              0x00000010
#define FILE_DEVICE_IS_MOUNTED          0x00000020
#define FILE_VIRTUAL_VOLUME             0x00000040


/*
 * PURPOSE: Bus types
 */
typedef enum _INTERFACE_TYPE
{
   InterfaceTypeUndefined = -1,
   Internal,
   Isa,
   Eisa,
   MicroChannel,
   TurboChannel,
   PCIBus,
   VMEBus,
   NuBus,
   PCMCIABus,
   CBus,
   MPIBus,
   MPSABus,
   ProcessorInternal,
   InternalPowerBus,
   PNPISABus,
   MaximumInterfaceType
} INTERFACE_TYPE, *PINTERFACE_TYPE;


/*
 * FIXME: These are not in the correct order
 */
enum
{  
     IRP_MJ_CREATE,
     IRP_MJ_CREATE_NAMED_PIPE,
     IRP_MJ_CLOSE,
     IRP_MJ_READ,
     IRP_MJ_WRITE,
     IRP_MJ_QUERY_INFORMATION,
     IRP_MJ_SET_INFORMATION,
     IRP_MJ_QUERY_EA,
     IRP_MJ_SET_EA,
     IRP_MJ_FLUSH_BUFFERS,
     IRP_MJ_QUERY_VOLUME_INFORMATION,
     IRP_MJ_SET_VOLUME_INFORMATION,
     IRP_MJ_DIRECTORY_CONTROL,
     IRP_MJ_FILE_SYSTEM_CONTROL,     
     IRP_MJ_DEVICE_CONTROL,
     IRP_MJ_INTERNAL_DEVICE_CONTROL,
     IRP_MJ_SHUTDOWN,
     IRP_MJ_LOCK_CONTROL,
     IRP_MJ_CLEANUP,
     IRP_MJ_CREATE_MAILSLOT,
     IRP_MJ_QUERY_SECURITY,
     IRP_MJ_SET_SECURITY,
     IRP_MJ_QUERY_POWER,
     IRP_MJ_SET_POWER,
     IRP_MJ_DEVICE_CHANGE,
     IRP_MJ_QUERY_QUOTA,
     IRP_MJ_SET_QUOTA,
     IRP_MJ_PNP_POWER,
     IRP_MJ_MAXIMUM_FUNCTION,
};

/*
 * Minor function numbers for IRP_MJ_FILE_SYSTEM_CONTROL
 */
#define IRP_MN_USER_FS_REQUEST          0x00
#define IRP_MN_MOUNT_VOLUME             0x01
#define IRP_MN_VERIFY_VOLUME            0x02
#define IRP_MN_LOAD_FILE_SYSTEM         0x03

#define IO_DISK_INCREMENT  4

#define  FILE_WORD_ALIGNMENT  0x0001

/*
 * File disposition values
 */
#define  FILE_SUPERSEDED      0x0000
#define  FILE_OPENED          0x0001
#define  FILE_CREATED         0x0002
#define  FILE_OVERWRITTEN     0x0003
#define  FILE_EXISTS          0x0004
#define  FILE_DOES_NOT_EXIST  0x0005


/*
 * file creation flags 
 */
#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200

#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_TRANSACTED_MODE                    0x00200000
#define FILE_OPEN_OFFLINE_FILE                  0x00400000

#define FILE_VALID_OPTION_FLAGS                 0x007fffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00001036


typedef ULONG FS_INFORMATION_CLASS;

/*
 * file system information class values
 */
#define FileFsVolumeInformation 		1
#define FileFsLabelInformation			2
#define FileFsSizeInformation			3
#define FileFsDeviceInformation			4
#define FileFsAttributeInformation		5
#define FileFsControlInformation		6
#define FileFsQuotaQueryInformation		7
#define FileFsQuotaSetInformation		8
#define FileFsMaximumInformation		9

#define IRP_MN_QUERY_DIRECTORY          0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x02

#endif
