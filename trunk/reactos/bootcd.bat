@echo off
set BOOTCD_DIR=..\bootcd
set FREELDR_DIR=..\freeldr
set ROSAPPS_DIR=..\rosapps

md %BOOTCD_DIR%
md %BOOTCD_DIR%\disk
md %BOOTCD_DIR%\disk\bootdisk
md %BOOTCD_DIR%\disk\loader
md %BOOTCD_DIR%\disk\reactos
md %BOOTCD_DIR%\disk\reactos\system32

rem copy FreeLoader files
copy /Y %FREELDR_DIR%\bootsect\isoboot.bin %BOOTCD_DIR%
copy /Y %FREELDR_DIR%\freeldr\obj\i386\setupldr.sys %BOOTCD_DIR%\disk\loader

copy /Y %FREELDR_DIR%\bootsect\dosmbr.bin %BOOTCD_DIR%\disk\loader
copy /Y %FREELDR_DIR%\bootsect\ext2.bin %BOOTCD_DIR%\disk\loader
copy /Y %FREELDR_DIR%\bootsect\fat.bin %BOOTCD_DIR%\disk\loader
copy /Y %FREELDR_DIR%\bootsect\fat32.bin %BOOTCD_DIR%\disk\loader
copy /Y %FREELDR_DIR%\bootsect\isoboot.bin %BOOTCD_DIR%\disk\loader
copy /Y %FREELDR_DIR%\freeldr\obj\i386\freeldr.sys %BOOTCD_DIR%\disk\loader

rem copy setup files
copy /Y lib\ntdll\ntdll.dll %BOOTCD_DIR%\disk\reactos\system32
copy /Y subsys\system\usetup\usetup.exe %BOOTCD_DIR%\disk\reactos\system32\smss.exe

rem copy data files
copy /Y bootdata\autorun.inf %BOOTCD_DIR%\disk
copy /Y bootdata\readme.txt %BOOTCD_DIR%\disk

copy /Y bootdata\hivecls.inf %BOOTCD_DIR%\disk\reactos
copy /Y bootdata\hivedef.inf %BOOTCD_DIR%\disk\reactos
copy /Y bootdata\hivesft.inf %BOOTCD_DIR%\disk\reactos
copy /Y bootdata\hivesys.inf %BOOTCD_DIR%\disk\reactos

copy /Y bootdata\txtsetup.sif %BOOTCD_DIR%\disk\reactos

rem copy install files
copy /Y ntoskrnl\ntoskrnl.exe %BOOTCD_DIR%\disk\reactos
copy /Y hal\halx86\hal.dll %BOOTCD_DIR%\disk\reactos

copy /Y drivers\bus\acpi\acpi.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\bus\isapnp\isapnp.sys %BOOTCD_DIR%\disk\reactos

copy /Y drivers\dd\beep\beep.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\blue\blue.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\floppy\floppy.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\null\null.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\serial\serial.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\vga\display\vgaddi.dll %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\vga\miniport\vgamp.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\dd\videoprt\videoprt.sys %BOOTCD_DIR%\disk\reactos

copy /Y drivers\fs\cdfs\cdfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\fs_rec\fs_rec.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\ms\msfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\mup\mup.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\np\npfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\ntfs\ntfs.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\fs\vfat\vfatfs.sys %BOOTCD_DIR%\disk\reactos

copy /Y drivers\input\keyboard\keyboard.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\input\mouclass\mouclass.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\input\psaux\psaux.sys %BOOTCD_DIR%\disk\reactos

copy /Y drivers\lib\bzip2\unbzip2.sys %BOOTCD_DIR%\disk\reactos

copy /Y drivers\net\afd\afd.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\net\dd\ne2000\ne2000.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\net\ndis\ndis.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\net\npf\npf.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\net\packet\packet.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\net\tcpip\tcpip.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\net\tdi\tdi.sys %BOOTCD_DIR%\disk\reactos

copy /Y drivers\storage\atapi\atapi.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\cdrom\cdrom.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\class2\class2.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\disk\disk.sys %BOOTCD_DIR%\disk\reactos
copy /Y drivers\storage\scsiport\scsiport.sys %BOOTCD_DIR%\disk\reactos

copy /Y lib\advapi32\advapi32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\crtdll\crtdll.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\fmifs\fmifs.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\freetype\freetype.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\gdi32\gdi32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\kernel32\kernel32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\msafd\msafd.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\msvcrt\msvcrt.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\ntdll\ntdll.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\ole32\ole32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\oleaut32\oleaut32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\packet\packet.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\secur32\secur32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\shell32\shell32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\syssetup\syssetup.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\user32\user32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\version\version.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\winedbgc\winedbgc.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\winmm\winmm.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\ws2_32\ws2_32.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\ws2help\ws2help.dll %BOOTCD_DIR%\disk\reactos
copy /Y lib\wshirda\wshirda.dll %BOOTCD_DIR%\disk\reactos

copy /Y services\eventlog\eventlog.exe %BOOTCD_DIR%\disk\reactos
copy /Y services\rpcss\rpcss.exe %BOOTCD_DIR%\disk\reactos

copy /Y subsys\csrss\csrss.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\ntvdm\ntvdm.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\smss\smss.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\system\autochk\autochk.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\system\cmd\cmd.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\system\lsass\lsass.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\system\services\services.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\system\setup\setup.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\system\winlogon\winlogon.exe %BOOTCD_DIR%\disk\reactos
copy /Y subsys\win32k\win32k.sys %BOOTCD_DIR%\disk\reactos

rem copy media files
copy /Y media\fonts\*.ttf %BOOTCD_DIR%\disk\reactos
copy /Y media\nls\*.nls %BOOTCD_DIR%\disk\reactos

rem copy test apps
copy /Y apps\tests\winhello\winhello.exe %BOOTCD_DIR%\disk\reactos

rem copy rosapps files
copy /Y %ROSAPPS_DIR%\games\winemine\winemine.exe %BOOTCD_DIR%\disk\reactos
copy /Y %ROSAPPS_DIR%\hcalc\hcalc.exe %BOOTCD_DIR%\disk\reactos
copy /Y %ROSAPPS_DIR%\mc\mc.exe %BOOTCD_DIR%\disk\reactos

rem create the reactos.iso image file
tools\cdmake\cdmake -m -b %BOOTCD_DIR%\isoboot.bin %BOOTCD_DIR%\disk REACTOS %BOOTCD_DIR%\reactos.iso
