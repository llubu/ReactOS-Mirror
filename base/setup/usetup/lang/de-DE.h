#ifndef LANG_DE_DE_H__
#define LANG_DE_DE_H__

static MUI_ENTRY deDELanguagePageEntries[] =
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
        "Sprachauswahl.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Bitte w�hlen Sie die Sprache, die Sie w�hrend des Setups verwenden wollen.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Dann dr�cken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Diese Sprache wird sp�ter als Standardsprache im System verwendet.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen  F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEWelcomePageEntries[] =
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
        "Willkommen zum ReactOS Setup",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Dieser Teil des Setups kopiert das ReactOS Betriebssystem auf Ihren",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Computer und bereitet den zweiten Teil des Setups vor.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Dr�cken Sie ENTER, um ReactOS zu installieren.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Dr�cken Sie R, um ReactOS zu reparieren.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Dr�cken Sie L, um die Lizenzabkommen von ReactOS zu lesen.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Dr�cken Sie F3, um das Setup zu beenden.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "F�r weitere Informationen, besuchen Sie bitte:",
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
        "   ENTER = Fortsetzen  R = Reparieren F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " Setup ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Das ReactOS Setup ist noch in einer fr�hen Entwicklungsphase. Es unter-",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "st�tzt noch nicht alle Funktionen eines vollst�ndig nutzbaren Setups.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Es gibt folgende Beschr�nkungen:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- Setup kann nur eine prim�re Partition auf einer HDD verwalten.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- Setup kann keine prim�re Partition von einer HDD l�schen",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  so lange erweiterte Partitionen auf dieser HDD existieren.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- Setup kann die erste erweiterte Partition nicht von der HDD l�schen",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  so lange weitere erweiterte Partitionen auf dieser HDD existieren.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- Setup unterst�tzt nur FAT Dateisysteme.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Dateisystem�berpr�fung ist noch nicht implementiert.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Dr�cken Sie ENTER, um ReactOS zu installieren.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Dr�cken Sie F3, um das Setup zu beenden.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDELicensePageEntries[] =
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
        "Lizenz:",
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
        "   ENTER = Zur�ck",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEDevicePageEntries[] =
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
        "Die untere Liste zeigt die derzeitigen Ger�teeinstellungen.",
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
        "     Bildschirm:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "       Tastatur:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Tastaturlayout:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "    Akzeptieren:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Diese Ger�teeinstellungen akzeptieren",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Sie k�nnen die Einstellungen durch die Pfeiltasten ausw�hlen.",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "Dann dr�cken Sie die Eingabetaste, um eine Einstellung abzu�ndern.",
        TEXT_NORMAL
    },
    {
        6,
        21,
        " ",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Wenn alle Einstellungen korrekt sind, w�hlen Sie \"Diese Ger�te-",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "einstellungen akzeptieren\" und dr�cken danach die Eingabetaste.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDERepairPageEntries[] =
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
        "Das ReactOS Setup ist noch in einer fr�hen Entwicklungsphase. Es unter-",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "st�tzt noch nicht alle Funktionen eines vollst�ndig nutzbaren Setups.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Die Reparaturfunktionen sind noch nicht implementiert.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Dr�cken Sie U, um ReactOS zu aktualisieren.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Dr�cken Sie R, f�r die Wiederherstellungskonsole.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Dr�cken Sie ESC, um zur Hauptseite zur�ckzukehren.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Dr�cken Sie ENTER, um den Computer neuzustarten.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Hauptseite  ENTER = Neustarten",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY deDEComputerPageEntries[] =
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
        "Sie wollen den Computertyp �ndern, der installiert wird.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Dr�cken Sie die HOCH- oder RUNTER-Taste, um den gew�nschten",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Typ zu w�hlen. Dann dr�cken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr�cken Sie ESC, um zur vorherigen Seite zur�ckzukehren,",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ohne den Computertyp zu �ndern.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEFlushPageEntries[] =
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
        "Das System vergewissert sich, dass alle Daten gespeichert sind.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Dies kann einige Minuten in Anspruch nehmen.",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Der PC wird automatisch neustarten, wenn der Vorgang beendet ist.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Cache wird geleert",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEQuitPageEntries[] =
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
        "ReactOS wurde nicht vollst�ndig installiert",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Entfernen Sie die Diskette aus Laufwerk A: und",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "alle CD-ROMs aus den CD-Laufwerken.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Dr�cken Sie ENTER, um den Computer neuzustarten.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Bitte warten ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEDisplayPageEntries[] =
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
        "Sie wollen den Bildschirmtyp �ndern, der installiert wird.",
        TEXT_NORMAL
    },
    {   8,
        10,
        "\x07  Dr�cken Sie die HOCH- oder RUNTER-Taste, um den gew�nschten",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Typ zu w�hlen. Dann dr�cken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr�cken Sie ESC, um zur vorherigen Seite zur�ckzukehren, ohne",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   den Bildschirmtyp zu �ndern.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDESuccessPageEntries[] =
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
        "Die Standardkomponenten von ReactOS wurden erfolgreich installiert.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Entfernen Sie die Diskette aus Laufwerk A: und",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "alle CD-ROMs aus den CD-Laufwerken.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Dr�cken Sie ENTER, um den Computer neuzustarten.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Computer neustarten",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEBootPageEntries[] =
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
        "Das Setup kann das Boot-Sektor nicht auf der",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Festplatte Ihres Computers installieren",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Bitte legen Sie eine formatierte Diskette in Laufwerk A: ein und",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "dr�cken Sie ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Fortsetzen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY deDESelectPartitionEntries[] =
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
        "Diese Liste zeigt existierende Partitionen an und den freien",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Speicherplatz f�r neue Partitionen.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Dr�cken Sie die Pfeiltasten, um eine Partition auszuw�hlen.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr�cken Sie die Eingabetaste, um die Auswahl zu best�tigen.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Dr�cken Sie C, um eine neue Partition zu erstellen.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Dr�cken Sie D, um eine vorhandene Partition zu l�schen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Bitte warten...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEFormatPartitionEntries[] =
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
        "Formatiere Partition",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "Setup wird nun die gew�nschte Partition formatieren.",
        TEXT_NORMAL
    },
    {
        6,
        11,
        "Dr�cken Sie die Eingabetaste, um fortzufahren.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortfahren   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY deDEInstallDirectoryEntries[] =
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
        "Setup installiert die ReactOS Installationsdateien in die ausgew�hlte",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Partition. W�hlen Sie ein Installationsverzeichnis f�r ReactOS:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "Um den Vorschlag zu �ndern dr�cken sie die 'Entf' Taste um",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "Zeichen zu l�schen und gegeben sie dann den Namen des Verzeichnis ein",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortfahren   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEFileCopyEntries[] =
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
        "Bitte warten Sie w�hrend ReactOS Setup die ReactOS Dateien",
        TEXT_NORMAL
    },
    {
        11,
        13,
        "in das Installationsverzeichnis kopiert.",
        TEXT_NORMAL
    },
    {
        11,
        14,
        "Dieser Vorgang kann mehrere Minuten in Anspruch nehmen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Bitte warten...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEBootLoaderEntries[] =
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
        "Setup installiert nun den Boot-Sektor.",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Boot-Sektor im MBR installieren.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Boot-Sektor auf einer Diskette installieren.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Boot-Sektor nicht installieren.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortfahren   F3 = Abbrechen",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEKeyboardSettingsEntries[] =
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
        "Sie wollen den Tastaturtyp �ndern, der installiert wird.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Dr�cken Sie die HOCH- oder RUNTER-Taste, um den gew�nschten",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Typ zu w�hlen. Dann dr�cken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr�cken Sie ESC, um zur vorherigen Seite zur�ckzukehren,",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ohne den Tastaturtyp zu �ndern.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortfahren   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDELayoutSettingsEntries[] =
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
        "Sie wollen das Tastaturlayout �ndern, der installiert wird.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Dr�cken Sie die HOCH- oder RUNTER-Taste, um den gew�nschten",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Typ zu w�hlen. Dann dr�cken Sie ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Dr�cken Sie ESC, um zur vorherigen Seite zur�ckzukehren,",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   ohne das Tastaturlayout zu �ndern.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Fortfahren   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY deDEPrepareCopyEntries[] =
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
        "Setup bereitet ihren Computer f�r die Installation vor.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Erstelle Liste der zu kopierenden Dateien...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY deDESelectFSEntries[] =
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
        "W�hlen Sie ein Dateisystem von der folgenden Liste.",
        0
    },
    {
        8,
        19,
        "\x07  Dr�cken Sie die Pfeiltasten, um das Dateisystem zu �ndern.",
        0
    },
    {
        8,
        21,
        "\x07  Dr�cken Sie die Eingabetaste, um die Partition zu formatieren.",
        0
    },
    {
        8,
        23,
        "\x07  Dr�cken Sie ESC, um eine andere Partition auszuw�hlen.",
        0
    },
    {
        0,
        0,
        "   ENTER = Fortfahren   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDEDeletePartitionEntries[] =
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
        "Sie haben sich entschieden diese Partition zu l�schen",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Dr�cken Sie D, um die Partition zu l�schen.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "Warnung: Alle Daten auf dieser Partition werden gel�scht!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  ESC um abzubrechen.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = L�sche Partition   ESC = Abbrechen   F3 = Beenden",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY deDERegistryEntries[] =
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

MUI_ERROR deDEErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS ist nicht vollst�ndig auf Ihrem System installiert.\n"
        "Wenn Sie das Setup jetzt beenden, m�ssen Sie das\n"
        "Setup erneut starten, um ROS zu installieren.\n"
        "\n"
        "  \x07  Dr�cken Sie ENTER um das Setup Fortzusetzen.\n"
        "  \x07  Dr�cken Sie F3 um das Setup zu beenden.",
        "F3 = Beenden  ENTER = Fortsetzen"
    },
    {
        //ERROR_NO_HDD
        "Setup konnte keine Festplatte finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup konnte das Quelllaufwerk nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup konnte TXTSETUP.SIF nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup fand eine korrupte TXTSETUP.SIF.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup fand eine ung�ltige Signatur in TXTSETUP.SIF.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup konnte keine Laufwerksinformationen abfragen.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup konnte den FAT Bootcode nicht auf der Partition installieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup konnte die Computertypenliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup konnte die Displayeinstellungsliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup konnte die Tastaturtypenliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup konnte die Tastaturlayoutliste nicht laden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup hat mindestens eine Festplatte mit einer inkompatiblen Partitionstabelle\n"
        "welche nicht richtig verwendet werden k�nnen!\n"
        "\n"
        "Partitionen zu erstellen/l�schen kann die Partitionstabelle zerst�ren.\n"
        "\n"
        "  \x07  Dr�cken Sie F3, um das Setup zu beenden."
        "  \x07  Dr�cken Sie ENTER, um das Setup Fortzusetzen.",
        "F3 = Beenden  ENTER = Fortsetzen"
    },
    {
        //ERROR_NEW_PARTITION,
        "Sie k�nnen keine neue Partition in einer bereits\n"
        "vohandenen Partition erstellen!\n"
        "\n"
        "  * * Eine beliebige Taste zum Fortsetzen dr�cken.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Sie k�nnen unpartitionieren Speicher nicht l�schen!\n"
        "\n"
        "  * Eine beliebige Taste zum Fortsetzen dr�cken.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup konnte den FAT Bootcode nicht auf der Partition installieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_NO_FLOPPY,
        "Keine Diskette in Laufwerk A:.",
        "ENTER = Fortsetzen"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup konnte das Tastaturlayout nicht aktualisieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup konnte die Display-Registrywerte nicht aktualisieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup konnte keine Hive Datei importieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup konnte die Registrydateien nicht finden.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup konnte die Registry-Hives nicht erstellen.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup konnte die Registry nicht initialisieren.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet hat keine g�ltige .inf Datei.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet nicht gefunden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet enth�lt kein Setup Skript.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup konnte die Liste mit zu kopierenden Dateien nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup konnte die Installationspfade nicht erstellen.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup konnte die 'Ordner' Sektion in\n"
        "TXTSETUP.SIF nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup konnte die 'Ordner' Sektion im\n"
        "Cabinet nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup konnte den Installationspfad nicht erstellen.",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup konnte die 'SetupData' Sektion in\n"
        "TXTSETUP.SIF nicht finden.\n",
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup konnte die Partitionstabellen nicht schreiben.\n"
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup konnte den CodePage-Eintrag nicht hinzuf�gen.\n"
        "ENTER = Computer neustarten"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup konnte die Systemsprache nicht einstellen.\n"
        "ENTER = Computer neustarten"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE deDEPages[] =
{
    {
        LANGUAGE_PAGE,
        deDELanguagePageEntries
    },
    {
        START_PAGE,
        deDEWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        deDEIntroPageEntries
    },
    {
        LICENSE_PAGE,
        deDELicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        deDEDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        deDERepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        deDEComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        deDEDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        deDEFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        deDESelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        deDESelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        deDEFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        deDEDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        deDEInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        deDEPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        deDEFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        deDEKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        deDEBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        deDELayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        deDEQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        deDESuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        deDEBootPageEntries
    },
    {
        REGISTRY_PAGE,
        deDERegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING deDEStrings[] =
{
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = Installieren   C = Partition erstellen  F3 = Beenden"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = Installieren   D = Partition l�schen    F3 = Beenden"},
    {STRING_PARTITIONSIZE,
     "Gr��e der neuen Partition:"},
    {STRING_PLEASEWAIT,
     "   Bitte warten..."},
    {STRING_CHOOSENEWPARTITION,
     "Sie haben beschlossen eine neue Partition zu erstellen auf"},
    {STRING_CREATEPARTITION,
     "   ENTER = Partition erstelln   ESC = Abbruch   F3 = Beenden"},
    {STRING_COPYING,
     "                                                   \xB3 Kopiere Datei: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Setup kopiert Dateien..."},
    {STRING_PAGEDMEM,
     "Paged Memory"},
    {STRING_NONPAGEDMEM,
     "Nonpaged Memory"},
    {STRING_FREEMEM,
     "Free Memory"},
    {0, 0}
};

#endif
