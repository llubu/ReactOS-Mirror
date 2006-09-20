#ifndef _I8042PRT_H_
#define _I8042PRT_H_

#include <ntifs.h>
#include <kbdmou.h>
#include <ntdd8042.h>
#include <ntddkbd.h>
#include <bugcodes.h>
#include <poclass.h>

/*-----------------------------------------------------
 * Structures
 * --------------------------------------------------*/

typedef enum
{
	dsStopped,
	dsStarted,
	dsPaused,
	dsRemoved,
	dsSurpriseRemoved
} DEVICE_STATE;

typedef struct _I8042_SETTINGS
{
	/* Registry settings */
	ULONG KeyboardDataQueueSize;           /* done */
	UNICODE_STRING KeyboardDeviceBaseName;
	ULONG MouseDataQueueSize;              /* done */
	ULONG MouseResolution;
	ULONG MouseSynchIn100ns;
	ULONG NumberOfButtons;
	UNICODE_STRING PointerDeviceBaseName;
	ULONG PollStatusIterations;            /* done */
	ULONG OverrideKeyboardType;
	ULONG OverrideKeyboardSubtype;
	ULONG PollingIterations;               /* done */
	ULONG PollingIterationsMaximum;
	ULONG ResendIterations;                /* done */
	ULONG SampleRate;
	ULONG CrashOnCtrlScroll;               /* done */
} I8042_SETTINGS, *PI8042_SETTINGS;

typedef enum _MOUSE_TIMEOUT_STATE
{
	NoChange,
	TimeoutStart,
	TimeoutCancel
} MOUSE_TIMEOUT_STATE, *PMOUSE_TIMEOUT_STATE;

typedef struct _INTERRUPT_DATA
{
	PKINTERRUPT Object;
	ULONG Vector;
	KIRQL Dirql;
	KINTERRUPT_MODE InterruptMode;
	BOOLEAN ShareInterrupt;
	KAFFINITY Affinity;
} INTERRUPT_DATA, *PINTERRUPT_DATA;

#define WHEEL_DELTA 120

struct _I8042_KEYBOARD_EXTENSION;
typedef struct _I8042_KEYBOARD_EXTENSION *PI8042_KEYBOARD_EXTENSION;
struct _I8042_MOUSE_EXTENSION;
typedef struct _I8042_MOUSE_EXTENSION *PI8042_MOUSE_EXTENSION;

/* PORT_DEVICE_EXTENSION.Flags */
#define KEYBOARD_PRESENT     0x01 /* A keyboard is attached */
#define MOUSE_PRESENT        0x02 /* A mouse is attached */
#define KEYBOARD_CONNECTED   0x04 /* Keyboard received IOCTL_INTERNAL_KEYBOARD_CONNECT */
#define MOUSE_CONNECTED      0x08 /* Mouse received IOCTL_INTERNAL_MOUSE_CONNECT */
#define KEYBOARD_STARTED     0x04 /* Keyboard FDO received IRP_MN_START_DEVICE */
#define MOUSE_STARTED        0x08 /* Mouse FDO received IRP_MN_START_DEVICE */
#define KEYBOARD_INITIALIZED 0x10 /* Keyboard interrupt is connected */
#define MOUSE_INITIALIZED    0x20 /* Mouse interrupt is connected */

typedef struct _PORT_DEVICE_EXTENSION
{
	PUCHAR DataPort;    /* Usually 0x60 */
	PUCHAR ControlPort; /* Usually 0x64 */
	I8042_SETTINGS Settings;
	ULONG Flags;

	PI8042_KEYBOARD_EXTENSION KeyboardExtension;
	INTERRUPT_DATA KeyboardInterrupt;
	PI8042_MOUSE_EXTENSION MouseExtension;
	INTERRUPT_DATA MouseInterrupt;
	PKINTERRUPT HighestDIRQLInterrupt;
	KSPIN_LOCK SpinLock;
	KIRQL HighestDirql;

	OUTPUT_PACKET Packet;
	ULONG PacketResends;
	BOOLEAN PacketComplete;
	NTSTATUS PacketResult;
	UCHAR PacketBuffer[16];
	UCHAR PacketPort;

	PIRP CurrentIrp;
	PDEVICE_OBJECT CurrentIrpDevice;
} PORT_DEVICE_EXTENSION, *PPORT_DEVICE_EXTENSION;

typedef struct _I8042_DRIVER_EXTENSION
{
	UNICODE_STRING RegistryPath;

	PORT_DEVICE_EXTENSION Port;
} I8042_DRIVER_EXTENSION, *PI8042_DRIVER_EXTENSION;

typedef enum _I8042_DEVICE_TYPE
{
	Unknown,
	Keyboard,
	Mouse,
	PhysicalDeviceObject
} I8042_DEVICE_TYPE, *PI8042_DEVICE_TYPE;

typedef struct _FDO_DEVICE_EXTENSION
{
	I8042_DEVICE_TYPE Type;
	// Associated device object (FDO)
	PDEVICE_OBJECT Fdo;
	// Associated device object (PDO)
	PDEVICE_OBJECT Pdo;
	// Lower device object
	PDEVICE_OBJECT LowerDevice;
	// Current state of the driver
	DEVICE_STATE PnpState;

	PPORT_DEVICE_EXTENSION PortDeviceExtension;
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _I8042_KEYBOARD_EXTENSION
{
	FDO_DEVICE_EXTENSION Common;
	CONNECT_DATA KeyboardData;
	INTERNAL_I8042_HOOK_KEYBOARD KeyboardHook;
	KDPC DpcKeyboard;

	KEYBOARD_INDICATOR_PARAMETERS KeyboardIndicators;

	KEYBOARD_SCAN_STATE KeyboardScanState;
	BOOLEAN KeyComplete;
	PKEYBOARD_INPUT_DATA KeyboardBuffer;
	ULONG KeysInBuffer;

	/* Power keys items */
	ULONG ReportedCaps;
	ULONG NewCaps;
	ULONG LastPowerKey;
	UNICODE_STRING PowerInterfaceName;
	PIO_WORKITEM PowerWorkItem;
	PIRP PowerIrp;

	/* Debug items */
	ULONG ComboPosition;
	PIO_WORKITEM DebugWorkItem;
#ifdef __REACTOS__
	ULONG DebugKey;
	BOOLEAN TabPressed;
#endif
} I8042_KEYBOARD_EXTENSION;

typedef enum _I8042_MOUSE_TYPE
{
	GenericPS2,
	Intellimouse,
	IntellimouseExplorer,
	Ps2pp
} I8042_MOUSE_TYPE, *PI8042_MOUSE_TYPE;

typedef struct _I8042_MOUSE_EXTENSION
{
	FDO_DEVICE_EXTENSION Common;
	CONNECT_DATA MouseData;
	INTERNAL_I8042_HOOK_MOUSE MouseHook;
	KDPC DpcMouse;

	MOUSE_ATTRIBUTES MouseAttributes;

	MOUSE_STATE MouseState;
	BOOLEAN MouseComplete;
	MOUSE_RESET_SUBSTATE MouseResetState;
	PMOUSE_INPUT_DATA MouseBuffer;
	ULONG MouseInBuffer;
	USHORT MouseButtonState;
	ULARGE_INTEGER MousePacketStartTime;

	KTIMER TimerMouseTimeout;
	KDPC DpcMouseTimeout;
	MOUSE_TIMEOUT_STATE MouseTimeoutState;
	BOOLEAN MouseTimeoutActive;

	UCHAR MouseLogiBuffer[3];
	I8042_MOUSE_TYPE MouseType;
} I8042_MOUSE_EXTENSION;

typedef struct _I8042_HOOK_WORKITEM
{
	PIO_WORKITEM WorkItem;
	PIRP Irp;
} I8042_HOOK_WORKITEM, *PI8042_HOOK_WORKITEM;

/*-----------------------------------------------------
 * Some defines
 * --------------------------------------------------*/

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#define KEYBOARD_POWER_CODE 0x5E
#define KEYBOARD_SLEEP_CODE 0x5F
#define KEYBOARD_WAKE_CODE  0x63

/*-----------------------------------------------------
 * Controller commands
 * --------------------------------------------------*/

#define KBD_READ_MODE      0x20
#define KBD_WRITE_MODE     0x60
#define KBD_LINE_TEST      0xAB
#define MOUSE_LINE_TEST    0xA9
#define CTRL_SELF_TEST     0xAA
#define CTRL_WRITE_MOUSE   0xD4

/*-----------------------------------------------------
 * Keyboard commands
 * --------------------------------------------------*/

#define KBD_CMD_SET_LEDS   0xED
#define KBD_CMD_GET_ID     0xF2

/*-----------------------------------------------------
 * Keyboard responses
 * --------------------------------------------------*/

#define KBD_ACK            0xFA
#define KBD_NACK           0xFC
#define KBD_RESEND         0xFE

/*-----------------------------------------------------
 * Controller status register bits
 * --------------------------------------------------*/

#define KBD_OBF            0x01
#define KBD_IBF            0x02
#define MOU_OBF            0x20
#define KBD_PERR           0x80

/*-----------------------------------------------------
 * Controller command byte bits
 * --------------------------------------------------*/

#define CCB_KBD_INT_ENAB   0x01
#define CCB_MOUSE_INT_ENAB 0x02
#define CCB_SYSTEM_FLAG    0x04
#define CCB_KBD_DISAB      0x10
#define CCB_MOUSE_DISAB    0x20
#define CCB_TRANSLATE      0x40

/*-----------------------------------------------------
 * LED bits
 * --------------------------------------------------*/

#define KBD_LED_SCROLL     0x01
#define KBD_LED_NUM        0x02
#define KBD_LED_CAPS       0x04

/*-----------------------------------------------------
 * Mouse commands
 * --------------------------------------------------*/

#define MOU_CMD_GET_ID     0xF2
#define MOU_CMD_RESET      0xFF

/*-----------------------------------------------------
 * Mouse responses
 * --------------------------------------------------*/

#define MOUSE_ACK          0xFA
#define MOUSE_ERROR        0xFC
#define MOUSE_NACK         0xFE

/*-----------------------------------------------------
 * Prototypes
 * --------------------------------------------------*/

/* createclose.c */

VOID NTAPI
i8042SendHookWorkItem(
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID Context);

NTSTATUS NTAPI
i8042Create(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
i8042Cleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
i8042Close(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* keyboard.c */

NTSTATUS NTAPI
i8042SynchWritePortKbd(
	IN PVOID Context,
	IN UCHAR Value,
	IN BOOLEAN WaitForAck);

BOOLEAN
i8042KbdStartIo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
i8042KbdDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
i8042KbdInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

BOOLEAN NTAPI
i8042KbdInterruptService(
	IN PKINTERRUPT Interrupt,
	PVOID Context);

/* i8042prt.c */

NTSTATUS NTAPI
i8042AddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

BOOLEAN
i8042PacketIsr(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Output);

NTSTATUS
i8042StartPacket(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN PFDO_DEVICE_EXTENSION FdoDeviceExtension,
	IN PUCHAR Bytes,
	IN ULONG ByteCount,
	IN PIRP Irp);

/* misc.c */

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* mouse.c */

VOID
i8042MouHandle(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN UCHAR Output);

VOID
i8042MouHandleButtons(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN USHORT Mask);

NTSTATUS
i8042MouInitialize(
	IN PI8042_MOUSE_EXTENSION DeviceExtension);

NTSTATUS
i8042MouInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

BOOLEAN NTAPI
i8042MouInterruptService(
	IN PKINTERRUPT Interrupt,
	PVOID Context);

VOID NTAPI
i8042MouQueuePacket(
	IN PVOID Context);

/* pnp.c */

BOOLEAN
i8042ChangeMode(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR FlagsToDisable,
	IN UCHAR FlagsToEnable);

NTSTATUS NTAPI
i8042Pnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/* ps2pp.c */
VOID
i8042MouHandlePs2pp(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN UCHAR Input);

/* readwrite.c */

VOID
i8042Flush(
	IN PPORT_DEVICE_EXTENSION DeviceExtension);

VOID
i8042IsrWritePort(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Value,
	IN UCHAR SelectCmd OPTIONAL);

NTSTATUS
i8042ReadData(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR StatusFlags,
	OUT PUCHAR Data);
#define i8042ReadKeyboardData(DeviceExtension, Data) \
	i8042ReadData(DeviceExtension, KBD_OBF, Data)
#define i8042ReadMouseData(DeviceExtension, Data) \
	i8042ReadData(DeviceExtension, MOU_OBF, Data)

NTSTATUS
i8042ReadDataWait(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	OUT PUCHAR Data);

NTSTATUS
i8042ReadStatus(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	OUT PUCHAR Status);

NTSTATUS NTAPI
i8042SynchReadPort(
	IN PVOID Context,
	IN PUCHAR Value,
	IN BOOLEAN WaitForAck);

NTSTATUS NTAPI
i8042SynchWritePort(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Port,
	IN UCHAR Value,
	IN BOOLEAN WaitForAck);

BOOLEAN
i8042Write(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN PUCHAR addr,
	IN UCHAR data);

/* registry.c */

NTSTATUS
ReadRegistryEntries(
	IN PUNICODE_STRING RegistryPath,
	OUT PI8042_SETTINGS Settings);

/* setup.c */

BOOLEAN
IsFirstStageSetup(
	VOID);

NTSTATUS
i8042AddLegacyKeyboard(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath);

#endif // _I8042PRT_H_
