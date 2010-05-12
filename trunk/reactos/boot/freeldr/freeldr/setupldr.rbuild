<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="setupldr" type="bootloader">
	<bootstrap installbase="loader" />
	<library>freeldr_startup</library>
	<library>freeldr_base64k</library>
	<library>freeldr_base</library>
	<if property="ARCH" value="i386">
		<library>mini_hal</library>
	</if>
	<library>freeldr_arch</library>
	<library>setupldr_main</library>
	<library>rossym</library>
	<library>cmlib</library>
	<library>rtl</library>
	<library>libcntpr</library>
	<group linkerset="ld">
		<linkerflag>-nostartfiles</linkerflag>
		<linkerflag>-nostdlib</linkerflag>
		<linkerflag>-lgcc</linkerflag>
	</group>
</module>
