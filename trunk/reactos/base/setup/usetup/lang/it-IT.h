#ifndef LANG_IT_IT_H__
#define LANG_IT_IT_H__

static MUI_ENTRY itITLanguagePageEntries[] =
{
    {
        4,
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Selezione della lingua.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Scegliere la lingua da usare durante l'installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Poi premere invio.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Questa lingua sar� quella predefinita per il sistema finale.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua  F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITWelcomePageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Benvenuto all'installazione di ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "Questa parte dell'installazione copia ReactOS nel vostro computer",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "e prepara la seconda parte dell'installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Premere INVIO per installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Premere R per riparare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        19,
        "\x07  Premere L per vedere i termini e condizioni della licenza",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        23,
        "Per maggiori informazioni riguardo ReactOS, visitate il sito:",
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
        "   INVIO = Continua  R = Ripara F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITIntroPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6, 
        8, 
        "Il setup di ReactOS � ancora in una fase preliminare.",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        9,
        "Non ha ancora tutte le funzioni di installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        12,
        "Ci sono delle limitazioni:",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        13,
        "- Il setup non gestisce pi� di una partizione primaria per disco.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- Il setup non pu� eliminare una partizione primaria",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  se ci sono partizioni estese nel disco fisso.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- Il setup non pu� eliminare la prima partizione estesa",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  se ci sono altre partizioni estese nel disco fisso.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        18, 
        "- Il setup supporta solamente il sistema FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        23, 
        "\x07  Premere INVIO per installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        25, 
        "\x07  Premere F3 per uscire senza installare ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0, 
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITLicensePageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "Licenza:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS aderisce ai termini di licenza",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL con parti che contengono codice di altre",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "licenze compatibili come la X11 o BSD e la GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "Tutto il software che fa parte del sistema ReactOS viene",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "rilasciato sotto licenza GNU GPL e mantiene anche",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "la licenza originale.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "Questo software viene fornito SENZA GARANZIA o limitazioni d'uso",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "eccetto per leggi locali o internazionali applicabili. La licenza",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "di ReactOS copre solo la distribuzione a terze parti.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "Se per un qualsiasi motivo non avesse ricevuto una copia",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "della licenza GNU GPL con ReactOS, visiti il sito:",
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
        "Garanzia:",
        TEXT_STYLE_HIGHLIGHT
    },
    {           
        8,
        24,
        "Questo software � libero; vedere il codice per le condizioni di copia.",
        TEXT_STYLE_NORMAL
    },
    {           
        8,
        25,
        "NON esiste garanzia; n� di COMMERCIABILIT�",
        TEXT_STYLE_NORMAL
    },
    {           
        8,
        26,
        "o adeguatezza ad un uso particolare",
        TEXT_STYLE_NORMAL
    },
    {           
        0,
        0,
        "   INVIO = Ritorna",
        TEXT_TYPE_STATUS
    },
    {           
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITDevicePageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6, 
        8,
        "La lista inferiore mostra la configurazione della periferica corrente.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "       Computer:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "        Schermo:",
        TEXT_STYLE_NORMAL,
    },
    {
        8,
        13,
        "       Tastiera:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Layout tastiera:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "       Conferma:",
        TEXT_STYLE_NORMAL
    },
    {
        25, 
        16, "Procedere con questa configurazione",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "Pu� scegliere la configurazione con i tasti SU e GI�",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        20, 
        "Premere INVIO per modificare la configurazione alternativa.",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        21,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        23, 
        "Quando le impostazioni saranno corrette, selezionare",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        24,
        "\"Confermare la configurazione delle periferiche\" e premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITRepairPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6, 
        8,
        "Il setup di ReactOS � ancora in una fase preliminare.",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        9,
        "Non ha ancora tutte le funzioni di installazione.",
        TEXT_STYLE_NORMAL
    },
    {
        6, 
        12,
        "Le funzioni di ripristino non sono state ancora implementate.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        15,
        "\x07  Premere U per aggiornare il SO.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        17,
        "\x07  Premere R per la console di ripristino.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        19,
        "\x07  Premere ESC tornare al men� principale.",
        TEXT_STYLE_NORMAL
    },
    {
        8, 
        21,
        "\x07  Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0, 
        0,
        "ESC = Men� iniziale INVIO = Riavvio",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY itITComputerPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desidera modificare il computer da installare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere i tasti SU e GI� per scegliere il tipo.",
        TEXT_STYLE_NORMAL
    },    
    {
        8,
        11,
        "Dopo premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "il tipo di computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITFlushPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "Il sistema si sta accertando che tutti i dati vengano salvati",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Questo potrebbe impiegare un minuto",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "Al termine, il computer verr� riavviato automaticamente",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "Svuotamento della cache in corso",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITQuitPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS non � stato installato completamente",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Rimuovere il disco floppy dall'unit� A: e",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tutti i CD-ROMs dalle unit�.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Attendere prego ...",
        TEXT_TYPE_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITDisplayPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Desidera modificare il tipo di schermo.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  Premere i tasti SU e GI� per modificare il tipo.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Dopo premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere il tasto ESC per tornare alla pagina precedente",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        " senza modificare il tipo di schermo.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITSuccessPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "I componenti base di ReactOS sono stati installati correttamente.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "Rimuovere il disco dall'unit� A: e",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "tutti i CD-ROMs dalle unit�.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "Premere INVIO per riavviare il computer.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Riavvia il computer",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITBootPageEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Il Setup non ha potuto installare il bootloader nel disco",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "del vostro computer",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "Inserire un disco floppy formattato nell'unit� A: e",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "premere INVIO.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY itITSelectPartitionEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "La lista seguente mostra le partizioni esistenti e lo spazio",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "libero per nuove partizioni.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  Premere SU o GIU' per per selezionare un elemento della lista.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere INVIO per installare ReactOS sulla partizione selezionata.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  Premere C per creare una nuova partizione.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  Premere D per cancellare una nuova partizione.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Attendere...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITFormatPartitionEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Formattazione della partizione",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "Setup formattera la partizione. Premere INVIO per continuare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY itITInstallDirectoryEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup installer� i file di ReactOS nella partizione selezionata.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "Scegliere una cartella dove volete che ReactOS verr� installato:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Per modificare la cartella suggerita premere CANC e poi digitate",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "la cartella dove volete che ReactOS sia installato.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        " ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITFileCopyEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        11,
        12,
        "Attendere mentre il setup di ReactOS copia i file nella",
        TEXT_STYLE_NORMAL
    },
    {
        18,
        13,
        "cartella di installazione di ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        20,
        14,
        "Potrebbe richiedere alcuni minuti.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Attendere prego...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITBootLoaderEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup sta installando il bootloader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "Installazione del bootloader sul disco fisso (MBR).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "Installazione del bootloader su un disco floppy.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "Salta l'installazione del bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITKeyboardSettingsEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Volete cambiare il tipo di tastiere da installare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere SU o GIU' per per selezionare il tipo di tastiera desiderato.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   Poi premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare alla pagina precedente senza modificare",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   il tipo di tastiera.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITLayoutSettingsEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Volete cambiare il tipo di layout di tastiera da installare.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  Premere SU o GIU' per per selezionare il tipo di layout di tastiera",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    desiderato. Poi premere INVIO.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  Premere ESC per tornare allapagina precedente senza modificare",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   il layout di tastiera.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY itITPrepareCopyEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Setup prepara il computer per la copia dei file di ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Costruzione dell'elenco dei file da copiare...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY itITSelectFSEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "Scegliere un file system dalla lista seguente.",
        0
    },
    {
        8,
        19,
        "\x07  Premere SU o GIU' per selezionare un filesystem.",
        0
    },
    {
        8,
        21,
        "\x07  Premere INVIO per formattare una partizione.",
        0
    },
    {
        8,
        23,
        "\x07  Premere ESC per selezionare un'altra partizione.",
        0
    },
    {
        0,
        0,
        "   INVIO = Continua   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITDeletePartitionEntries[] =
{
    {
        4, 
        3,
        " Installazione di ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Avete scelto di cancellare la partizione",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  Premere D per cancellare la partizione.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
		"ATTENZIONE: Tutti i dati di questa partizione saranno persi!!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  Premere ESC per annullare.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   D = Cancella la partizione   ESC = Annulla   F3 = Termina",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY itITRegistryEntries[] =
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
        "Setup sta aggiornando la configurazione del sistema. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "   Creazione degli hive del registro in corso...",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR itITErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS non � installato completamente nel vostro\n"
        "computer. Se esce adesso, dovr� eseguire il Setup\n"
        "nuovamente per installare ReactOS.\n"
        "\n"
        "  \x07  Premere INVIO per continuare il setup.\n"
        "  \x07  Premere F3 per uscire.",
        "F3= Uscire INVIO = Continuare"
    },
    {
        //ERROR_NO_HDD
        "Setup non ha trovato un disco fisso.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "Setup non ha trovato l'unit� di origine.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "Setup non ha potuto caricare il file TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "Setup ha trovato un file TXTSETUP.SIF corrotto.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "Setup ha trovato una firma invalida in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "Setup non ha potuto recuperare le informazioni dell'unit� di sistema.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WRITE_BOOT,
        "Impossibile installare il bootcode FAT nella partizione di sistema.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "Setup non ha potuto caricare l'elenco di tipi di computer.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "Setup non ha potuto caricare l'elenco di tipi di schermo.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "Setup non ha potuto caricare l'elenco di tipi di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "Setup non ha potuto caricare l'elenco di tipi di layout di tastiera.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WARN_PARTITION,
        "Setup ha trovato che al meno un disco fisso contiene una tabella delle\n"
        "partizioni incompatibile che non pu� essere gestita correttamente!\n"
        "\n"
        "Il creare o cancellare partizioni pu� distruggere la tabella delle partizioni.\n"
        "\n"
        "  \x07  Premere F3 per uscire dal Setup."
        "  \x07  Premere INVIO per continuare.",
        "F3= Uscire  INVIO = Continuare"
    },
    {
        //ERROR_NEW_PARTITION,
        "Non si pu� creare una nuova partizione all'interno\n"
        "di una partizione gi� esistente!\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "Non si pu� cancellare spazio di disco non partizionato!\n"
        "\n"
        "  * Premere un tasto qualsiasi per continuare.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "Impossibile installare il bootcode FAT nella partizione di sistema.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_NO_FLOPPY,
        "Non c'� un disco nell'unit� A:.",
        "ENTER = Continue"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "Setup non ha potuto aggiornare la configurazione di layout di tastiera.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "Setup non ha potuto aggiornare la configurazione di registro dello schermo.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_IMPORT_HIVE,
        "Setup non ha potuto importare un file hive.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_FIND_REGISTRY
        "Setup non ha potuto trovare i file di dati del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CREATE_HIVE,
        "Setup non ha potuto creare gli hive del registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "Setup non ha potuto inizializzare il registro.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Il Cabinet non ha un file inf valido.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet non trovato.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Il Cabinet non ha uno script di setup.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_COPY_QUEUE,
        "Setup non ha potuto aprire la coda di copia di file.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CREATE_DIR,
        "Setup non ha potuto creare le cartelle d'installazione.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "Setup non ha potuto trovare le sezioni 'Cartelle'\n"
        "in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CABINET_SECTION,
        "Setup non ha potuto trovare le sezioni 'Cartelle'\n"
        "nel cabinet.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "Setup non ha potuto creare la cartella d'installazione.",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "Setup non ha trovato la sezione 'SetupData'\n"
        "in TXTSETUP.SIF.\n",
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_WRITE_PTABLE,
        "Setup non ha potuto scrivere le tabelle delle partizioni.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "Setup non ha potuto aggiungere la codepage al registry.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "Setup non ha potuto impostare la regionalizzazione del sistema.\n"
        "INVIO = Riavviare il computer"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "Impossibile aggiungere le disposizioni di tastiera al registry.\n"
        "INVIO = Riavviare il computer"
    },
    {
        NULL,
        NULL
    }
};


MUI_PAGE itITPages[] =
{
    {
        LANGUAGE_PAGE,
        itITLanguagePageEntries
    },
    {
        START_PAGE,
        itITWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        itITIntroPageEntries
    },
    {
        LICENSE_PAGE,
        itITLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        itITDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        itITRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        itITComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        itITDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        itITFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        itITSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        itITSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        itITFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        itITDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        itITInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        itITPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        itITFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        itITKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        itITBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        itITLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        itITQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        itITSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        itITBootPageEntries
    },
    {
        REGISTRY_PAGE,
        itITRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING itITStrings[] =
{
    {STRING_PLEASEWAIT,
     "   Attendere..."},
    {STRING_INSTALLCREATEPARTITION,
     "   INVIO = Installa   C = Crea Partizione   F3 = Esci"},
    {STRING_INSTALLDELETEPARTITION,
     "   INVIO = Installa   D = Rimuovi Partizione   F3 = Esci"},
    {STRING_PARTITIONSIZE,
     "Dimensione della nuova partizione:"},
    {STRING_CHOOSENEWPARTITION,
     "Avete scelto di creare una nuova partizione su"},
    {STRING_HDDSIZE,
    "Indicare la dimensione della nuova partizione in megabyte."},
    {STRING_CREATEPARTITION,
     "   INVIO = Creare la partizione   ESC = Annulla   F3 = Esci"},
    {STRING_PARTFORMAT,
    "Questa partizione verr� formattata successivamente."},
    {STRING_NONFORMATTEDPART,
    "Avete scelto di installare ReactOS su una partizione nuova o non formattata."},
    {STRING_INSTALLONPART,
    "Setup installer� ReactOS sulla partitione"},
    {STRING_CHECKINGPART,
    "Setup sta controllando la partizione selezionata."},
    {STRING_QUITCONTINUE,
    "F3= Esci  INVIO = Continua"},
    {STRING_REBOOTCOMPUTER,
    "INVIO = Riavvia il computer"},
    {STRING_TXTSETUPFAILED,
    "Setup non ha trovato la sezione '%S' \nin TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "\xB3 Copia di: %S"},
    {STRING_SETUPCOPYINGFILES,
     "Copia dei file in corso..."},
    {STRING_PAGEDMEM,
     "Memoria paginata"},
    {STRING_NONPAGEDMEM,
     "Memoria non paginata"},
    {STRING_FREEMEM,
     "Memoria libera"},
    {STRING_REGHIVEUPDATE,
    "   Aggiornamento del registry hives..."},
    {STRING_IMPORTFILE,
    "   Importazione di %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   Aggiornamento delle impostazioni del display nel registry..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   Aggiornamento delle impostazioni di regionalizzazione..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   Aggiornamento delle impostazioni della tastiera..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   Aggiunta delle informazioni di codepage al registry..."},
    {STRING_DONE,
    "   Fatto..."},
    {STRING_REBOOTCOMPUTER2,
    "   INVIO = Riavvia il computer"},
    {STRING_CONSOLEFAIL1,
    "Impossibile aprire la console\n\n"},
    {STRING_CONSOLEFAIL2,
    "La causa piu' frequente di questo e' l'uso di una tastiera USB\n"},
    {STRING_CONSOLEFAIL3,
    "le tastiere USB non sono ancora supportate per intero\n"},
    {STRING_FORMATTINGDISK,
    "Setup sta formattando il disco"},
    {STRING_CHECKINGDISK,
    "Setup sta controllando il disco"},
    {STRING_FORMATDISK1,
    " Formatta la partizione con file system %S (formattazione rapida) "},
    {STRING_FORMATDISK2,
    " Formatta la partizione con file system %S "},
    {STRING_KEEPFORMAT,
    " Mantieni il file system attuale (nessuna modifica) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) su %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Tipo %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "su %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) su %wZ."},
    {STRING_HDDINFOUNK3,
    "su %I64u %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Tipo %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "su Harddisk %lu (%I64u %s), Port=%hu, Bus=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Tipo %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu) su %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  Harddisk %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "Setup ha creato una nuova partizione su"},
    {STRING_UNPSPACE,
    "    Spazio non partizionato               %6lu %s"},
    {STRING_MAXSIZE,
    "MB (max. %lu MB)"},
    {STRING_UNFORMATTED,
    "Nuova (Non formattata)"},
    {STRING_FORMATUNUSED,
    "Non usata"},
    {STRING_FORMATUNKNOWN,
    "Sconsciuta"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "Aggiunta delle disposizioni di tastiera"},
    {0, 0}
};

#endif
