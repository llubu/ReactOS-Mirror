/* TRANSLATOR:  M�rio Ka�m�r /Mario Kacmar/ aka Kario (kario@szm.sk)
 * DATE OF TR:  22-01-2008
 * Encoding  :  Latin II (852)
 */

#ifndef LANG_SK_SK_H__
#define LANG_SK_SK_H__

static MUI_ENTRY skSKLanguagePageEntries[] =
{
    {
        4,
        3,
         " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "V�ber jazyka.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Zvo�te si jazyk, ktor� sa pou�ije po�as in�tal�cie.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stla�te ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Tento jazyk bude predvolen�m jazykom nain�talovan�ho syst�mu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKWelcomePageEntries[] =
{
    {
        4,
        3,
         " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "V�ta V�s In�tal�tor syst�mu ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Tento stupe� In�tal�tora skop�ruje opera�n� syst�m ReactOS na V��",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "po��ta� a priprav� druh� stupe� In�tal�tora.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Stla�te ENTER pre nain�talovanie syst�mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Stla�te R pre opravu syst�mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Stla�te L, ak chcete zobrazi� licen�n� podmienky syst�mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Stla�te F3 pre skon�enie in�tal�cie bez nain�talovania syst�mu ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Pre viac inform�ci� o syst�me ReactOS, nav�t�vte pros�m:",
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
        "   ENTER = Pokra�ova�   R = Opravi�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKIntroPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "In�tal�tor syst�mu ReactOS je v za�iato�nom �t�diu v�voja. Zatia�",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "nepodporuje v�etky funkcie plne vyu��vaj�ce program In�tal�tor.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "M� nasleduj�ce obmedzenia:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- In�tal�tor nepracuje s viac ako 1 prim�rnou oblas�ou na 1 disku.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- In�tal�tor nevie odstr�ni� prim�rnu oblas� z disku,",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  pokia� existuj� roz�ren� oblasti na disku.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- In�tal�tor nevie odstr�ni� prv� roz�ren� oblas� z disku,",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  pokia� existuj� in� roz�ren� oblasti na disku.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- In�tal�tor podporuje iba s�borov� syst�m FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- Kontrola s�borov�ho syst�mu zatia� nie je implementovan�.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Stla�te ENTER pre nain�talovanie syst�mu ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Stla�te F3 pre skon�enie in�tal�cie bez nain�talovania syst�mu ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKLicensePageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        6,
        "Licencia:",
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
        "Z�ruka:",
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
        "   ENTER = N�vrat",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKDevicePageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Zoznam ni��ie, zobrazuje s��asn� nastavenia zariaden�.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "        Po��ta�:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "        Monitor:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "     Kl�vesnica:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Rozlo�enie kl.:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "     Akceptova�:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Akceptova� tieto nastavenia zariaden�",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "M��ete zmeni� hardv�rov� nastavenia stla�en�m kl�vesov HORE alebo DOLE",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "pre v�ber polo�ky. Potom stla�te kl�ves ENTER pre v�ber alternat�vnych",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "nastaven�.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Ak s� v�etky nastavenia spr�vne, vyberte polo�ku",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "\"Akceptova� tieto nastavenia zariaden�\" a stla�te ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKRepairPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "In�tal�tor syst�mu ReactOS je v za�iato�nom �t�diu v�voja. Zatia�",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "nepodporuje v�etky funkcie plne vyu��vaj�ce program In�tal�tor.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Funkcie na opravu syst�mu zatia� nie s� implementovan�.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Stla�te U pre aktualiz�ciu OS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Stla�te R pre z�chrann� konzolu.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Stla�te ESC pre n�vrat na hlavn� str�nku.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Stla�te ENTER pre re�tart po��ta�a.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ESC = Hlavn� str�nka  ENTER = Re�tart",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY skSKComputerPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeni� typ po��ta�a, ktor� m� by� nain�talovan�.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Stla�te kl�ves HORE alebo DOLE pre v�ber po�adovan�ho typu po��ta�a.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stla�te ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Stla�te kl�ves ESC pre n�vrat na predch�dzaj�cu str�nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu po��ta�a.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   ESC = Zru�i�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKFlushPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Syst�m pr�ve overuje v�etky ulo�en� �daje na Va�om disku",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "To m��e trva� nieko�ko min�t",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Ke� skon��, po��ta� sa automaticky re�tartuje",
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

static MUI_ENTRY skSKQuitPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Syst�m ReactOS nie je nain�talovan� kompletne",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "v�etky m�di� CD-ROM z CD mechan�k.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Stla�te ENTER pre re�tart po��ta�a.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Po�kajte, pros�m ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKDisplayPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeni� typ monitora, ktor� m� by� nain�talovan�.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Stla�te kl�ves HORE alebo DOLE pre v�ber po�adovan�ho typu monitora.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stla�te ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Stla�te kl�ves ESC pre n�vrat na predch�dzaj�cu str�nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu monitora.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   ESC = Zru�i�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKSuccessPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Z�kladn� s��ast� syst�mu ReactOS boli �spe�ne nain�talovan�.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Vyberte disketu z mechaniky A: a",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "v�etky m�di� CD-ROM z CD mechan�k.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Stla�te ENTER pre re�tart po��ta�a.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Re�tart po��ta�a",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKBootPageEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "In�tal�tor nem��e nain�talova� zav�dza� syst�mu na pevn� disk V��ho", //bootloader = zav�dza� syst�mu
        TEXT_NORMAL
    },
    {
        6,
        9,
        "po��ta�a",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Vlo�te pros�m, naform�tovan� disketu do mechaniky A:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "a stla�te ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY skSKSelectPartitionEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
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
        "\x07  Stla�te HORE alebo DOLE pre v�ber zo zoznamu polo�iek.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Stla�te ENTER to install ReactOS onto the selected partition.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Stla�te C pre vytvorenie novej oblasti.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Stla�te D pre vymazanie existuj�cej oblasti.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Po�kajte, pros�m ...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKFormatPartitionEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Form�tovanie oblasti",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "In�tal�tor teraz naform�tuje oblas�. Stla�te ENTER pre pokra�ovanie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY skSKInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "In�tal�tor nain�taluje s�bory syst�mu ReactOS na zvolen� oblas�.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "Vyberte adres�r kam chcete nain�talova� syst�m ReactOS:",
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
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKFileCopyEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Po�kajte, pros�m, k�m In�tal�tor skop�ruje s�bory do in�tala�n�ho",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "prie�inka pre ReactOS.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "Dokon�enie m��e trva� nieko�ko min�t.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                     \xB3 Po�kajte, pros�m ...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKBootLoaderEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "In�tal�tor in�taluje zav�dza� opera�n�ho syst�mu",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Nain�talova� zav�dza� syst�mu na pevn� disk (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Nain�talova� zav�dza� syst�mu na disketu.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Presko�i� in�tal�ciu zav�dza�a syst�mu.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeni� typ kl�vesnice, ktor� m� by� nain�talovan�.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Stla�te kl�ves HORE alebo DOLE a vyberte po�adovan� typ kl�vesnice.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Potom stla�te ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Stla�te kl�ves ESC pre n�vrat na predch�dzaj�cu str�nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   typu kl�vesnice.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   ESC = Zru�i�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Chcete zmeni� rozlo�enie kl�vesnice, ktor� m� by� nain�talovan�.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Stla�te kl�ves HORE alebo DOLE pre v�ber po�adovan�ho rozlo�enia",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   kl�vesnice. Potom stla�te ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Stla�te kl�ves ESC pre n�vrat na predch�dzaj�cu str�nku bez zmeny",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   rozlo�enia kl�vesnice.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   ESC = Zru�i�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY skSKPrepareCopyEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Pripravuje sa kop�rovanie s�borov syst�mu ReactOS. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vytv�ra sa zoznam potrebn�ch s�borov ...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY skSKSelectFSEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Vyberte syst�m s�borov zo zoznamu uveden�ho ni��ie.",
        0
    },
    {
        8,
        19,
        "\x07  Stla�te HORE alebo DOLE pre v�ber syst�mu s�borov.",
        0
    },
    {
        8,
        21,
        "\x07  Stla�te ENTER pre form�tovanie oblasti.",
        0
    },
    {
        8,
        23,
        "\x07  Stla�te ESC pre v�ber inej oblasti.",
        0
    },
    {
        0,
        0,
        "   ENTER = Pokra�ova�   ESC = Zru�i�   F3 = Skon�i�",
        TEXT_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKDeletePartitionEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Vybrali Ste si odstr�nenie oblasti",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Stla�te D pre odstr�nenie oblasti.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "UPOZORNENIE: V�etky �daje na tejto oblasti sa nen�vratne stratia!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Stla�te ESC pre zru�enie.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Odstr�ni� oblas�   ESC = Zru�i�   F3 = Skon�i�",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY skSKRegistryEntries[] =
{
    {
        4,
        3,
        " In�tal�tor syst�mu ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Aktualizuj� sa syst�mov� nastavenia.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Vytv�raj� sa polo�ky registrov ...", //registry hives
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR skSKErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "Syst�m ReactOS nie je kompletne nain�talovan� na V��om\n"
        "po��ta�i. Ak teraz preru�te in�tal�ciu, budete musie�\n"
        "spusti� In�tal�tor znova, aby sa syst�m ReactOS nain�taloval.\n"
        "\n"
        "  \x07  Stla�te ENTER pre pokra�ovanie v in�tal�cii.\n"
        "  \x07  Stla�te F3 pre skon�enie in�tal�cie.",
        "F3 = Skon�i�  ENTER = Pokra�ova�"
    },
    {
        //ERROR_NO_HDD
        "In�tal�toru sa nepodarilo n�js� pevn� disk.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "In�tal�toru sa nepodarilo n�js� jej zdrojov� mechaniku.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "In�tal�tor zlyhal pri nahr�van� s�boru TXTSETUP.SIF.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "In�tal�tor na�iel po�koden� s�bor TXTSETUP.SIF.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup found an invalid signature in TXTSETUP.SIF.\n", //chybn� (neplatn�) podpis (znak, zna�ka, �ifra)
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup could not retrieve system drive information.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_WRITE_BOOT,
        "Setup failed to install FAT bootcode on the system partition.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "In�tal�tor zlyhal pri nahr�van� zoznamu typov po��ta�ov.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "In�tal�tor zlyhal pri nahr�van� zoznamu nastaven� monitora.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "In�tal�tor zlyhal pri nahr�van� zoznamu typov kl�vesn�c.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "In�tal�tor zlyhal pri nahr�van� zoznamu rozlo�enia kl�vesn�c.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_WARN_PARTITION,
          "Setup found that at least one harddisk contains an incompatible\n"
          "partition table that can not be handled properly!\n"
          "\n"
          "Vytvorenie alebo odstr�nenie oblast� m��e zni�i� tabu�ku oblast�.\n"
          "\n"
          "  \x07  Stla�te F3 pre skon�enie in�tal�cie."
          "  \x07  Stla�te ENTER pre pokra�ovanie.",
          "F3 = Skon�i�  ENTER = Pokra�ova�"
    },
    {
        //ERROR_NEW_PARTITION,
        "Nem��ete vytvori� nov� oblas�\n"
        "vo vn�tri u� existuj�cej oblasti!\n"
        "\n"
        "  * Pokra�ujte stla�en�m �ubovo�n�ho kl�vesu.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Nem��ete odstr�ni� miesto na disku, ktor� nie je oblas�ou!\n"
        "\n"
        "  * Pokra�ujte stla�en�m �ubovo�n�ho kl�vesu.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Setup failed to install the FAT bootcode on the system partition.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_NO_FLOPPY,
        "V mechanike A: nie je disketa.",
        "ENTER = Pokra�ova�"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup failed to update keyboard layout settings.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup failed to update display registry settings.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup failed to import a hive file.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup failed to find the registry data files.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup failed to create the registry hives.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup failed to set the initialize the registry.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet has no valid inf file.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet not found.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet has no setup script.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup failed to open the copy file queue.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup could not create install directories.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in TXTSETUP.SIF.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup failed to find the 'Directories' section\n"
        "in the cabinet.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup could not create the install directory.",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "In�tal�tor zlyhal pri h�adan� sekcie 'SetupData'\n"
        "v s�bore TXTSETUP.SIF.\n",
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_WRITE_PTABLE,
        "In�tal�tor zlyhal pri z�pise do tabuliek oblast�.\n"
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup failed to add codepage to registry.\n"
        "ENTER = Re�tart po��ta�a"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup could not set the system locale.\n"
        "ENTER = Re�tart po��ta�a"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE skSKPages[] =
{
    {
        LANGUAGE_PAGE,
        skSKLanguagePageEntries
    },
    {
        START_PAGE,
        skSKWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        skSKIntroPageEntries
    },
    {
        LICENSE_PAGE,
        skSKLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        skSKDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        skSKRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        skSKComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        skSKDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        skSKFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        skSKSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        skSKSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        skSKFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        skSKDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        skSKInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        skSKPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        skSKFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        skSKKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        skSKBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        skSKLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        skSKQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        skSKSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        skSKBootPageEntries
    },
    {
        REGISTRY_PAGE,
        skSKRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING skSKStrings[] =
{
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = In�talova�   C = Vytvori� oblas�   F3 = Skon�i�"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = In�talova�   D = Odstr�ni� oblas�   F3 = Skon�i�"},
    {STRING_PARTITIONSIZE,
     "Ve�kos� novej oblasti:"},
    {STRING_PLEASEWAIT,
     "   Po�kajte, pros�m ..."},
    {STRING_CHOOSENEWPARTITION,
     "Zvolili Ste vytvorenie novej oblasti na"},
    {STRING_CREATEPARTITION,
     "   ENTER = Vytvori� oblas�   ESC = Zru�i�   F3 = Skon�i�"},
    {STRING_COPYING,
     "                                              \xB3 Kop�ruje sa s�bor: %S"},
    {STRING_SETUPCOPYINGFILES,
     "In�tal�tor kop�ruje s�bory..."},
    {STRING_PAGEDMEM,
     "Str�nkovan� pam��"},
    {STRING_NONPAGEDMEM,
     "Nestr�nkovan� pam��"},
    {STRING_FREEMEM,
     "Vo�n� pam��"},
    {0, 0}
};

#endif
