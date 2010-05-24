#pragma once

MUI_LAYOUTS svSELayouts[] =
{
    { L"041D", L"0000041D" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY svSELanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Language Selection.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Please choose the language used for the installation process.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  This Language will be the default language for the final system.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue  F3 = Quit",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "V�lkommen till ReactOS Setup!",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Denna del av installationen kopierar ReactOS till eran",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "dator och f�rbereder den andra delen av installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck p� ENTER f�r att installera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck p� R f�r att reparera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck p� L f�r att l�sa licensavtalet till ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck p� F3 f�r att avbryta installationen av ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "F�r mer information om ReactOS, bes�k:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "   ENTER = Forts�tt  R = Reparera F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Setup �r i en tidig utvecklingsfas och saknar d�rf�r ett antal",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan f�rv�ntas av ett fullt anv�ndbart setup-program.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "F�ljande begr�nsningar g�ller:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- Setup kan ej hantera mer �n 1 prim�r partition per h�rddisk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Setup kan ej radera en prim�r partition fr�n en h�rddisk",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  om ut�kade partitioner existerar p� h�rddisken.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Setup kan ej radera den f�rsta ut�kade partitionen fr�n en h�rddisk",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  om andra ut�kade partitioner existerar p� h�rddisken.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- Setup st�der endast filsystem av typen FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- Kontrollering av h�rddiskens filsystem st�ds (�nnu) ej.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  Tryck p� ENTER f�r att installera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  Tryck p� F3 f�r att avbryta installationen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licensering:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS �r licenserad under GNU GPL med delar",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "av den medf�ljande koden licenserad under GPL-f�renliga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenser s�som X11-, BSD- och GNU LGPL-licenserna.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "All mjukvara som �r del av ReactOS �r publicerad",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "under GNU GPL, men �ven den ursprungliga",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "licensen �r uppr�tth�llen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Denna mjukvara har INGEN GARANTI eller begr�nsing p� anv�ndning",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "bortsett fr�n till�mplig lokal och internationell lag. Licenseringen av",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "ReactOS t�cker endast distrubering till tredje part.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Om Ni av n�gon anledning ej f�tt en kopia av",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License med ReactOS, bes�k",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        22,
        "Garanti:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "Detta �r gratis mjukvara; se k�llkoden f�r restriktioner ang�ende kopiering.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "INGEN GARANTI ges; inte ens f�r S�LJBARHET eller PASSANDE F�R ETT",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "SPECIELLT SYFTE. ALL ANV�NDNING SKER P� EGEN RISK!",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = �terv�nd",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Listan nedanf�r visar inst�llningarna f�r maskinvaran.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "       Dator:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "        Bildsk�rm:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "       Tangentbord:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Tangentbordslayout:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "         Acceptera:",
        TEXT_STYLE_NORMAL
    },
    {
        25,
        16, "Acceptera dessa maskinvaruinst�llningar",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "�ndra inst�llningarna genom att trycka p� UPP- och NED-piltangenterna",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "f�r att markera en inst�llning, och tryck p� ENTER f�r att v�lja",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "inst�llningen.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "N�r alla inst�llningar �r korrekta, v�lj \"Acceptera dessa maskinvaruinst�llningar\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "och tryck p� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "ReactOS Setup �r i en tidig utvecklingsfas och saknar d�rf�r ett antal",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "funktioner som kan f�rv�ntas av ett fullt anv�ndbart setup-program.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "Reparations- och uppdateringsfunktionerna fungerar ej.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Tryck p� U f�r att uppdatera ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Tryck p� R f�r �terst�llningskonsolen.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Tryck p� ESC f�r att �terv�nda till f�reg�ende sida.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Tryck p� ENTER f�r att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ESC = G� till f�reg�ende sida  ENTER = Starta om datorn",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "�ndra vilken typ av dator som ska installeras.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Anv�nd UPP- och NED-piltangenterna f�r att v�lja �nskad datortyp.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sen p� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck p� ESC f�r att �terv�nda till den f�reg�ende sidan utan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   att �ndra datortypen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   ESC = �terv�nd   F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Datorn f�rs�krar sig om att all data �r lagrad p� h�rdisken.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Detta kommer att ta en stund.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "N�r detta �r f�rdigt kommer datorn att startas om automatiskt.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Rensar cachen",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Installationen av ReactOS har inte slutf�rts.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-l�sare A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor fr�n CD/DVD-l�sarna.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryck p� ENTER f�r att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Var god v�nta ...",
        TEXT_TYPE_STATUS,
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "�ndra vilken typ av bildsk�rmsinst�llning som ska installeras.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Anv�nd UPP- och NED-piltangenterna f�r att v�lja �nskad inst�llning.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Tryck sedan p� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Tryck p� ESC f�r att �terv�nda till den f�reg�ende sidan utan",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   att �ndra bildsk�rmsinst�llningen.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   ESC = �terv�nd   F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS har nu installerats p� datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Se till att ingen floppy-disk finns i floppy-l�sare A:",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "och tag ur alla skivor fr�n CD/DVD-l�sarna.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Tryck p� ENTER f�r att starta om datorn.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Starta om datorn",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup misslyckades med att installera bootloadern p� datorns",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "h�rddisk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Var god s�tt in en formatterad floppy-disk i l�sare A: och",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "tryck p� ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Forts�tt   F3 = Avbryt",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "The list below shows existing partitions and unused disk",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "space for new partitions.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Press UP or DOWN to select a list entry.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press ENTER to install ReactOS onto the selected partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Press C to create a new partition.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Press D to delete an existing partition.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Please wait...",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Format partition",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Setup will now format the partition. Press ENTER to continue.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY svSEInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installs ReactOS files onto the selected partition. Choose a",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "directory where you want ReactOS to be installed:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "To change the suggested directory, press BACKSPACE to delete",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "characters and then type the directory where you want ReactOS to",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Please wait while ReactOS Setup copies files to your ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        30,
        13,
        "installation folder.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "This may take several minutes to complete.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Please wait...    ",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup is installing the boot loader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Install bootloader on the harddisk (MBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Install bootloader on a floppy disk.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Skip install bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   F3 = Quit",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You want to change the type of keyboard to be installed.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard type.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the keyboard type.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Please select a layout to be installed by default.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Press the UP or DOWN key to select the desired keyboard",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    layout. Then press ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Press the ESC key to return to the previous page without changing",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   the keyboard layout.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continue   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup prepares your computer for copying the ReactOS files. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Building the file copy list...",
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
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
        TEXT_TYPE_STATUS
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
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "You have chosen to delete the partition",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Press D to delete the partition.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "WARNING: All data on this partition will be lost!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Press ESC to cancel.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Delete Partition   ESC = Cancel   F3 = Quit",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY svSERegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup is updating the system configuration. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Creating registry hives...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR svSEErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS is not completely installed on your\n"
        "computer. If you quit Setup now, you will need to\n"
        "run Setup again to install ReactOS.\n"
        "\n"
        "  \x07  Press ENTER to continue Setup.\n"
        "  \x07  Press F3 to quit Setup.",
        "F3= Quit  ENTER = Continue"
    },
    {
        //ERROR_NO_HDD
        "Setup could not find a harddisk.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup could not find its source drive.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup failed to load the file TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup found a corrupt TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup could not retrieve system drive information.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup failed to load the computer type list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup failed to load the display settings list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup failed to load the keyboard type list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup failed to load the keyboard layout list.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup found that at least one harddisk contains an incompatible\n"
        "partition table that can not be handled properly!\n"
        "\n"
        "Creating or deleting partitions can destroy the partiton table.\n"
        "\n"
        "  \x07  Press F3 to quit Setup."
        "  \x07  Press ENTER to continue.",
        "F3= Quit  ENTER = Continue"
    },
    {
        //ERROR_NEW_PARTITION,
        "You can not create a new Partition inside\n"
        "of an already existing Partition!\n"
        "\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "You can not delete unpartitioned disk space!\n"
        "\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_NO_FLOPPY,
        "No disk in drive A:.",
        "ENTER = Continue"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup failed to update keyboard layout settings.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup could not create install directories.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup could not create the install directory.",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup failed to find the 'SetupData' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Reboot computer"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup failed to write partition tables.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Setup failed to add keyboard layouts to registry.\n"
        "ENTER = Reboot computer"
    },
    {
        //ERROR_INSUFFICIENT_DISKSPACE,
        "Not enough free space in the selected partition.\n"
        "  * Press any key to continue.",
        NULL
    },
    {
        //ERROR_UPDATE_GEOID,
        "Setup could not set the geo id.\n"
        "ENTER = Reboot computer"
    },
    {
        NULL,
        NULL
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
        REGISTRY_PAGE,
        svSERegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING svSEStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Please wait..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Install   C = Create Partition   F3 = Quit"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Install   D = Delete Partition   F3 = Quit"},
    {STRING_PARTITIONSIZE,
     "Size of new partition:"},
    {STRING_CHOOSENEWPARTITION,
     "You have chosen to create a new partition on"},
    {STRING_HDDSIZE,
    "Please enter the size of the new partition in megabytes."},
    {STRING_CREATEPARTITION,
     "   ENTER = Create Partition   ESC = Cancel   F3 = Quit"},
    {STRING_PARTFORMAT,
    "This Partition will be formatted next."},
    {STRING_NONFORMATTEDPART,
    "You chose to install ReactOS on a new or unformatted Partition."},
    {STRING_INSTALLONPART,
    "Setup install ReactOS onto Partition"},
    {STRING_CHECKINGPART,
    "Setup is now checking the selected partition."},
    {STRING_QUITCONTINUE,
    "F3= Quit  ENTER = Continue"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = Reboot computer"},
    {STRING_TXTSETUPFAILED,
    "Setup failed to find the '%S' section\nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "   Copying file: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup is copying files..."},
    {STRING_REGHIVEUPDATE,
    "   Updating registry hives..."},
    {STRING_IMPORTFILE,
    "   Importing %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Updating display registry settings..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Updating locale settings..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Updating keyboard layout settings..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Adding codepage information to registry..."},
    {STRING_DONE,
    "   Done..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = Reboot computer"},
    {STRING_CONSOLEFAIL1,
    "Unable to open the console\n\n"},
    {STRING_CONSOLEFAIL2,
    "The most common cause of this is using an USB keyboard\n"},
    {STRING_CONSOLEFAIL3,
    "USB keyboards are not fully supported yet\n"},
    {STRING_FORMATTINGDISK,
    "Setup is formatting your disk"},
    {STRING_CHECKINGDISK,
    "Setup is checking your disk"},
    {STRING_FORMATDISK1,
    " Format partition as %S file system (quick format) "},
    {STRING_FORMATDISK2,
    " Format partition as %S file system "},
    {STRING_KEEPFORMAT,
    " Keep current file system (no changes) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "on %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "on Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Type %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup created a new partition on"},
    {STRING_UNPSPACE,
    "    Unpartitioned space              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_UNFORMATTED,
    "New (Unformatted)"},
    {STRING_FORMATUNUSED,
    "Unused"},
    {STRING_FORMATUNKNOWN,
    "Unknown"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Adding keyboard layouts"},
    {0, 0}
};
