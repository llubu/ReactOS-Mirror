<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbhub" type="kernelmodedriver" installbase="system32/drivers" installname="usbhub.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>usbd</library>
	<file>createclose.c</file>
	<file>fdo.c</file>
	<file>misc.c</file>
	<file>pdo.c</file>
	<file>usbhub.c</file>
	<file>usbhub.rc</file>
</module>
