<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<group>
	<module name="ntdll" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_NTDLL}" installbase="system32" installname="ntdll.dll">
		<bootstrap installbase="$(CDOUTPUT)/system32" />
		<importlibrary definition="def/ntdll.def" />
		<include base="ntdll">inc</include>
		<include base="ntdll" root="intermediate">def</include>
		<include base="ReactOS">include/reactos/subsys</include>
		<define name="__NTDLL__" />
		<define name="_DISABLE_TIDENTS" />
		<define name="_WIN32_WINNT">0x0502</define>
		<define name="_NTOSKRNL_" />
		<define name="__NO_CTYPE_INLINES" />
		<library>rtl</library>
		<library>ntdllsys</library>
		<library>libcntpr</library>
		<library>pseh</library>
		<dependency>ntstatus</dependency>
		<directory name="csr">
			<file>api.c</file>
			<file>capture.c</file>
			<file>connect.c</file>
		</directory>
		<directory name="dbg">
			<file>dbgui.c</file>
		</directory>
		<directory name="ldr">
			<file>startup.c</file>
			<file>utils.c</file>
		</directory>
		<directory name="main">
			<if property="ARCH" value="i386">
				<directory name="i386">
					<file>dispatch.S</file>
				</directory>
			</if>
			<ifnot property="ARCH" value="i386">
				<file>dispatch.c</file>
			</ifnot>
		</directory>
		<directory name="rtl">
			<file>libsupp.c</file>
			<file>version.c</file>
		</directory>
		<directory name="def">
			<file>ntdll.rc</file>
		</directory>
		<directory name="inc">
			<pch>ntdll.h</pch>
		</directory>
	</module>
</group>