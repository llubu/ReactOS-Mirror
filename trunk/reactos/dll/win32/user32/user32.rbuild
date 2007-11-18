<module name="user32" type="win32dll" baseaddress="${BASEADDRESS_USER32}" installbase="system32" installname="user32.dll" allowwarnings="true">
	<importlibrary definition="user32.def" />
	<include base="user32">.</include>
	<include base="user32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="UNICODE" />
	<define name="WINVER">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>imm32</library>
	<library>win32ksys</library>
	<library>pseh</library>

	<directory name="include">
		<pch>user32.h</pch>
	</directory>
	<directory name="controls">
		<file>button.c</file>
		<file>combo.c</file>
		<file>edit.c</file>
		<file>icontitle.c</file>
		<file>listbox.c</file>
		<file>regcontrol.c</file>
		<file>scrollbar.c</file>
		<file>static.c</file>
	</directory>
	<directory name="misc">
		<file>dde.c</file>
		<file>ddeclient.c</file>
		<file>ddeserver.c</file>
		<file>desktop.c</file>
		<file>display.c</file>
		<file>dllmain.c</file>
		<file>exit.c</file>
		<file>exticon.c</file>
		<file>misc.c</file>
		<file>object.c</file>
		<file>resources.c</file>
		<file>stubs.c</file>
		<file>timer.c</file>
		<file>winhelp.c</file>
		<file>winsta.c</file>
		<file>wsprintf.c</file>
	</directory>
	<directory name="windows">
		<file>accel.c</file>
		<file>bitmap.c</file>
		<file>caret.c</file>
		<file>class.c</file>
		<file>clipboard.c</file>
		<file>cursor.c</file>
		<file>dc.c</file>
		<file>defwnd.c</file>
		<file>dialog.c</file>
		<file>draw.c</file>
		<file>font.c</file>
		<file>hook.c</file>
		<file>icon.c</file>
		<file>input.c</file>
		<file>mdi.c</file>
		<file>menu.c</file>
		<file>message.c</file>
		<file>messagebox.c</file>
		<file>nonclient.c</file>
		<file>paint.c</file>
		<file>prop.c</file>
		<file>rect.c</file>
		<file>spy.c</file>
		<file>text.c</file>
		<file>window.c</file>
		<file>winpos.c</file>
	</directory>
	<linkerflag>-lgcc</linkerflag>
	<linkerflag>-nostartfiles</linkerflag>
	<linkerflag>-nostdlib</linkerflag>
	<file>user32.rc</file>
</module>
