<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="buslogic" type="kernelmodedriver" installbase="system32/drivers" installname="buslogic.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="buslogic">.</include>
	<library>scsiport</library>
	<file>BusLogic958.c</file>
	<file>BusLogic958.rc</file>
</module>
