<module name="newdev" type="win32dll" installbase="system32" installname="newdev.dll">
	<include base="newdev">.</include>
	<importlibrary definition="newdev.spec.def" />
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<file>newdev.c</file>
	<file>stubs.c</file>
	<file>wizard.c</file>
	<file>newdev.rc</file>
	<file>newdev.spec</file>
	<library>wine</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>setupapi</library>
</module>
