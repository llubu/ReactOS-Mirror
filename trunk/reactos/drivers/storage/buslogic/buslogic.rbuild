<module name="buslogic" type="kernelmodedriver" installbase="system32/drivers" installname="buslogic.sys">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<include base="buslogic">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>scsiport</library>
	<file>BusLogic958.c</file>
	<file>BusLogic958.rc</file>
</module>
