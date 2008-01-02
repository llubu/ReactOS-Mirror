#ifndef LANG_EL_GR_H__
#define LANG_EL_GR_H__

static MUI_ENTRY elGRLanguagePageEntries[] =
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
        "Language Selection.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
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
        "\x07  This Language will be the default language for the final system.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue  F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRWelcomePageEntries[] =
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
        "����� ������� ���� ����������� ��� ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "���� �� ����� ��� ������������ ���������� �� ����������� ������� ReactOS ����",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "���������� ��� ��� ������������ �� ������� ����� ��� ������������.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  ������� ENTER ��� �� ������������� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  ������� R ��� �� ������������� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  ������� L ��� �� ����� ���� ����� ������������ ��� ��� ������������ ��� �� ReactOS",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  ������� F3 ��� �� ����������� ����� �� ������������� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "��� ������������ ����������� ��� �� ReactOS, ����������� ������������ ��:",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = ��������  R = ����������� F3 = ���������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRIntroPageEntries[] =
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
        "� ����������� ��� ReactOS ����� �� ������ ���� ���������������. ��� ����������� �����",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "���� ��� ���������������� ���� ������� ������������ ������������.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "�� �������� ����������� �������:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- � ����������� �� ������ �� ������������ �� ���� ��� ��� primary partition ��� �����.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- � ����������� �� ������ �� ��������� ��� primary partition ��� ��� �����",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  ������ �������� extended partitions ��� ����� ����.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- � ���������� �� ������ �� ��������� �� ����� extended partition ���� ������",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  ������ �������� �� ���� extended partitions ��� ����� ����.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- � ����������� ����������� ���� FAT ��������� �������.",
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
        "\x07  ������� ENTER ��� �� ������������� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  ������� F3 ��� �� ����������� ����� �� ������������� �� ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ��������   F3 = ���������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
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

static MUI_ENTRY elGRDevicePageEntries[] =
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
        "� �������� ����� ������� ��� ��������� ��������.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       �����������:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        ��������:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       ������������:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "������� �������������:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         �������:",
        TEXT_NORMAL
    },
    {
        25,
        16, "������� ����� ��� ��������� ��������",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "�������� �� �������� ��� ��������� ������ �������� �� ������� UP � DOWN",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "��� �� ��������� ��� �������. ���� ������� �� ������� ENTER ��� �� ��������� �����",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "���������.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "���� ���� �� ��������� ����� ������, �������� \"������� ����� ��� ��������� ��������\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "��� ������� ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ��������   F3 = ���������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRRepairPageEntries[] =
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
        "� ����������� ��� ReactOS ��������� �� ������ ������ ���������. ��� ����������� �����",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "���� ��� ���������������� ���� ������� ������������ ������������.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "� ����������� ������������ ��� ����� ���������� �����.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  ������� U ��� �������� ��� OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  ������� R ��� �� Recovery Console.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  ������� ESC ��� �� ����������� ���� ����� ������.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  ������� ENTER ��� �� �������������� ��� ����������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = ����� ������  ENTER = ������������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY elGRComputerPageEntries[] =
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
        "������ �� �������� ��� ���� ��� ���������� ��� �� ������������.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  ������� �� ������� UP � DOWN ��� �� ��������� ��� ��������� ���� ����������.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   ���� ������� ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  ������� �� ������� ESC ��� �� ����������� ���� ����������� ������ ����� �� ��������",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ��� ���� ����������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ��������   ESC = �������   F3 = ���������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "�� ������� ������������ ���� ��� ��� �� �������� ������������� ��� �����",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "���� ���� ����� ���� ���",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "���� �����������, � ����������� ��� �� ������������� ��������",
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

static MUI_ENTRY elGRQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "�� ReactOS ��� ������������� ������",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "��������� �� ������� ��� �� A: ���",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "��� �� CD-ROMs  ��� �� CD-Drives.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "������� ENTER ��� �� �������������� ��� ����������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   �������� ���������� ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRDisplayPageEntries[] =
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
        "������ �� �������� ��� ���� ��� ��������� ��� �� ������������.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  ������� �� ������� UP � DOWN ��� �� ��������� ��� ��������� ���� ���������.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   ���� ������� ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  ������� �� ������� ESC ��� �� ����������� ���� ����������� ������ ����� �� ��������",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ��� ���� ���������.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ��������   ESC = �������   F3 = ���������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "�� ������ �������� ��� ReactOS �������������� ��������.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "��������� �� ������� ��� �� A: ���",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "��� �� CD-ROMs ��� �� CD-Drive.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "������� ENTER ��� �� �������������� ��� ���������� ���.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = ������������ ����������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootPageEntries[] =
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
        "� ����������� �� ������ �� ������������ ��� bootloader ��� ������ �����",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "��� ���������� ���",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "�������� �������� ��� ������������ ������� ��� A: ���",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "������� ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = ��������   F3 = ���������",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY elGRSelectPartitionEntries[] =
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
        "The list below shows existing partitions and unused disk",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRFormatPartitionEntries[] =
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
        "Format partition",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
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
        TEXT_NORMAL
    }
};

static MUI_ENTRY elGRInstallDirectoryEntries[] =
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
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "be installed.",
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

static MUI_ENTRY elGRFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRBootLoaderEntries[] =
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
        "Setup is installing the boot loader",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
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

static MUI_ENTRY elGRKeyboardSettingsEntries[] =
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
        "You want to change the type of keyboard to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
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
        "   the keyboard type.",
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

static MUI_ENTRY elGRLayoutSettingsEntries[] =
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
        "You want to change the keyboard layout to be installed.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
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
        "   the keyboard layout.",
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
    },

};

static MUI_ENTRY elGRPrepareCopyEntries[] =
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
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY elGRSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Select a file system from the list below.",
        0
    },
    {
        8,
        19,
        "\x07  Press UP or DOWN to select a file system.",
        0
    },
    {
        8,
        21,
        "\x07  Press ENTER to format the partition.",
        0
    },
    {
        8,
        23,
        "\x07  Press ESC to select another partition.",
        0
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

static MUI_ENTRY elGRDeletePartitionEntries[] =
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
        "You have chosen to delete the partition",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY elGRRegistryEntries[] =
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
        "Setup is updating the system configuration. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Creating registry hives...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR elGRErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "�� ReactOS ��� ������������� ������ ����\n"
	     "���������� ���. �� ����������� ��� ��� ����������� ����, �� ������ ��\n"
	     "����������� ��� ����������� ��� �� ������������ �� ReactOS.\n"
	     "\n"
	     "  \x07  ������� ENTER ��� �� ���������� ��� �����������.\n"
	     "  \x07  ������� F3 ��� �� ����������� ��� ��� �����������.",
	     "F3= ���������  ENTER = ��������"
    },
    {
        //ERROR_NO_HDD
        "� ����������� �� ������� �� ���� ������� ������ �����.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "� ����������� �� ������� �� �������� �� ������ TXTSETUP.SIF.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "� ���������� ����� ��� ������������ ������ TXTSETUP.SIF.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "� ����������� ����� ��� �� ������ �������� ��� TXTSETUP.SIF.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "� ����������� �� ������� �� �������� ��� ����������� ��� ������ ����������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "� ����������� �� ������� �� �������� �� ����� ����� ����������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "� ����������� �� ������� �� �������� �� ����� ����� ���������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "� ����������� �� ������� �� �������� �� ����� ����� �������������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "� ����������� �� ������� �� �������� �� ����� ��������� �������������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_WARN_PARTITION,
          "� ����������� ����� ��� ����������� ���� ������� ������ �������� ��� �� �������\n"
		  "partition table ��� �� ������ �� �������� �����!\n"
		  "\n"
		  "� ���������� � �������� partitions ������ �� ����������� �� partiton table.\n"
		  "\n"
		  "  \x07  ������� F3 ��� �� ����������� ��� ��� �����������."
		  "  \x07  ������� ENTER ��� �� ����������.",
          "F3= ���������  ENTER = ��������"
    },
    {
        //ERROR_NEW_PARTITION,
        "�� �������� �� ������������� ��� Partition ���� ��\n"
		"��� ���� ������� Partition!\n"
		"\n"
		"  * ������� ����������� ������� ��� �� ����������.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "�� �������� �� ���������� ���� �� ������������ ���� ������!\n"
        "\n"
        "  * ������� ����������� ������� ��� �� ����������.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_NO_FLOPPY,
        "��� ������� ������� ��� A:.",
        "ENTER = ��������"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "� ���������� ������� �� ��������� ��� ��������� ��� �� ������� �������������.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "� ����������� ������� �� ��������� ��� ��������� ������� ��� ��� ��������.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_IMPORT_HIVE,
        "� ����������� ������� �� �������� ��� hive ������.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_FIND_REGISTRY
        "� ���������� ������� �� ���� �� ������ ��������� ��� �������.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CREATE_HIVE,
        "� ����������� ������� �� ������������ �� registry hives.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "�� cabinet ��� ���� ������ ������ inf.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CABINET_MISSING,
        "�� cabinet �� �������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "�� cabinet ��� ���� ������ ������ ������������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_COPY_QUEUE,
        "� ����������� ������� �� ������� ��� ���� ������� ���� ���������.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CREATE_DIR,
        "� ����������� �� ������� �� ������������ ���� ���������� ������������.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "� ����������� ������� �� ���� ��� ����� 'Directories'\n"
        "��� TXTSETUP.SIF.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CABINET_SECTION,
        "� ����������� ������� �� ���� ��� ����� 'Directories'\n"
        "��� cabinet.\n",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "� ����������� �� ������� �� ������������ ��� �������� ������������.",
        "ENTER = ������������ ����������"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "� ����������� ������� �� ���� ��� ����� 'SetupData'\n"
		 "��� TXTSETUP.SIF.\n",
		 "ENTER = ������������ ����������"
    },
    {
        //ERROR_WRITE_PTABLE,
        "� ����������� ������� �� ������ �� partition tables.\n"
        "ENTER = ������������ ����������"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE elGRPages[] =
{
    {
        LANGUAGE_PAGE,
        elGRLanguagePageEntries
    },
    {
       START_PAGE,
       elGRWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        elGRIntroPageEntries
    },
    {
        LICENSE_PAGE,
        elGRLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        elGRDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        elGRRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        elGRComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        elGRDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        elGRFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        elGRSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        elGRSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        elGRFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        elGRDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        elGRInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        elGRPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        elGRFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        elGRKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        elGRBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        elGRLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        elGRQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        elGRSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        elGRBootPageEntries
    },
    {
        REGISTRY_PAGE,
        elGRRegistryEntries
    },
    {
        -1,
        NULL
    }
};

#endif
