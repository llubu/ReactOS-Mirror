<module name="comctl32" type="win32dll" baseaddress="${BASEADDRESS_COMCTL32}" installbase="system32" installname="comctl32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllInstall" />
	<importlibrary definition="comctl32.spec.def" />
	<include base="comctl32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>uxtheme</library>
	<library>winmm</library>
	<file>animate.c</file>
	<file>comboex.c</file>
	<file>comctl32undoc.c</file>
	<file>commctrl.c</file>
	<file>datetime.c</file>
	<file>dpa.c</file>
	<file>draglist.c</file>
	<file>dsa.c</file>
	<file>flatsb.c</file>
	<file>header.c</file>
	<file>hotkey.c</file>
	<file>imagelist.c</file>
	<file>ipaddress.c</file>
	<file>listview.c</file>
	<file>monthcal.c</file>
	<file>nativefont.c</file>
	<file>pager.c</file>
	<file>progress.c</file>
	<file>propsheet.c</file>
	<file>rebar.c</file>
	<file>smoothscroll.c</file>
	<file>string.c</file>
	<file>status.c</file>
	<file>syslink.c</file>
	<file>tab.c</file>
	<file>theme_combo.c</file>
	<file>theme_dialog.c</file>
	<file>theme_edit.c</file>
	<file>theme_listbox.c</file>
	<file>theming.c</file>
	<file>toolbar.c</file>
	<file>tooltips.c</file>
	<file>trackbar.c</file>
	<file>treeview.c</file>
	<file>updown.c</file>
	<file>rsrc.rc</file>
	<file>comctl32.spec</file>
</module>
