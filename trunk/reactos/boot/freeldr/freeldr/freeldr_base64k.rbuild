<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freeldr_base64k" type="objectlibrary">
	<include base="freeldr_base64k">include</include>
	<include base="ntoskrnl">include</include>
	<define name="_NTHAL_" />
	<directory name="arch">
		<if property="ARCH" value="i386">
			<directory name="i386">
				<file>boot.S</file>
				<file>drvmap.S</file>
				<file>i386cpu.S</file>
				<file>i386idt.S</file>
				<file>i386pnp.S</file>
				<file>i386pxe.S</file>
				<file>i386trap.S</file>
				<file>int386.S</file>
				<file>linux.S</file>
				<file>mb.S</file>
			</directory>
		</if>
		<if property="ARCH" value="amd64">
			<directory name="amd64">
				<file>drvmap.S</file>
				<file>i386cpu.S</file>
				<file>i386idt.S</file>
				<file>i386trap.S</file>
				<file>mb.S</file>
			</directory>
		</if>
	</directory>
</module>
