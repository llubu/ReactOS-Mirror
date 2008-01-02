#ifndef LANG_ES_ES_H__
#define LANG_ES_ES_H__

static MUI_ENTRY esESLanguagePageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Selecci�n de idioma",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Por favor, seleccione el idioma a utilizar durante la instalaci�n.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  El idioma seleccionado ser� tambi�n el idioma por defacto del sistema.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar  R = Reparar F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESWelcomePageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Bienvenido a la instalaci�n de ReactOS",
        TEXT_HIGHLIGHT
    },
    {
        6,
        11,
        "Esta parte de la instalaci�n copia ReactOS en su equipo y",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "prepara la segunda parte de la instalaci�n.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione ENTER para instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione R para reparar ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Presione L para ver las condiciones y t�rminos de licencia",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione F3 para salir sin instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Para m�s informaci�n sobre ReactOS, visite por favor:",
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
        "   ENTER = Continuar  R = Reparar F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESIntroPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El instalador de ReactOS se encuentra en una etapa preliminar.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "A�n no posee todas las funciones de un instalador.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Se presentan las siguientes limitaciones:",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "- El instalador no soporta m�s de una partici�n primaria por disco.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "- El instalador no puede eliminar una partici�n primaria de un disco",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "  si hay particiones extendidas en el mismo.",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "- El instalador no puede eliminar la primer partici�n extendida",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "  si existen otras particiones extendidas en el disco.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "- El instalador soporta solamente el sistema de archivos FAT.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "- El comprobador de integridad del sistema de archivos no est� a�n implementado.",
        TEXT_NORMAL
    },
    {
        8,
        23,
        "\x07  Presione ENTER para instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "\x07  Presione F3 para salir sin instalar ReactOS.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESLicensePageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
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
        "ReactOS obedece los terminos de la licencia",
        TEXT_NORMAL
    },
    {
        8,
        9,
        "GNU GPL con partes que contienen c�digo de otras",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "licencias compatibles como la X11 o BSD y la GNU LGPL.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Todo el software que forma parte del sistema ReactOS est�",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "por tanto liberado bajo licencia GNU GPL as� como manteniendo",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "la licencia original.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "Este software viene SIN GARANTIA o restricciones de uso",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "excepto leyes locales o internacionales aplicables. La licencia",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "de ReactOS cubre solo la distribuci�n a terceras partes.",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "Si por alg�n motivo no recibi� una copia de la",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "GNU General Public License con ReactOS por favor visite",
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
        "Garant�a:",
        TEXT_HIGHLIGHT
    },
    {
        8,
        24,
        "Este es software libre; vea el c�digo para las condiciones de copia.",
        TEXT_NORMAL
    },
    {
        8,
        25,
        "NO existe garant�a; ni siquiera para MERCANTIBILIDAD",
        TEXT_NORMAL
    },
    {
        8,
        26,
        "o el cumplimiento de alg�n prop�sito particular",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Regresar",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESDevicePageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "La lista inferior muestra la configuraci�n del dispositivo actual.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "        Equipo:",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "       Pantalla:",
        TEXT_NORMAL,
    },
    {
        8,
        13,
        "        Teclado:",
        TEXT_NORMAL
    },
    {
        8,
        14,
        " Disp. Teclado:",
        TEXT_NORMAL
    },
    {
        8,
        16,
        "        Aceptar:",
        TEXT_NORMAL
    },
    {
        25,
        16, "Aceptar la configuraci�n de los dispositivos",
        TEXT_NORMAL
    },
    {
        6,
        19,
        "Puede modificar la configuraci�n con las teclas ARRIBA y ABAJO",
        TEXT_NORMAL
    },
    {
        6,
        20,
        "para elegir. Luego presione ENTER para cambiar a una configuraci�n",
        TEXT_NORMAL
    },
    {
        6,
        21,
        "alternativa.",
        TEXT_NORMAL
    },
    {
        6,
        23,
        "Cuando la configuraci�n sea correcta, elija \"Aceptar la configuraci�n",
        TEXT_NORMAL
    },
    {
        6,
        24,
        "de los dispostivos\" y presione ENTER.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESRepairPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El instalador de ReactOS se encuentra en una etapa preliminar.",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "A�n no posee todas las funciones de un instalador.",
        TEXT_NORMAL
    },
    {
        6,
        12,
        "Las funciones de reparaci�n no han sido a�n implementadas.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione U para actualizar el SO.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione R para la consola de recuperaci�n.",
        TEXT_NORMAL
    },
    {
        8,
        19,
        "\x07  Presione ESC para volver al men� principal.",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione ENTER para reiniciar su computadora.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "ESC = Men� inicial ENTER = Reiniciar",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};
static MUI_ENTRY esESComputerPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Desea modificar el tipo de equipo a instalar.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione las teclas ARRIBA y ABAJO para elegir el tipo.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione ESC para volver a la p�gina anterior sin cambiar",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "el tipo de equipo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESFlushPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "El sistema se est� asegurando que todos los datos sean salvados",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Esto puede tardar un minuto",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "Cuando haya terminado, su equipo se reiniciar� autom�ticamente",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "Vaciando el cache",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESQuitPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS no ha sido instalado completamente",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Quite el diskette de la unidad A: y",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "todos los CD-ROMs de la unidades.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Presione ENTER para reiniciar su equipo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Espere por favor ...",
        TEXT_STATUS,
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESDisplayPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Desea modificar el tipo de pantalla a instalar.",
        TEXT_NORMAL
    },
    {   8,
        10,
         "\x07  Presione las teclas ARRIBA y ABAJO para modificar el tipo.",
         TEXT_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la p�gina anterior sin",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   modificar el tipo de pantalla.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESSuccessPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        10,
        6,
        "Los componentes b�sicos de ReactOS han sido instalados correctamente.",
        TEXT_NORMAL
    },
    {
        10,
        8,
        "Retire el disco de la unidad A: y",
        TEXT_NORMAL
    },
    {
        10,
        9,
        "todos los CD-ROMs de las unidades.",
        TEXT_NORMAL
    },
    {
        10,
        11,
        "Presione ENTER para reiniciar su equipo.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Reiniciar su equipo",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESBootPageEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El instalador no pudo instalar el cargador de arranque en el disco",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "de su equipo",
        TEXT_NORMAL
    },
    {
        6,
        13,
        "Por favor inserte un disco formateado en la unidad A: y",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "presione ENTER.",
        TEXT_NORMAL,
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY esESSelectPartitionEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "La lista inferior muestra las particiones existentes y el espacio libre",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "en el disco para nuevas particiones.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "\x07  Presione las teclas ARRIBA o ABAJO para seleccionar un elemento de la lista.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione ENTER para instalar ReactOS en la partici�n seleccionada.",
        TEXT_NORMAL
    },
    {
        8,
        15,
        "\x07  Presione C para crear una nueva partici�n.",
        TEXT_NORMAL
    },
    {
        8,
        17,
        "\x07  Presione D para borrar una partici�n existente.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Espere por favor ...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESFormatPartitionEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Formato de la partici�n",
        TEXT_NORMAL
    },
    {
        6,
        10,
        "El instalador formatear� la partici�n. Presione ENTER para continuar.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        TEXT_NORMAL
    }
};

static MUI_ENTRY esESInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El programa instalar� los archivos de ReactOS en la partici�n seleccionada. Seleccione un",
        TEXT_NORMAL
    },
    {
        6,
        9,
        "directorio donde quiere que sea instalado ReactOS:",
        TEXT_NORMAL
    },
    {
        6,
        14,
        "Para cambiar el directorio sugerido, presione RETROCESO para borrar",
        TEXT_NORMAL
    },
    {
        6,
        15,
        "caracteres y escriba el directorio donde desea que ReactOS",
        TEXT_NORMAL
    },
    {
        6,
        16,
        "sea instalado.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESFileCopyEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        11,
        12,
        "Por favor espere mientras el Instalador de ReactOS copia archivos en su ",
        TEXT_NORMAL
    },
    {
        30,
        13,
        "carpeta de instalaci�n de ReactOS.",
        TEXT_NORMAL
    },
    {
        20,
        14,
        "Puede durar varios minutos.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "                                                           \xB3 Espere por favor...    ",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESBootLoaderEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El programam est� instalando el cargador de arranque",
        TEXT_NORMAL
    },
    {
        8,
        12,
        "Instalar cargador de arranque en el disco duro (MBR).",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "Instalar cargador de inicio en un disquete.",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "Omitir la instalaci�n del cargador de arranque.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Desea cambiar el tipo de teclado instalado.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione las teclas ARRIBA o ABAJO para seleccionar el tipo de teclado.",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "   Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la p�gina anterior sin cambiar",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   el tipo de teclado.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Desea cambiar la disposici�n del teclado a instalar.",
        TEXT_NORMAL
    },
    {
        8,
        10,
        "\x07  Presione las teclas ARRIBA o ABAJO para select the la disposici�n del teclado",
        TEXT_NORMAL
    },
    {
        8,
        11,
        "    deseada. Luego presione ENTER.",
        TEXT_NORMAL
    },
    {
        8,
        13,
        "\x07  Presione la tecla ESC para volver a la p�gina anterior sin cambiar",
        TEXT_NORMAL
    },
    {
        8,
        14,
        "   la disposici�n del teclado.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY esESPrepareCopyEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "El programa prepara su computadora para copiar los archivos de ReactOS. ",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   Creando la lista de archivos a copiar...",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY esESSelectFSEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        17,
        "Seleccione un sistema de archivos de la lista inferior.",
        0
    },
    {
        8,
        19,
        "\x07  Presione las teclas ARRIBA o ABAJO para seleccionar el sistema de archivos.",
        0
    },
    {
        8,
        21,
        "\x07  Presione ENTER para formatear partici�n.",
        0
    },
    {
        8,
        23,
        "\x07  Presione ESC para seleccionar otra partici�n.",
        0
    },
    {
        0,
        0,
        "   ENTER = Continuar   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESDeletePartitionEntries[] =
{
    {
        4,
        3,
        " Instalaci�n de ReactOS " KERNEL_VERSION_STR " ",
        TEXT_UNDERLINE
    },
    {
        6,
        8,
        "Ha elegido borrar la partici�n",
        TEXT_NORMAL
    },
    {
        8,
        18,
        "\x07  Presione D para borrar la partici�n.",
        TEXT_NORMAL
    },
    {
        11,
        19,
        "ADVERTENCIA: �Se perder�n todos los datos de esta partici�n!",
        TEXT_NORMAL
    },
    {
        8,
        21,
        "\x07  Presione ESC para cancelar.",
        TEXT_NORMAL
    },
    {
        0,
        0,
        "   D = Borrar Partici�n   ESC = Cancelar   F3 = Salir",
        TEXT_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY esESRegistryEntries[] =
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

MUI_ERROR esESErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS is not completely installed on your\n"
        "computer. If you quit Setup now, you will need to\n"
        "run Setup again to install ReactOS.\n"
        "\n"
        "  \x07  Presione ENTER para continuar el Setup.\n"
        "  \x07  Presione F3 para abandonar el Setup.",
        "F3 = Salir  ENTER = Continuar"
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
        NULL,
        NULL
    }
};

MUI_PAGE esESPages[] =
{
    {
        LANGUAGE_PAGE,
        esESLanguagePageEntries
    },
    {
        START_PAGE,
        esESWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        esESIntroPageEntries
    },
    {
        LICENSE_PAGE,
        esESLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        esESDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        esESRepairPageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        esESComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        esESDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        esESFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        esESSelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        esESSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        esESFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        esESDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        esESInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        esESPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        esESFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        esESKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        esESBootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        esESLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        esESQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        esESSuccessPageEntries
    },
    {
        BOOT_LOADER_FLOPPY_PAGE,
        esESBootPageEntries
    },
    {
        REGISTRY_PAGE,
        esESRegistryEntries
    },
    {
        -1,
        NULL
    }
};

#endif




