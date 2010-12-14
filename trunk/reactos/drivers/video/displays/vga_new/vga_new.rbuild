<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="vga_new" type="kernelmodedll" entrypoint="DrvEnableDriver@12" installbase="system32" installname="vga_new.dll" crt="libcntpr">
	<importlibrary definition="vga_new.spec" />
	<include base="vga_new">.</include>
	<library>win32k</library>
	<file>debug.c</file>
	<file>enable.c</file>
	<file>palette.c</file>
	<file>screen.c</file>
	<file>vga_new.rc</file>
	<if property="ARCH" value="i386">
		<group compilerset="gcc">
	        	<compilerflag>-mrtd</compilerflag>
        		<compilerflag>-fno-builtin</compilerflag>
			<compilerflag>-Wno-unused-variable</compilerflag>
		</group>
	</if>
	<pch>driver.h</pch>
</module>
