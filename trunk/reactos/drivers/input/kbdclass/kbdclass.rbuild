<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdclass" type="kernelmodedriver" installbase="system32/drivers" installname="kbdclass.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<library>pseh</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kbdclass.c</file>
	<file>misc.c</file>
	<file>kbdclass.rc</file>
	<pch>kbdclass.h</pch>
</module>
