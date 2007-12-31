#ifndef LANG_SV_SE_H__
#define LANG_SV_SE_H__

static MUI_ENTRY svSELanguagePageEntries[] =
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

static MUI_ENTRY svSEWelcomePageEntries[] =
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
        "V�lkommen till ReactOS Setup!",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Denna del av installationen kopierar ReactOS till eran",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "dator och f�rbereder den andra delen av installationen.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck p� ENTER f�r att installera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck p� R f�r att reparera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck p� L f�r att l�sa licensavtalet till ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck p� F3 f�r att avbryta installationen av ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "F�r mer information om ReactOS, bes�k:",
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
        "   ENTER = Forts�tt  R = Reparera F3 = Avbryt",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEIntroPageEntries[] =
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
        "ReactOS Setup �r i en tidig utvecklingsfas och saknar d�rf�r ett antal",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan f�rv�ntas av ett fullt anv�ndbart setup-program.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "F�ljande begr�nsningar g�ller:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Setup kan ej hantera mer �n 1 prim�r partition per h�rddisk.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Setup kan ej radera en prim�r partition fr�n en h�rddisk",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  om ut�kade partitioner existerar p� h�rddisken.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Setup kan ej radera den f�rsta ut�kade partitionen fr�n en h�rddisk",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  om andra ut�kade partitioner existerar p� h�rddisken.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Setup st�der endast filsystem av typen FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Kontrollering av h�rddiskens filsystem st�ds (�nnu) ej.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Tryck p� ENTER f�r att installera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Tryck p� F3 f�r att avbryta installationen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   F3 = Avbryt",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSELicensePageEntries[] =
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
        "Licensering:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS �r licenserad under GNU GPL med delar",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "av den medf�ljande koden licenserad under GPL-f�renliga",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licenser s�som X11-, BSD- och GNU LGPL-licenserna.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "All mjukvara som �r del av ReactOS �r publicerad",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "under GNU GPL, men �ven den ursprungliga",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "licensen �r uppr�tth�llen.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "Denna mjukvara har INGEN GARANTI eller begr�nsing p� anv�ndning",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "bortsett fr�n till�mplig lokal och internationell lag. Licenseringen av",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "ReactOS t�cker endast distrubering till tredje part.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Om Ni av n�gon anledning ej f�tt en kopia av",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License med ReactOS, bes�k",
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
        "Garanti:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "Detta �r gratis mjukvara; se k�llkoden f�r restriktioner ang�ende kopiering.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "INGEN GARANTI ges; inte ens f�r S�LJBARHET eller PASSANDE F�R ETT",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "SPECIELLT SYFTE. ALL ANV�NDNING SKER P� EGEN RISK!",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = �terv�nd",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEDevicePageEntries[] =
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
        "Listan nedanf�r visar inst�llningarna f�r maskinvaran.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "       Dator:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Bildsk�rm:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Tangentbord:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Tangentbordslayout:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "         Acceptera:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Acceptera dessa maskinvaruinst�llningar",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "�ndra inst�llningarna genom att trycka p� UPP- och NED-piltangenterna",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "f�r att markera en inst�llning, och tryck p� ENTER f�r att v�lja",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "inst�llningen.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "N�r alla inst�llningar �r korrekta, v�lj \"Acceptera dessa maskinvaruinst�llningar\"",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "och tryck p� ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   F3 = Avbryt",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSERepairPageEntries[] =
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
        "ReactOS Setup �r i en tidig utvecklingsfas och saknar d�rf�r ett antal",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan f�rv�ntas av ett fullt anv�ndbart setup-program.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Reparations- och uppdateringsfunktionerna fungerar ej.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck p� U f�r att uppdatera ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck p� R f�r �terst�llningskonsolen.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck p� ESC f�r att �terv�nda till f�reg�ende sida.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck p� ENTER f�r att starta om datorn.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = G� till f�reg�ende sida  ENTER = Starta om datorn",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY svSEComputerPageEntries[] =
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
        "�ndra vilken typ av dator som ska installeras.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Anv�nd UPP- och NED-piltangenterna f�r att v�lja �nskad datortyp.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Tryck sen p� ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck p� ESC f�r att �terv�nda till den f�reg�ende sidan utan",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   att �ndra datortypen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   ESC = �terv�nd   F3 = Avbryt",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEFlushPageEntries[] =
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
        "Datorn f�rs�krar sig om att all data �r lagrad p� h�rdisken.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Detta kommer att ta en stund.",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "N�r detta �r f�rdigt kommer datorn att startas om automatiskt.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Rensar cachen",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEQuitPageEntries[] =
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
        "Installationen av ReactOS har inte slutf�rts.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-l�sare A:",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor fr�n CD/DVD-l�sarna.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Tryck p� ENTER f�r att starta om datorn.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Var god v�nta ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEDisplayPageEntries[] =
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
        "�ndra vilken typ av bildsk�rmsinst�llning som ska installeras.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Anv�nd UPP- och NED-piltangenterna f�r att v�lja �nskad inst�llning.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Tryck sedan p� ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck p� ESC f�r att �terv�nda till den f�reg�ende sidan utan",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   att �ndra bildsk�rmsinst�llningen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   ESC = �terv�nd   F3 = Avbryt",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSESuccessPageEntries[] =
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
        "ReactOS har nu installerats p� datorn.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-l�sare A:",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor fr�n CD/DVD-l�sarna.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Tryck p� ENTER f�r att starta om datorn.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Starta om datorn",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSEBootPageEntries[] =
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
        "Setup misslyckades med att installera bootloadern p� datorns",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "h�rddisk",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Var god s�tt in en formatterad floppy-disk i l�sare A: och",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "tryck p� ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   F3 = Avbryt",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY svSESelectPartitionEntries[] =
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

static MUI_ENTRY svSEFormatPartitionEntries[] =
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

static MUI_ENTRY svSEInstallDirectoryEntries[] =
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

static MUI_ENTRY svSEFileCopyEntries[] =
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

static MUI_ENTRY svSEBootLoaderEntries[] =
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

static MUI_ENTRY svSEKeyboardSettingsEntries[] =
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

static MUI_ENTRY svSELayoutSettingsEntries[] =
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

static MUI_ENTRY svSEPrepareCopyEntries[] =
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

static MUI_ENTRY svSESelectFSEntries[] =
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

static MUI_ENTRY svSEDeletePartitionEntries[] =
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


MUI_PAGE svSEPages[] =
{
    {
        LANGUAGE_PAGE,
        svSELanguagePageEntries
    },
    {
       START_PAGE,
       svSEWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        svSEIntroPageEntries
    },
    {
        LICENSE_PAGE,
        svSELicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        svSEDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        svSERepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        svSEComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        svSEDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        svSEFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        svSESelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        svSESelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        svSEFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        svSEDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        svSEInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        svSEPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        svSEFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        svSEKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        svSEBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        svSELayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        svSEQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        svSESuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        svSEBootPageEntries
    },
    {
        -1,
        NULL
    }
};

#endif
