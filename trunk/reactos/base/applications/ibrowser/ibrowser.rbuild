<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <module name="ibrowser" type="win32gui" installbase="system32" installname="ibrowser.exe" allowwarnings ="true">
    <linkerflag>-fexceptions</linkerflag>
    <include base="ibrowser">.</include>
    <include base="ReactOS">include/expat</include>
    <define name="__USE_W32API" />
    <define name="UNICODE" />
    <define name="WIN32" />
    <define name="_ROS_" />
    <define name="_WIN32_IE">0x0600</define>
    <define name="_WIN32_WINNT">0x0501</define>
    <define name="WINVER">0x0500</define>
    <library>uuid</library>
    <library>kernel32</library>
    <library>gdi32</library>
    <library>comctl32</library>
    <library>ole32</library>
    <library>oleaut32</library>
    <library>shell32</library>
    <library>expat</library>
    <pch>precomp.h</pch>
    <directory name="utility">
      <file>utility.cpp</file>
      <file>window.cpp</file>
      <file>xmlstorage.cpp</file>
    </directory>
    <file>ibrowser.cpp</file>
    <file>favorites.cpp</file>
    <file>mainframe.cpp</file>
    <file>webchild.cpp</file>
    <file>ibrowser.rc</file>
  </module>
</rbuild>
