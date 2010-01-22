/*
 *      translated by Artem Reznikov, Igor Paliychuk, 2010
 *      http://www.reactos.org/uk/
 */ 

#ifndef LANG_UK_UA_H__
#define LANG_UK_UA_H__

MUI_LAYOUTS ukUALayouts[] =
{
    { L"0422", L"00000422" },
    { L"0409", L"00000409" },
    { NULL, NULL }
};

static MUI_ENTRY ukUALanguagePageEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "췢�� ���",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ���-Р�Ơ, 뷢���� ����, �Ơ �禨 ������Ԡ ؊� ��� ������Ш���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   � Ԡ��Ԋ�� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  �� ��� �禨 ������������� �� ��������Ԝ � ������ШԊ� ���Ҋ.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����  F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAWelcomePageEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "Ѡ�Ơ�� ������ �� ��֬�ҷ ������Ш��� ReactOS",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        6,
        11,
        "ՠ ����� ��؊ ������Ш��� 늦�禨���� ��؊����� ReactOS Ԡ ��",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "����'��� � ؊������Ơ �� ���֬� ���� ������Ш���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ՠ��Ԋ�� ENTER �֢ ������� ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ՠ��Ԋ�� R ��� ����Ш��� ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  Ԡ��Ԋ�� L ��� بᨬ�ަ� Њ����Է� ���� ReactOS",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ՠ��Ԋ�� F3 �֢ 뷽�. Ԩ ������М��� ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "��� ���Ҡ��� �����Ԋ�֌ �Ԫ��Ҡ��� ��� ReactOS, ���-Р�Ơ 늦늦���:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "http://www.reactos.org/uk/",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        0,
        0,
        "ENTER = ��֦����  R = 슦����  L = ъ�����  F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� ReactOS �Ԡ�֦����� � ��Ԋ� �堦�� ����֢Ʒ � �� Ԩ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "؊����� �� ���Ƥ�� ����֤���֌ ��֬�ҷ ������Ш���.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "�����Ԋ Ԡ����Ԋ ֢Ҩ����:",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "- ������М�� Ԩ ؊����� ������ Ԋ� ֦�� ب���Է� ���� Ԡ ����.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "- ������М�� Ԩ ��� 뷦�з� ب���Է� ���� � �����",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "  ��Ʒ Ԡ ����� Ԡ��Է� ������Է� ����.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "- ������М�� Ԩ ��� 뷦�з� ب���� ������Է� ���� � �����",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "  ��Ʒ Ԡ ����� ������ ���� ������Ԋ ���з.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "- ������М�� ؊����� з�� ������� ����� FAT.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "- ݨ���Ơ ������֌ ���ҷ �� Ԩ ����렦�Ԡ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        23,
        "\x07  ՠ��Ԋ�� ENTER �֢ ������� ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "\x07  ՠ��Ԋ�� F3 �֢ 뷽�, Ԩ ������М��� ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   F3 = 췽�",
        TEXT_TYPE_STATUS| TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        6,
        "ъ�����:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        8,
        "ReactOS Њ�������� 늦��늦�� �� ����",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        9,
        "GNU GPL. ���� ReactOS Ҋ���� �����Ԩ��, �Ɗ Њ��������",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "� ��Ҋ�Էҷ Њ�����ҷ: X11, BSD, GNU LGPL.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "�� ��֬��Ԩ 󠢨�ب�����, �ƨ �֦��� � ����� ReactOS, �������",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "�i� �i����֜ �i����i��֜ �֦֜ GNU GPL i� �������",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "ب���Է� �i����i�.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "��Ԩ ��֬��Ԩ 󠢨�ب����� ������ކ���� ��� �����i� i ��� ֢Ҩ���",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        16,
        "� ��������i, �� � �i㤨��, �� i �i�Ԡ�֦Է� �����",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "�i����i� ReactOS ������ކ بᨦ��� ��֦���� ���i� ��֢��.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "���� ���� ���-��i ����Է � Ԩ ���Ҡз ���i� �i����֌",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "�i����i��֌ �֦� GNU ���� � ReactOS, �i��i����",
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
        "����劌:",
        TEXT_STYLE_HIGHLIGHT
    },
    {
        8,
        24,
        "�� � �i��Ԩ ��֬��Ԩ 󠢨�ب�����; ���. ����� ��� بᨬ�ަ� ���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        25,
        "ը ������� Ջ�ǋ ����劌; Ԋ �����i� �����׭� ����, Ԋ ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        26,
        "�⸧������i ��� ������ո� �iѩ�",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ���������",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "� �ط��� Է��� ��먦��i �����Ԋ ؠ�Ҩ�� �����֌�.",
        TEXT_STYLE_NORMAL
    },
    {
        24,
        11,
        "����'���:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        12,
        "����:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        13,
        "�Р�i����:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        14,
        "�Р�. ����Р�Ơ:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        24,
        16,
        "�᷽���:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT
    },
    {
        25,
        16, "��������� ���i ؠ�Ҩ�� �����֌�",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        19,
        "� ���� ��iԷ� ؠ�Ҩ�� �����֌� Ԡ��Ơ��� �Р�i�i ���� i �ո�",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        20,
        "��� 뷦iШ��� �ШҨ��� i �Р�i�� ENTER ��� 뷢��� i���� ��i���i�",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        21,
        "ؠ�Ҩ��i�.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        23,
        "��з ��i ؠ�Ҩ�� ����� ��Ԡ���i, 뷢��i�� \"��������� ���i ؠ�Ҩ�� �����֌�\"",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        24,
        "i Ԡ���i�� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� ReactOS �Ԡ�֦����� � ��Ԋ� �堦�� ����֢Ʒ � �� Ԩ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "؊����� �� ���Ƥ�� ����֤���֌ ��֬�ҷ ������Ш���.",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        12,
        "���Ƥ�� 늦���Ш��� �� Ԩ ����렦�Ԋ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ՠ��Ԋ�� U �֢ ����� OS.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ՠ��Ԋ�� R ��� ������ �����Њ 슦���Ш���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        19,
        "\x07  ՠ��Ԋ�� ESC ��� ����Ԩ��� �� ������֌ �����Ʒ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ՠ��Ԋ�� ENTER �֢ ب������� ����'���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ESC = �����Ԡ �����Ơ  U = �����  R = 슦����  ENTER = ݨ�������",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "��� � ���� �ҊԷ� �� ��֬� ����'���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ՠ��Ơ�� �Р��� ��� � �ո� ��� 뷢��� ��� ��֬� ����'���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   � Ԡ��Ԋ�� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ՠ��Ԋ�� ESC ��� ����Ԩ��� �� ��بᨦ��֌ �����Ʒ ��� �ҊԷ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ��� ����'���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   ESC = 슦ҊԷ�   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "���Ҡ ب���ކ �� �� ��Ԋ ����� Ԡ ����",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "�� ��� ���� ��Ɗ��Ơ ��з�",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "݊��� �������� ����'��� �禨 ����Ҡ���� ب��������",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "����� ƨ�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "ReactOS Ԩ ������Ш�� ���Ԋ��",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "��ެԊ�� ������� � ������֦� A: �",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "�� CD-ROM � CD-���֦��.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ՠ��Ԋ�� ENTER �֢ ب������� ����'���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "���-Р�Ơ ���Ơ�� ...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "��� � ���� �ҊԷ� �� �����.",
        TEXT_STYLE_NORMAL
    },
    {   8,
        10,
         "\x07  ՠ��Ơ�� �Р��� ��� � �ո� ��� 뷢��� ���ኢ�֬� ��� ��Ԋ����.",
         TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   � Ԡ��Ԋ�� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ՠ��Ԋ�� ESC ��� ����Ԩ��� �� ��بᨦ��֌ �����Ʒ ��� �ҊԷ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ��� ��Ԋ���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   ESC = 슦ҊԷ�   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        10,
        6,
        "�����Ԋ �����Ԩ�� ReactOS ��з ��؊��� ������ШԊ.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        8,
        "��ެԊ�� ���ƨ�� � ������֦� A: �",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        9,
        "�㊵ CD-ROM � CD-���֦��.",
        TEXT_STYLE_NORMAL
    },
    {
        10,
        11,
        "ՠ��Ԋ�� ENTER �֢ ب������� ����'���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ݨ������� ����'���",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
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
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� Ԩ ��� ������� bootloader Ԡ �����Ʒ� ����",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "��֬� ����'���",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        13,
        "���-Р�Ơ ����� 늦���Ҡ����� ���ƨ�� � ������֦ A: �",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "Ԡ��Ԋ�� ENTER.",
        TEXT_STYLE_NORMAL,
    },
    {
        0,
        0,
        "ENTER = ��֦����   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY ukUASelectPartitionEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "շ��� ��먦�Է� �ط��� ������� ���Њ� � Ԩ����֬� Ҋ��, �� ���Ԡ",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "������ ��� ���з.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "\x07  ՠ��Ơ�� �Р��� ��� � �ո� ��� 뷢��� ������.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ՠ��Ԋ�� ENTER �֢ ������� ReactOS Ԡ 뷢�Է� ����.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        15,
        "\x07  ՠ��Ԋ�� C �֢ ������ ��뷽 ����.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        17,
        "\x07  ՠ��Ԋ�� D �֢ 뷦�з� ������� ����.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "���-Р�Ơ ���Ơ��...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "���Ҡ������ �����",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        10,
        "���� ������М�� 늦���Ҡ�� ����. ՠ��Ԋ�� ENTER ��� ��֦������.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        TEXT_STYLE_NORMAL
    }
};

static MUI_ENTRY ukUAInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� �������� ���з ReactOS Ԡ 뷢�Է� ����. 췢����",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        9,
        "������ኜ, � ��� � ����� ������� ReactOS:",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        14,
        "�֢ �ҊԷ� ������ኜ Ԡ��Ԋ�� BACKSPACE ��� 뷦�Ш���",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        15,
        "����Њ� ؊��� �֬� �먦��� Ԡ��� ������ኌ ���",
        TEXT_STYLE_NORMAL
    },
    {
        6,
        16,
        "������Ш��� ReactOS.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAFileCopyEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        0,
        12,
        "���-Р�Ơ, ���Ơ�� ��Ʒ ������М�� ReactOS ��؊�� ���з ��",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        13,
        "ؠ�Ʒ ���Ԡ�����.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        0,
        14,
        "�� ��� ���� ��Ɗ��Ơ ��з�.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER
    },
    {
        50,
        0,
        "\xB3 ���-Р�Ơ ���Ơ��...    ",
        TEXT_TYPE_STATUS
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUABootLoaderEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� ������М� boot loader",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        12,
        "������� bootloader Ԡ �����Ʒ� ���� (bootsector).",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "������� bootloader Ԡ ���ƨ��.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "ը ������М�� bootloader.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUAKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "��� � ���� �ҊԷ� �� �Р늠���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ՠ��Ơ�� �Р��� ��� � �ո� ��� 뷢��� ���ኢ�֬� ��� �Р늠���.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "   � Ԡ��Ԋ�� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ՠ��Ԋ�� ESC ��� ����Ԩ��� Ԡ ��بᨦԜ ������� ��� �ҊԷ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ��� �Р늠���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   ESC = 슦ҊԷ�   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUALayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "췢���� ����Р���, �Ơ �禨 ������ШԠ �Ơ ��Ԧ���Ԡ.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        10,
        "\x07  ՠ��Ơ�� �Р��� ��� � �ո� ��� 뷢��� ���ኢ�֌ ����Р�Ʒ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        11,
        "    �Р늠��� � Ԡ��Ԋ�� ENTER.",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        13,
        "\x07  ՠ��Ԋ�� ESC ��� ����Ԩ��� Ԡ ��بᨦԜ ������� ��� �ҊԷ",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        14,
        "   ����Р�Ʒ �Р늠���.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "ENTER = ��֦����   ESC = 슦ҊԷ�   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ukUAPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� ���� �� ����'��� ��� ��؊����� ���Њ� ReactOS. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "��Ԩ�� �ط��� ���Њ�...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY ukUASelectFSEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        17,
        "췢���� ������� ����� � �ط��� Է���.",
        0
    },
    {
        8,
        19,
        "\x07  ՠ��Ơ�� �Р��� ��� � �ո� ��� 뷢��� ������֌ ���ҷ.",
        0
    },
    {
        8,
        21,
        "\x07  ՠ��Ԋ�� ENTER �֢ 늦���Ҡ���� ����.",
        0
    },
    {
        8,
        23,
        "\x07  ՠ��Ԋ�� ESC ��� 뷢��� ���֬� �����.",
        0
    },
    {
        0,
        0,
        "ENTER = ��֦����   ESC = 슦ҊԷ�   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },

    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUADeletePartitionEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "� 뷢�з 뷦�Ш��� �����",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        18,
        "\x07  ՠ��Ԋ�� D ��� 뷦�Ш��� �����.",
        TEXT_STYLE_NORMAL
    },
    {
        11,
        19,
        "�졭�: �� ��Ԋ Ԡ ����� ���Њ ����� �����Ԋ!",
        TEXT_STYLE_NORMAL
    },
    {
        8,
        21,
        "\x07  ՠ��Ԋ�� ESC ��� 늦��Է.",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "D = 췦�з� ����   ESC = 슦ҊԷ�   F3 = 췽�",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY ukUARegistryEntries[] =
{
    {
        4,
        3,
        " ������Ш��� ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE
    },
    {
        6,
        8,
        "������М�� ����М� ��Ԫ���ᠤ�� ���ҷ. ",
        TEXT_STYLE_NORMAL
    },
    {
        0,
        0,
        "����᜜ ��������� ᨆ����...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR ukUAErrorEntries[] =
{
    {
        //ERROR_NOT_INSTALLED
        "ReactOS Ԩ ��� ���Ԋ�� ������ШԷ� Ԡ ��\n"
        "����'���. ���� � 뷽��� � ������М��� ���,\n"
        "�� �� �禨 Ԩ֢����� ������ ��֬��� ������Ш���\n"
        "�����, ���� � ����� ������� ReactOS,\n"
        "\n"
        "  \x07  ՠ��Ԋ�� ENTER �֢ ��֦���� ������Ш���.\n"
        "  \x07  ՠ��Ԋ�� F3 ��� 뷵֦� � ������М���.",
        "F3 = 췽�  ENTER = ��֦����"
    },
    {
        //ERROR_NO_HDD
        "ը 릠���� �Ԡ�� �����Ʒ� ����.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_NO_SOURCE_DRIVE
        "ը 릠���� �Ԡ�� ��������Է� ����.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_LOAD_TXTSETUPSIF
        "ը 릠���� ������ ���� TXTSETUP.SIF.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CORRUPT_TXTSETUPSIF
        "���� TXTSETUP.SIF ����֦�Է�.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_SIGNATURE_TXTSETUPSIF,
        "���Ш�� Ԩ�����Է� ؊�ط� � TXTSETUP.SIF.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_DRIVE_INFORMATION
        "ը 릠���� ���Ҡ� ��Ԋ ��� ����Է� ����.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_WRITE_BOOT,
        "ը 릠���� ������� ���������Է� �֦ FAT Ԡ ���Է� ����.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_LOAD_COMPUTER,
        "ը 릠���� ������ �ط��� �؊� ����'���.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_LOAD_DISPLAY,
        "ը 릠���� ������ �ط��� ��Ҋ� �����.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_LOAD_KEYBOARD,
        "ը 릠���� ������ �ط��� �؊� �Р늠���.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_LOAD_KBLAYOUT,
        "ը 릠���� ������ �ط��� ����Р��� �Р늠���.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_WARN_PARTITION,
          "�Ԡ����� �� ҊԊ��� ֦�� �����Ʒ� ����, �� Ҋ���� ����,\n"
          "�Ʒ� Ԩ ؊��������� ReactOS!\n"
          "\n"
          "�������� �� 뷦�Ш��� ���Њ� ��� ������� 堢з�� ���Њ�.\n"
          "\n"
          "  \x07  ՠ��Ԋ�� F3 ��� 뷵֦� � ������М���.\n"
          "  \x07  ՠ��Ԋ�� ENTER �֢ ��֦����.",
          "F3= 췽�  ENTER = ��֦����"
    },
    {
        //ERROR_NEW_PARTITION,
        "� Ԩ ���� ������ ��뷽 ���� Ԡ\n"
        "�� �������� ���Њ!\n"
        "\n"
        "  * ՠ��Ԋ�� ���-��� �Р��� �֢ ��֦����.",
        NULL
    },
    {
        //ERROR_DELETE_SPACE,
        "ը ���Ԡ 뷦�з� Ԩ���Ҋ���� ֢Р��� Ԡ �����!\n"
        "\n"
        "  * ՠ��Ԋ�� ���-��� �Р��� �֢ ��֦����.",
        NULL
    },
    {
        //ERROR_INSTALL_BOOTCODE,
        "ը 릠���� ������� ���������Է� �֦ FAT Ԡ ���Է� ����.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_NO_FLOPPY,
        "슦����� ���ƨ� � ������֦� A:.",
        "ENTER = ��֦����"
    },
    {
        //ERROR_UPDATE_KBSETTINGS,
        "ը 릠���� ����� ؠ�Ҩ�� ����Р�Ʒ �Р늠���.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_UPDATE_DISPLAY_SETTINGS,
        "ը 릠���� ����� ؠ�Ҩ�� ����� � ᨆ���.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_IMPORT_HIVE,
        "ը 릠���� ��������� ���� ����� ᨆ����.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_FIND_REGISTRY
        "ը 릠���� �Ԡ�� ���з ��Է� ᨆ����.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CREATE_HIVE,
        "ը 릠���� ������ ���� ᨆ����.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_INITIALIZE_REGISTRY,
        "ը 릠���� �Ԋ���Њ���� ᨆ���.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_INVALID_CABINET_INF,
        "Cabinet Ҡ� Ԩ�����Է� inf-����.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CABINET_MISSING,
        "Cabinet Ԩ �Ԡ�����.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CABINET_SCRIPT,
        "Cabinet Ԩ Ҡ� ���������֬� 㤨Ԡኜ.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_COPY_QUEUE,
        "ը 릠���� 늦��� ���� ��؊����� ���Њ�.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CREATE_DIR,
        "ը 릠���� ������ ������ኌ ��� ������Ш���.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_TXTSETUP_SECTION,
        "ը 릠���� �Ԡ�� �Ƥ�� 'Directories'\n"
        "� ���Њ TXTSETUP.SIF.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CABINET_SECTION,
        "ը 릠���� �Ԡ�� �Ƥ�� 'Directories'\n"
        "� cabinet.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_CREATE_INSTALL_DIR
        "ը 릠���� ������ ������ኜ ��� ����������.",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_FIND_SETUPDATA,
        "ը 릠���� �Ԡ�� �Ƥ�� 'SetupData'\n"
        "� ���Њ TXTSETUP.SIF.\n",
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_WRITE_PTABLE,
        "ը 릠���� �ط�� 堢з�� ���Њ�.\n"
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_ADDING_CODEPAGE,
        "ը 릠���� �֦�� ؠ�Ҩ�� �֦����� � ᨆ���.\n"
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_UPDATE_LOCALESETTINGS,
        "ը 릠���� ������� ��Ơ�� ���ҷ.\n"
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_ADDING_KBLAYOUTS,
        "ը 릠���� �֦�� ����Р�Ʒ �Р늠��� �� ᨆ����.\n"
        "ENTER = ݨ������� ����'���"
    },
    {
        //ERROR_UPDATE_GEOID,
        "ը 릠���� ������� geo id.\n"
        "ENTER = ݨ������� ����'���"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE ukUAPages[] =
{
    {
        LANGUAGE_PAGE,
        ukUALanguagePageEntries
    },
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
        SELECT_PARTITION_PAGE,
        ukUASelectPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        ukUASelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        ukUAFormatPartitionEntries
    },
    {
        DELETE_PARTITION_PAGE,
        ukUADeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        ukUAInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        ukUAPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        ukUAFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        ukUAKeyboardSettingsEntries
    },
    {
        BOOT_LOADER_PAGE,
        ukUABootLoaderEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        ukUALayoutSettingsEntries
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
        REGISTRY_PAGE,
        ukUARegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING ukUAStrings[] =
{
    {STRING_PLEASEWAIT,
     "   ���-Р�Ơ, ���Ơ��..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = �������   C = ������ ����   F3 = 췽�"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = �������   D = 췦�з� ����   F3 = 췽�"},
    {STRING_PARTITIONSIZE,
     "���Ҋ� ���֬� �����:"},
    {STRING_CHOOSENEWPARTITION,
     "� ����� ������ ��뷽 ���� Ԡ"},
    {STRING_HDDSIZE,
    "���-Р�Ơ, �먦��� ���Ҋ� ���֬� ����� � Ҩ�����堵."},
    {STRING_CREATEPARTITION,
     "   ENTER = ������ ����   ESC = 슦ҊԷ�   F3 = 췽�"},
    {STRING_PARTFORMAT,
    "��� ���� �禨 늦���Ҡ�����."},
    {STRING_NONFORMATTEDPART,
    "� 뷢�з ������Ш��� ReactOS Ԡ ��뷽 ��� Ԩ���Ҡ���Է� ����."},
    {STRING_INSTALLONPART,
    "ReactOS ������М����� Ԡ ����"},
    {STRING_CHECKINGPART,
    "������М�� ب���ކ 뷢�Է� ����."},
    {STRING_QUITCONTINUE,
    "F3= 췽�  ENTER = ��֦����"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = ݨ������� ����'���"},
    {STRING_TXTSETUPFAILED,
    "������М�� Ԩ �Ҋ� �Ԡ�� �Ƥ�� '%S' \n� ���Њ TXTSETUP.SIF.\n"},
    {STRING_COPYING,
     "\xB3 ��؊�����: %S"},
    {STRING_SETUPCOPYINGFILES,
     "������М�� ��؊�� ���з..."},
    {STRING_REGHIVEUPDATE,
    "   ����Ш��� ����� ᨆ����..."},
    {STRING_IMPORTFILE,
    "   ����������� %S..."},
    {STRING_DISPLAYETTINGSUPDATE,
    "   ����Ш��� ؠ�Ҩ��� ����� � ᨆ���..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   ����Ш��� ؠ�Ҩ��� ��ƠЊ..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   ����Ш��� ؠ�Ҩ��� ����Р�Ʒ �Р늠���..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   �֦����� ��Է� ��� �֦��� ������� � ᨆ���..."},
    {STRING_DONE,
    "   ������..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = ݨ������� ����'���"},
    {STRING_CONSOLEFAIL1,
    "ը 릠���� 늦��� �������\n\n"},
    {STRING_CONSOLEFAIL2,
    "ՠ������ �����Ԡ ����Ԡ ��֬� -  ��������� USB �Р늠���\n"},
    {STRING_CONSOLEFAIL3,
    "USB �Р늠��� �� Ԩ ؊��������� ���Ԋ��\n"},
    {STRING_FORMATTINGDISK,
    "������М�� ���Ҡ�� �� ����"},
    {STRING_CHECKINGDISK,
    "������М�� ب���ކ �� ����"},
    {STRING_FORMATDISK1,
    " ���Ҡ���� ���� � �����늽 ���Ҋ %S (�뷦ƨ ���Ҡ������) "},
    {STRING_FORMATDISK2,
    " ���Ҡ���� ���� � �����늽 ���Ҋ %S  "},
    {STRING_KEEPFORMAT,
    " ��з��� ������ ������� ����� (��� �Ҋ�) "},
    {STRING_HDINFOPARTCREATE,
    "%I64u %s  �����Ʒ� ���� %lu  (Port=%hu, Bus=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK1,
    "%I64u %s  �����Ʒ� ���� %lu  (Port=%hu, Bus=%hu, Id=%hu)."},
    {STRING_HDDINFOUNK2,
    "   %c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTDELETE,
    "Ԡ %I64u %s  �����Ʒ� ���� %lu  (����=%hu, ��Ԡ=%hu, Id=%hu) on %wZ."},
    {STRING_HDDINFOUNK3,
    "Ԡ %I64u %s  �����Ʒ� ���� %lu  (����=%hu, ��Ԡ=%hu, Id=%hu)."},
    {STRING_HDINFOPARTZEROED,
    "�����Ʒ� ���� %lu (%I64u %s), ����=%hu, ��Ԡ=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK4,
    "%c%c  Type %lu    %I64u %s"},
    {STRING_HDINFOPARTEXISTS,
    "Ԡ ��������� ����� %lu (%I64u %s), ����=%hu, ��Ԡ=%hu, Id=%hu (%wZ)."},
    {STRING_HDDINFOUNK5,
    "%c%c  Type %-3u                         %6lu %s"},
    {STRING_HDINFOPARTSELECT,
    "%6lu %s  �����Ʒ� ���� %lu  (Port=%hu, Bus=%hu, Id=%hu) on %S"},
    {STRING_HDDINFOUNK6,
    "%6lu %s  �����Ʒ� ���� %lu  (Port=%hu, Bus=%hu, Id=%hu)"},
    {STRING_NEWPARTITION,
    "������М�� ������ ��뷽 ���� Ԡ"},
    {STRING_UNPSPACE,
    "    ը���Ҋ��Ԡ ֢Р���              %6lu %s"},
    {STRING_MAXSIZE,
    "MB (Ҡ��. %lu MB)"},
    {STRING_UNFORMATTED,
    "��뷽 (ը���Ҡ���Է�)"},
    {STRING_FORMATUNUSED,
    "ը ��������"},
    {STRING_FORMATUNKNOWN,
    "ը늦���"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "�֦����� ����Р��� �Р늠���"},
    {0, 0}
};

#endif
