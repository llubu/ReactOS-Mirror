<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="scsiport" type="kernelmodedriver" installbase="system32/drivers" installname="scsiport.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<define name="_SCSIPORT_" />
	<importlibrary definition="scsiport.pspec" />
	<include base="scsiport">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>scsiport.c</file>
	<file>stubs.c</file>
	<file>scsiport.rc</file>
	<pch>precomp.h</pch>
</module>
