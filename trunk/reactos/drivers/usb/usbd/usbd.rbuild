<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbd" type="kernelmodedriver" installbase="system32/drivers" installname="usbd.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<importlibrary definition="usbd.spec" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>usbd.c</file>
	<file>usbd.rc</file>
</module>
