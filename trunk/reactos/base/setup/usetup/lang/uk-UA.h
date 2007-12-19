#ifndef LANG_UK_UA_H__
#define LANG_UK_UA_H__

static MUI_ENTRY ukUAWelcomePageEntries[] =
{
    {
        6,
        8,
        "��᪠�� ��ᨬ� �� �ணࠬ� ��⠭���� ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "�� ��⨭� ��⠭���� ���_�� �����_��� ��⥬� ReactOS � ���",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "����'��� _ ����� ���� ��⨭� ��⠭����.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  ����_�� <ENTER> 鮡 ��⠭�� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  ����_�� <R> 鮡 �_������ ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  ����_�� <L> 鮡 ��ॣ���� �_業�_��_ 㬮�� ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  ����_�� <F3> 鮡 ����, �� ��⠭������ ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "��� �_��� �����⭮� _��ଠ�_� �� ReactOS, ��� ��᪠ �_��_����:",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org/uk/",
        TEXT_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER=�த�����  R=�_������ F3=���_�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Setup is in an early development phase. It does not yet",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "support all the functions of a fully usable setup application.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "The following limitations apply:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Setup can not handle more than one primary partition per disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Setup can not delete a primary partition from a disk",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  as long as extended partitions exist on this disk.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Setup can not delete the first extended partition from a disk",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  as long as other extended partitions exist on this disk.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Setup supports FAT file systems only.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- File system checks are not implemented yet.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Press ENTER to install ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Press F3 to quit without installing ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUALicensePageEntries[] =
{
    {
        6,
        6,
        "Licensing:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "The ReactOS System is licensed under the terms of the",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL with parts containing code from other compatible",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licenses such as the X11 or BSD and GNU LGPL licenses.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "All software that is part of the ReactOS system is",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "therefore released under the GNU GPL as well as maintaining",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "the original license.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "This software comes with NO WARRANTY or restrictions on usage",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "save applicable local and international law. The licensing of",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "ReactOS only covers distribution to third parties.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "If for some reason you did not receive a copy of the",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License with ReactOS please visit",
        TEXT_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_HIGHLIGHT
    },
    {
        8,
        22,
        "Warranty:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "This is free software; see the source for copying conditions.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "There is NO warranty; not even for MERCHANTABILITY or",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "FITNESS FOR A PARTICULAR PURPOSE",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Return",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADevicePageEntries[] =
{
    {
        6,
        8,
        "The list below shows the current device settings.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Computer:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Display:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Keyboard:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Keyboard layout:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Accept:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Accept these device settings",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "You can change the hardware settings by pressing the UP or DOWN keys",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "to select an entry. Then press the ENTER key to select alternative",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "settings.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "When all settings are correct, select \"Accept these device settings\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "and press ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUARepairPageEntries[] =
{
    {
        6,
        8,
        "ReactOS Setup is in an early development phase. It does not yet",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "support all the functions of a fully usable setup application.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "The repair functions are not implemented yet.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Press U for Updating OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Press R for the Recovery Console.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Press ESC to return to the main page.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ENTER to reboot your computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Main page  ENTER = Reboot",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY ukUAComputerPageEntries[] =
{
    {
        6,
        8,
        "You want to change the type of computer to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired computer type.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the computer type.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFlushPageEntries[] =
{
    {
        10,
        6,
        "The system is now making sure all data is stored on your disk",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "This may take a minute",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "When finished, your computer will reboot automatically",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Flushing cache",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAQuitPageEntries[] =
{
    {
        10,
        6,
        "ReactOS is not completely installed",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Remove floppy disk from Drive A: and",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "all CD-ROMs from CD-Drives.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Press ENTER to reboot your computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Please wait ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADisplayPageEntries[] =
{
    {
        6,
        8,
        "You want to change the type of display to be installed.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Press the UP or DOWN key to select the desired display type.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   the display type.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUASuccessPageEntries[] =
{
    {
        10,
        6,
        "The basic components of ReactOS have been installed successfully.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Remove floppy disk from Drive A: and",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "all CD-ROMs from CD-Drive.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Press ENTER to reboot your computer.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reboot computer",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUABootPageEntries[] =
{
    {
        6,
        8,
        "Setup cannot install the bootloader on your computers",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "hardisk",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Please insert a formatted floppy disk in drive A: and",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "press ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

MUI_PAGE ukUAPages[] =
{
    {
       START_PAGE,
       ukUAWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        ukUAIntroPageEntries
    },
    {
        LICENSE_PAGE,
        ukUALicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        ukUADevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        ukUARepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        ukUAComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        ukUADisplayPageEntries
    },
    {
        FLUSH_PAGE,
        ukUAFlushPageEntries
    },
    {
        QUIT_PAGE,
        ukUAQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        ukUASuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        ukUABootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif
