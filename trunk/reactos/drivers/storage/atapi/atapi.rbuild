<module name="atapi" type="kernelmodedriver" installbase="system32/drivers" installname="atapi.sys">
	<bootstrap base="reactos" />
	<define name="__USE_W32API" />
	<include base="atapi">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>scsiport</library>
	<file>atapi.c</file>
	<file>atapi.rc</file>
</module>
