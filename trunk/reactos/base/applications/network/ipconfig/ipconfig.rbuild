<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="ipconfig" type="win32cui" installbase="system32" installname="ipconfig.exe" allowwarnings="true">
    <include base="ipconfig">.</include>
    <define name="__USE_W32API" />
    <library>kernel32</library>
    <library>user32</library>
    <library>iphlpapi</library>
    <file>ipconfig.c</file>
    <file>ipconfig.rc</file>
</module>
