<module name="usbdriver" type="kernelmodedriver" installbase="system32/drivers" installname="usbdriver.sys" allowwarnings="true">
	<define name="INCLUDE_EHCI" />
	<include base="usbdriver">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>ehci.c</file>
	<file>uhci.c</file>
	<file>hub.c</file>
	<file>td.c</file>
	<file>usb.c</file>
	<file>umss.c</file>
	<file>bulkonly.c</file>
	<file>cbi.c</file>
	<file>dmgrdisp.c</file>
	<file>compdrv.c</file>
	<file>etd.c</file>
	<file>gendrv.c</file>
	<file>usbdriver.rc</file>
</module>
