@echo off
if "%1" == "" goto NoParameter
set ROS_INSTALL=%1
goto Install
:NoParameter
set ROS_INSTALL=c:\reactos
:Install
echo Installing to %ROS_INSTALL%
@echo off

set ROS_INSTALL_TESTS=%ROS_INSTALL%\test

md %ROS_INSTALL%
md %ROS_INSTALL%\bin
md %ROS_INSTALL_TESTS%
md %ROS_INSTALL%\symbols
md %ROS_INSTALL%\system32
md %ROS_INSTALL%\system32\config
md %ROS_INSTALL%\system32\drivers
md %ROS_INSTALL%\media
md %ROS_INSTALL%\media\fonts

echo.
echo installing boot files...
copy boot.bat %ROS_INSTALL%
copy bootc.lst %ROS_INSTALL%
copy aboot.bat %ROS_INSTALL%
copy loaders\dos\loadros.com %ROS_INSTALL%

echo.
echo installing ntoskrnl and hal...
copy ntoskrnl\ntoskrnl.exe %ROS_INSTALL%\system32
copy ntoskrnl\ntoskrnl.sym %ROS_INSTALL%\symbols
copy ntoskrnl\ntoskrnl.map %ROS_INSTALL%\symbols
copy hal\halx86\hal.dll %ROS_INSTALL%\system32

echo.
echo installing file system drivers...
copy drivers\fs\vfat\vfatfs.sys %ROS_INSTALL%\system32\drivers
copy drivers\fs\cdfs\cdfs.sys %ROS_INSTALL%\system32\drivers
copy drivers\fs\fs_rec\fs_rec.sys %ROS_INSTALL%\system32\drivers
copy drivers\fs\ms\msfs.sys %ROS_INSTALL%\system32\drivers
copy drivers\fs\np\npfs.sys %ROS_INSTALL%\system32\drivers
copy drivers\fs\ntfs\ntfs.sys %ROS_INSTALL%\system32\drivers
copy drivers\fs\mup\mup.sys %ROS_INSTALL%\system32\drivers

echo.
echo installing bus and misc drivers...
copy drivers\bus\acpi\acpi.sys %ROS_INSTALL%\system32\drivers
copy drivers\bus\isapnp\isapnp.sys %ROS_INSTALL%\system32\drivers
copy drivers\bus\pci\pci.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\floppy\floppy.sys %ROS_INSTALL%\system32\drivers
copy drivers\lib\bzip2\unbzip2.sys %ROS_INSTALL%\system32\drivers

echo.
echo installing input drivers...
copy drivers\input\keyboard\keyboard.sys %ROS_INSTALL%\system32\drivers
copy drivers\input\mouclass\mouclass.sys %ROS_INSTALL%\system32\drivers
copy drivers\input\psaux\psaux.sys %ROS_INSTALL%\system32\drivers
copy drivers\input\sermouse\sermouse.sys %ROS_INSTALL%\system32\drivers

echo.
echo installing dd drivers...
copy drivers\dd\blue\blue.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\beep\beep.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\debugout\debugout.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\null\null.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\serial\serial.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\vga\miniport\vgamp.sys %ROS_INSTALL%\system32\drivers
copy drivers\dd\vga\display\vgaddi.dll %ROS_INSTALL%\system32
copy drivers\dd\videoprt\videoprt.sys %ROS_INSTALL%\system32\drivers

rem copy drivers\dd\serenum\serenum.sys %ROS_INSTALL%\system32\drivers

echo.
echo installing net drivers...
copy drivers\net\afd\afd.sys %ROS_INSTALL%\system32\drivers
copy drivers\net\dd\ne2000\ne2000.sys %ROS_INSTALL%\system32\drivers
copy drivers\net\ndis\ndis.sys %ROS_INSTALL%\system32\drivers
copy drivers\net\tdi\tdi.sys %ROS_INSTALL%\system32\drivers
copy drivers\net\tcpip\tcpip.sys %ROS_INSTALL%\system32\drivers
copy drivers\net\wshtcpip\wshtcpip.dll %ROS_INSTALL%\system32

rem copy drivers\net\dd\miniport\nscirda\nscirda.sys %ROS_INSTALL%\system32\drivers
rem copy drivers\net\packet\packet.sys %ROS_INSTALL%\system32\drivers

echo.
echo installing storage drivers...
copy drivers\storage\atapi\atapi.sys %ROS_INSTALL%\system32\drivers
copy drivers\storage\scsiport\scsiport.sys %ROS_INSTALL%\system32\drivers
copy drivers\storage\cdrom\cdrom.sys %ROS_INSTALL%\system32\drivers
copy drivers\storage\disk\disk.sys %ROS_INSTALL%\system32\drivers
copy drivers\storage\class2\class2.sys %ROS_INSTALL%\system32\drivers

echo.
echo installing system files and services...
copy subsys\system\autochk\autochk.exe %ROS_INSTALL%\system32
copy subsys\system\cmd\cmd.exe %ROS_INSTALL%\system32
copy subsys\system\services\services.exe %ROS_INSTALL%\system32
copy subsys\system\setup\setup.exe %ROS_INSTALL%\system32
copy subsys\system\winlogon\winlogon.exe %ROS_INSTALL%\system32
copy services\eventlog\eventlog.exe %ROS_INSTALL%\system32
copy services\rpcss\rpcss.exe %ROS_INSTALL%\system32
copy subsys\smss\smss.exe %ROS_INSTALL%\system32
copy subsys\csrss\csrss.exe %ROS_INSTALL%\system32
copy subsys\ntvdm\ntvdm.exe %ROS_INSTALL%\system32
copy subsys\win32k\win32k.sys %ROS_INSTALL%\system32
copy subsys\system\usetup\usetup.exe %ROS_INSTALL%\system32

echo.
echo installing user-mode dlls...
copy lib\advapi32\advapi32.dll %ROS_INSTALL%\system32
copy lib\cards\cards.dll %ROS_INSTALL%\system32
copy lib\crtdll\crtdll.dll %ROS_INSTALL%\system32
copy lib\fmifs\fmifs.dll %ROS_INSTALL%\system32
copy lib\freetype\freetype.dll %ROS_INSTALL%\system32
copy lib\gdi32\gdi32.dll %ROS_INSTALL%\system32
copy lib\iphlpapi\iphlpapi.dll %ROS_INSTALL%\system32
copy lib\kernel32\kernel32.dll %ROS_INSTALL%\system32
copy lib\lzexpand\lz32.dll %ROS_INSTALL%\system32
copy lib\msafd\msafd.dll %ROS_INSTALL%\system32
copy lib\msvcrt\msvcrt.dll %ROS_INSTALL%\system32

echo.
echo installing user-mode dlls (2)...
copy lib\ntdll\ntdll.dll %ROS_INSTALL%\system32
copy lib\packet\packet.dll %ROS_INSTALL%\system32
copy lib\secur32\secur32.dll %ROS_INSTALL%\system32
copy lib\syssetup\syssetup.dll %ROS_INSTALL%\system32
copy lib\twain\twain_32.dll %ROS_INSTALL%\system32
copy lib\user32\user32.dll %ROS_INSTALL%\system32
copy lib\version\version.dll %ROS_INSTALL%\system32
copy lib\winedbgc\winedbgc.dll %ROS_INSTALL%\system32
copy lib\winspool\winspool.drv %ROS_INSTALL%\system32
copy lib\ws2_32\ws2_32.dll %ROS_INSTALL%\system32

echo.
echo installing user-mode dlls (3)...
copy lib\ws2help\ws2help.dll %ROS_INSTALL%\system32
copy lib\wshirda\wshirda.dll %ROS_INSTALL%\system32
copy lib\wsock32\wsock32.dll %ROS_INSTALL%\system32
copy lib\kbdus\kbdus.dll %ROS_INSTALL%\system32

rem copy lib\winmm\winmm.dll %ROS_INSTALL%\system32
rem copy lib\shell32\roshel32.dll %ROS_INSTALL%\system32
rem copy lib\snmpapi\snmpapi.dll %ROS_INSTALL%\system32
rem copy lib\libpcap\libpcap.dll %ROS_INSTALL%\system32

echo.
echo installing utilities...
copy apps\utils\cat\cat.exe %ROS_INSTALL%\bin
copy apps\utils\partinfo\partinfo.exe %ROS_INSTALL%\bin
copy apps\utils\objdir\objdir.exe %ROS_INSTALL%\bin
copy apps\utils\pice\pice.cfg %ROS_INSTALL%\symbols
copy apps\utils\sc\sc.exe %ROS_INSTALL%\bin

rem copy apps\utils\pice\module\pice.sys %ROS_INSTALL%\system32\drivers
rem copy apps\utils\pice\module\pice.sym %ROS_INSTALL%\symbols

echo.
echo installing tests...
copy apps\tests\hello\hello.exe %ROS_INSTALL%\bin
copy apps\tests\args\args.exe %ROS_INSTALL%\bin
copy apps\tests\apc\apc.exe %ROS_INSTALL%\bin
copy apps\tests\shm\shmsrv.exe %ROS_INSTALL%\bin
copy apps\tests\shm\shmclt.exe %ROS_INSTALL%\bin
copy apps\tests\lpc\lpcsrv.exe %ROS_INSTALL%\bin
copy apps\tests\lpc\lpcclt.exe %ROS_INSTALL%\bin
copy apps\tests\thread\thread.exe %ROS_INSTALL%\bin
copy apps\tests\enumwnd\enumwnd.exe %ROS_INSTALL%\bin
copy apps\tests\event\event.exe %ROS_INSTALL%\bin

echo.
echo installing tests (2)...
copy apps\tests\file\file.exe %ROS_INSTALL%\bin
copy apps\tests\pteb\pteb.exe %ROS_INSTALL%\bin
copy apps\tests\consume\consume.exe %ROS_INSTALL%\bin
copy apps\tests\button\btntest.exe %ROS_INSTALL_TESTS%
copy apps\tests\button2\btntest2.exe %ROS_INSTALL_TESTS%
copy apps\tests\combo\combotst.exe %ROS_INSTALL_TESTS%
copy apps\tests\lineclip\lineclip.exe %ROS_INSTALL_TESTS%
copy apps\tests\edit\edittest.exe %ROS_INSTALL_TESTS%
copy apps\tests\vmtest\vmtest.exe %ROS_INSTALL_TESTS%
copy apps\tests\gditest\gditest.exe %ROS_INSTALL_TESTS%

echo.
echo installing tests (3)...
copy apps\tests\shaptest\shaptest.exe %ROS_INSTALL_TESTS%
copy apps\tests\dibtest\dibtest.exe %ROS_INSTALL_TESTS%
copy apps\tests\mstest\msserver.exe %ROS_INSTALL_TESTS%
copy apps\tests\mstest\msclient.exe %ROS_INSTALL_TESTS%
copy apps\tests\nptest\npserver.exe %ROS_INSTALL_TESTS%
copy apps\tests\nptest\npclient.exe %ROS_INSTALL_TESTS%
copy apps\tests\atomtest\atomtest.exe %ROS_INSTALL_TESTS%
copy apps\tests\mutex\mutex.exe %ROS_INSTALL%\bin
copy apps\tests\winhello\winhello.exe %ROS_INSTALL%\bin
copy apps\tests\multiwin\multiwin.exe %ROS_INSTALL%\bin

echo.
echo installing tests (4)...
copy apps\tests\wm_paint\wm_paint.exe %ROS_INSTALL_TESTS%
copy apps\tests\bitblt\lena.bmp %ROS_INSTALL_TESTS%
copy apps\tests\bitblt\bitblt.exe %ROS_INSTALL_TESTS%
copy apps\tests\sectest\sectest.exe %ROS_INSTALL_TESTS%
copy apps\tests\isotest\isotest.exe %ROS_INSTALL_TESTS%
copy apps\tests\regtest\regtest.exe %ROS_INSTALL_TESTS%
copy apps\tests\hivetest\hivetest.exe %ROS_INSTALL_TESTS%
copy apps\tests\tokentest\tokentst.exe %ROS_INSTALL_TESTS%
copy apps\tests\icontest\icontest.exe %ROS_INSTALL_TESTS%
copy apps\tests\icontest\icon.ico %ROS_INSTALL_TESTS%

echo.
echo installing tests (5)...
copy apps\tests\wm_paint\wm_paint.exe %ROS_INSTALL_TESTS%
copy apps\tests\primitives\primitives.exe %ROS_INSTALL_TESTS%
copy apps\tests\patblt\patblt.exe %ROS_INSTALL_TESTS%
copy apps\tests\patblt\Penguin.bmp %ROS_INSTALL_TESTS%
copy apps\tests\wm_erasebkgnd\wm_erasebkgnd.exe %ROS_INSTALL_TESTS%
copy apps\tests\wm_erasebkgnd\BACKBITMAP.BMP %ROS_INSTALL_TESTS%
copy apps\tests\stretchblt\stretchblt.exe %ROS_INSTALL_TESTS%
copy apps\tests\txtscale\txtscale.exe %ROS_INSTALL_TESTS%
copy apps\tests\linetest\linetest.exe %ROS_INSTALL_TESTS%
rem copy apps\tests\restest\restest.exe %ROS_INSTALL_TESTS%
rem copy apps\testsets\msvcrt\fileio\fileio.exe %ROS_INSTALL_TESTS%
rem copy apps\testsets\msvcrt\mbtowc\mbtowc.exe %ROS_INSTALL_TESTS%
rem copy apps\testsets\test\test.exe %ROS_INSTALL_TESTS%
rem copy apps\testsets\testperl\testperl.exe %ROS_INSTALL_TESTS%

echo.
echo installing fonts and media...
copy media\fonts\*.ttf %ROS_INSTALL%\media\fonts
copy media\nls\c_1252.nls %ROS_INSTALL%\system32\ansi.nls
copy media\nls\c_1252.nls %ROS_INSTALL%\system32
copy media\nls\c_437.nls %ROS_INSTALL%\system32\oem.nls
copy media\nls\c_437.nls %ROS_INSTALL%\system32
copy media\nls\l_intl.nls %ROS_INSTALL%\system32\casemap.nls
copy media\nls\l_intl.nls %ROS_INSTALL%\system32

tools\mkhive\mkhive bootdata %ROS_INSTALL%\system32\config

if "%ROS_BUILD_EXT%" == "" goto Finish

echo Installing extra programs from rosapps directory...
call ..\rosapps\install.bat %1

echo Installing targets modules ported from WINE...
call ..\wine\install.bat %1

echo Installing targets for POSIX+ support...
call ..\posix\install.bat %1

echo Installing targets for OS/2 support...
call ..\os2\install.bat %1

echo Done.

:Finish
