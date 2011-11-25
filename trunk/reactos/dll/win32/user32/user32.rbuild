<module name="user32" type="win32dll" baseaddress="${BASEADDRESS_USER32}" installbase="system32" installname="user32.dll" unicode="yes" crt="msvcrt">
	<importlibrary definition="user32.pspec" />
	<include base="user32">.</include>
	<include base="user32">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>user32_wsprintf</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>imm32</library>
	<library>win32ksys</library>
	<library>pseh</library>
	<library>ntdll</library>
	<directory name="include">
		<pch>user32.h</pch>
	</directory>
	<directory name="controls">
		<file>appswitch.c</file>
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
		<file>imm.c</file>
		<file>misc.c</file>
		<file>object.c</file>
		<file>resources.c</file>
		<file>rtlstr.c</file>
		<file>stubs.c</file>
		<file>timer.c</file>
		<file>usrapihk.c</file>
		<file>winhelp.c</file>
		<file>winsta.c</file>
	</directory>
	<directory name="windows">
		<file>accel.c</file>
		<file>caret.c</file>
		<file>class.c</file>
		<file>clipboard.c</file>
		<file>cursoricon.c</file>
		<file>dc.c</file>
		<file>defwnd.c</file>
		<file>dialog.c</file>
		<file>draw.c</file>
		<file>font.c</file>
		<file>hook.c</file>
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
	<file>user32.rc</file>
</module>
