/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/hidparser/api.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "parser.h"

static ULONG KeyboardScanCodes[256] =
{
    0x0000, 0x0000, 0x0000, 0x0000, 0x001e, 0x0030, 0x002e, 0x0020, 0x0012, 0x0021, 0x0022, 0x0023, 0x0017, 0x0024, 0x0025, 0x0026,
    0x0032, 0x0031, 0x0018, 0x0019, 0x0010, 0x0013, 0x001f, 0x0014, 0x0016, 0x002f, 0x0011, 0x002d, 0x0015, 0x002c, 0x0002, 0x0003,
    0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x001c, 0x0001, 0x000e, 0x000f, 0x0039, 0x000c, 0x000d, 0x001a,
    0x001b, 0x002b, 0x002b, 0x0027, 0x0028, 0x0029, 0x0033, 0x0034, 0x0035, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0040,
    0x0041, 0x0042, 0x0043, 0x0044, 0x0057, 0x0058, 0x0063, 0x0046, 0x0077, 0xE052, 0xE047, 0xE049, 0xE053, 0xE04F, 0xE051, 0xE04D,
    0xE04B, 0xE050, 0xE048, 0x0045, 0xE035, 0x0037, 0x004a, 0x004e, 0xE01C, 0x004f, 0x0050, 0x0051, 0x004b, 0x004c, 0x004d, 0x0047,
    0x0048, 0x0049, 0x0052, 0x0053, 0x0056, 0xE05D, 0xE05E, 0x0075, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be,
    0x00bf, 0x00c0, 0x00c1, 0x00c2, 0x0086, 0x008a, 0x0082, 0x0084, 0x0080, 0x0081, 0x0083, 0x0089, 0x0085, 0x0087, 0x0088, 0x0071,
    0x0073, 0x0072, 0x0000, 0x0000, 0x0000, 0x0079, 0x0000, 0x0059, 0x005d, 0x007c, 0x005c, 0x005e, 0x005f, 0x0000, 0x0000, 0x0000,
    0x007a, 0x007b, 0x005a, 0x005b, 0x0055, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x001D, 0x002A, 0x0038, 0xE05B, 0xE01D, 0x0036, 0xE038, 0xE05C, 0x00a4, 0x00a6, 0x00a5, 0x00a3, 0x00a1, 0x0073, 0x0072, 0x0071,
    0x0096, 0x009e, 0x009f, 0x0080, 0x0088, 0x00b1, 0x00b2, 0x00b0, 0x008e, 0x0098, 0x00ad, 0x008c, 0x0000, 0x0000, 0x0000, 0x0000,
};

//#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

HIDPARSER_STATUS
HidParser_GetCollectionUsagePage(
    IN PVOID CollectionContext,
    OUT PUSHORT Usage,
    OUT PUSHORT UsagePage)
{
    PHID_COLLECTION Collection;

    //
    // find collection
    //
    Collection = HidParser_GetCollectionFromContext(CollectionContext);
    if (!Collection)
    {
        //
        // collection not found
        //
        return HIDPARSER_STATUS_COLLECTION_NOT_FOUND;
    }

    //
    // store result
    //
    *UsagePage = (Collection->Usage >> 16);
    *Usage = (Collection->Usage & 0xFFFF);
    return HIDPARSER_STATUS_SUCCESS;
}

ULONG
HidParser_GetReportLength(
    IN PVOID CollectionContext,
    IN UCHAR ReportType)
{
    PHID_REPORT Report;
    ULONG ReportLength;

    //
    // get first report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no report found
        //
        return 0;
    }

    //
    // get report length
    //
    ReportLength = Report->ReportSize;

    //
    // done
    //
    if (ReportLength)
    {
        //
        // byte aligned length
        //
        ASSERT(ReportLength % 8 == 0);
        return ReportLength / 8;
    }
    return ReportLength;
}

ULONG
HidParser_GetReportItemCountFromReportType(
    IN PVOID CollectionContext,
    IN UCHAR ReportType)
{
    PHID_REPORT Report;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    //
    // return report item count
    //
    return Report->ItemCount;
}


ULONG
HidParser_GetReportItemTypeCountFromReportType(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN ULONG bData)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    //
    // enumerate all items
    //
    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // check item type
        //
        if (Report->Items[Index].HasData && bData == TRUE)
        {
            //
            // found data item
            //
            ItemCount++;
        }
        else if (Report->Items[Index].HasData == FALSE && bData == FALSE)
        {
            //
            // found value item
            //
            ItemCount++;
        }
    }

    //
    // no report items
    //
    return ItemCount;
}


VOID
HidParser_InitParser(
    IN PHIDPARSER_ALLOC_FUNCTION AllocFunction,
    IN PHIDPARSER_FREE_FUNCTION FreeFunction,
    IN PHIDPARSER_ZERO_FUNCTION ZeroFunction,
    IN PHIDPARSER_COPY_FUNCTION CopyFunction,
    IN PHIDPARSER_DEBUG_FUNCTION DebugFunction,
    OUT PHID_PARSER Parser)
{
    Parser->Alloc = AllocFunction;
    Parser->Free = FreeFunction;
    Parser->Zero = ZeroFunction;
    Parser->Copy = CopyFunction;
    Parser->Debug = DebugFunction;
}

ULONG
HidParser_GetMaxUsageListLengthWithReportAndPage(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USAGE  UsagePage  OPTIONAL)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // check usage page
        //
        CurrentUsagePage = (Report->Items[Index].UsageMinimum >> 16);
        if (CurrentUsagePage == UsagePage && Report->Items[Index].HasData)
        {
            //
            // found item
            //
            ItemCount++;
        }
    }

    //
    // done
    //
    return ItemCount;
}

HIDPARSER_STATUS
HidParser_GetSpecificValueCapsWithReport(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USHORT UsagePage,
    IN USHORT Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PULONG  ValueCapsLength)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    USHORT CurrentUsage;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return HIDPARSER_STATUS_REPORT_NOT_FOUND;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // check usage page
        //
        CurrentUsagePage = (Report->Items[Index].UsageMinimum >> 16);
        CurrentUsage = (Report->Items[Index].UsageMinimum & 0xFFFF);

        if ((Usage == CurrentUsage && UsagePage == CurrentUsagePage) || (Usage == 0 && UsagePage == CurrentUsagePage) || (Usage == CurrentUsage && UsagePage == 0) || (Usage == 0 && UsagePage == 0))
        {
            //
            // check if there is enough place for the caps
            //
            if (ItemCount < *ValueCapsLength)
            {
                //
                // zero caps
                //
                Parser->Zero(&ValueCaps[ItemCount], sizeof(HIDP_VALUE_CAPS));

                //
                // init caps
                //
                ValueCaps[ItemCount].UsagePage = CurrentUsagePage;
                ValueCaps[ItemCount].ReportID = Report->ReportID;
                ValueCaps[ItemCount].LogicalMin = Report->Items[Index].Minimum;
                ValueCaps[ItemCount].LogicalMax = Report->Items[Index].Maximum;
                ValueCaps[ItemCount].IsAbsolute = !Report->Items[Index].Relative;
                ValueCaps[ItemCount].BitSize = Report->Items[Index].BitCount;

                //
                // FIXME: FILLMEIN
                //
            }


            //
            // found item
            //
            ItemCount++;
        }
    }

    //
    // store result
    //
    *ValueCapsLength = ItemCount;

    if (ItemCount)
    {
        //
        // success
        //
        return HIDPARSER_STATUS_SUCCESS;
    }

    //
    // item not found
    //
    return HIDPARSER_STATUS_USAGE_NOT_FOUND;
}

HIDPARSER_STATUS
HidParser_GetUsagesWithReport(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN UCHAR  ReportType,
    IN USAGE  UsagePage,
    OUT USAGE  *UsageList,
    IN OUT PULONG UsageLength,
    IN PCHAR  ReportDescriptor,
    IN ULONG  ReportDescriptorLength)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    PHID_REPORT_ITEM ReportItem;
    UCHAR Activated;
    ULONG Data;
    PUSAGE_AND_PAGE UsageAndPage = NULL;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return HIDPARSER_STATUS_REPORT_NOT_FOUND;
    }

    if (Report->ReportSize / 8 != (ReportDescriptorLength - 1))
    {
        //
        // invalid report descriptor length
        //
        return HIDPARSER_STATUS_INVALID_REPORT_LENGTH;
    }

    //
    // cast to usage and page
    //
    if (UsagePage == HID_USAGE_PAGE_UNDEFINED)
    {
        //
        // the caller requested any set usages
        //
        UsageAndPage = (PUSAGE_AND_PAGE)UsageList;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // get report item
        //
        ReportItem = &Report->Items[Index];

        //
        // does it have data
        //
        if (!ReportItem->HasData)
            continue;

        //
        // check usage page
        //
        CurrentUsagePage = (ReportItem->UsageMinimum >> 16);

        if (UsagePage != HID_USAGE_PAGE_UNDEFINED)
        {
            //
            // does usage match
            //
            if (UsagePage != CurrentUsagePage)
                continue;
        }

        //
        // check if the specified usage is activated
        //
        ASSERT(ReportItem->ByteOffset < ReportDescriptorLength);
        ASSERT(ReportItem->BitCount <= 8);

        //
        // one extra shift for skipping the prepended report id
        //
        Data = ReportDescriptor[ReportItem->ByteOffset + 1];

        //
        // shift data
        //
        Data >>= ReportItem->Shift;

        //
        // clear unwanted bits
        //
        Data &= ReportItem->Mask;

        //
        // is it activated
        //
        Activated = (Data != 0);

        if (!Activated)
            continue;

        //
        // is there enough space for the usage
        //
        if (ItemCount >= *UsageLength)
        {
            ItemCount++;
            continue;
        }

        if (UsagePage != HID_USAGE_PAGE_UNDEFINED)
        {
            //
            // store item
            //
            UsageList[ItemCount] = (ReportItem->UsageMinimum & 0xFFFF);
        }
        else
        {
            //
            // store usage and page
            //
            if (ReportItem->BitCount == 1)
            {
                //
                // use usage minimum
                //
                UsageAndPage[ItemCount].Usage =(ReportItem->UsageMinimum & 0xFFFF);
            }
            else
            {
                //
                // use value from control
                //
                UsageAndPage[ItemCount].Usage = (USHORT)Data;
            }
            UsageAndPage[ItemCount].UsagePage = CurrentUsagePage;
        }
        ItemCount++;
    }

    if (ItemCount > *UsageLength)
    {
        //
        // list too small
        //
        return HIDPARSER_STATUS_BUFFER_TOO_SMALL;
    }

    if (UsagePage == HID_USAGE_PAGE_UNDEFINED)
    {
        //
        // success, clear rest of array
        //
        Parser->Zero(&UsageAndPage[ItemCount], (*UsageLength - ItemCount) * sizeof(USAGE_AND_PAGE));
    }
    else
    {
        //
        // success, clear rest of array
        //
        Parser->Zero(&UsageList[ItemCount], (*UsageLength - ItemCount) * sizeof(USAGE));
    }


    //
    // store result size
    //
    *UsageLength = ItemCount;

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}

ULONG
HidParser_UsesReportId(
    IN PVOID CollectionContext,
    IN UCHAR  ReportType)
{
    PHID_REPORT Report;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    //
    // returns true when report id != 0
    //
    return (Report->ReportID != 0);

}


HIDPARSER_STATUS
HidParser_GetScaledUsageValueWithReport(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USAGE UsagePage,
    IN USAGE  Usage,
    OUT PLONG UsageValue,
    IN PCHAR ReportDescriptor,
    IN ULONG ReportDescriptorLength)
{
    ULONG Index;
    PHID_REPORT Report;
    USHORT CurrentUsagePage;
    PHID_REPORT_ITEM ReportItem;
    ULONG Data;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return HIDPARSER_STATUS_REPORT_NOT_FOUND;
    }

    if (Report->ReportSize / 8 != (ReportDescriptorLength - 1))
    {
        //
        // invalid report descriptor length
        //
        return HIDPARSER_STATUS_INVALID_REPORT_LENGTH;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // get report item
        //
        ReportItem = &Report->Items[Index];

        //
        // check usage page
        //
        CurrentUsagePage = (ReportItem->UsageMinimum >> 16);

        //
        // does usage page match
        //
        if (UsagePage != CurrentUsagePage)
            continue;

        //
        // does the usage match
        //
        if (Usage != (ReportItem->UsageMinimum & 0xFFFF))
            continue;

        //
        // check if the specified usage is activated
        //
        ASSERT(ReportItem->ByteOffset < ReportDescriptorLength);

        //
        // one extra shift for skipping the prepended report id
        //
        Data = 0;
        Parser->Copy(&Data, &ReportDescriptor[ReportItem->ByteOffset +1], min(sizeof(ULONG), ReportDescriptorLength - (ReportItem->ByteOffset + 1)));
        Data = ReportDescriptor[ReportItem->ByteOffset + 1];

        //
        // shift data
        //
        Data >>= ReportItem->Shift;

        //
        // clear unwanted bits
        //
        Data &= ReportItem->Mask;

        if (ReportItem->Minimum > ReportItem->Maximum)
        {
            //
            // logical boundaries are signed values
            //
            if ((Data & ~(ReportItem->Mask >> 1)) != 0)
            {
                Data |= ~ReportItem->Mask;
            }
        }

        //
        // store result
        //
        *UsageValue = Data;
        return HIDPARSER_STATUS_SUCCESS;
    }

    //
    // usage not found
    //
    return HIDPARSER_STATUS_USAGE_NOT_FOUND;
}

ULONG
HidParser_GetScanCode(
    IN USAGE Usage)
{
    if (Usage < sizeof(KeyboardScanCodes) / sizeof(KeyboardScanCodes[0]))
    {
        //
        // valid usage
        //
        return KeyboardScanCodes[Usage];
    }

    //
    // invalid usage
    //
    return 0;
}

VOID
HidParser_DispatchKey(
    IN PCHAR ScanCodes,
    IN HIDP_KEYBOARD_DIRECTION KeyAction,
    IN PHIDP_INSERT_SCANCODES InsertCodesProcedure,
    IN PVOID InsertCodesContext)
{
    ULONG Index;
    ULONG Length = 0;

    //
    // count code length
    //
    for(Index = 0; Index < sizeof(ULONG); Index++)
    {
        if (ScanCodes[Index] == 0)
        {
            //
            // last scan code
            //
            break;
        }

        //
        // is this a key break
        //
        if (KeyAction == HidP_Keyboard_Break)
        {
            //
            // add break - see USB HID to PS/2 Scan Code Translation Table
            //
            ScanCodes[Index] |= 0x80;
        }

        //
        // more scan counts
        //
        Length++;
    }

    if (Length > 0)
    {
         //
         // dispatch scan codes
         //
         InsertCodesProcedure(InsertCodesContext, ScanCodes, Length);
    }
}


HIDPARSER_STATUS
HidParser_TranslateUsage(
    IN PHID_PARSER Parser,
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    ULONG ScanCode;

    //
    // get scan code
    //
    ScanCode = HidParser_GetScanCode(Usage);
    if (!ScanCode)
    {
        //
        // invalid lookup or no scan code available
        //
        DPRINT1("No Scan code for Usage %x\n", Usage);
        return HIDPARSER_STATUS_I8042_TRANS_UNKNOWN;
    }

#if 0
    if ((ScanCode & 0xE000) == 0xE000)
    {
        //
        // swap scan code
        //
        ScanCode = NTOHS(ScanCode);
    }
#endif

    //
    // FIXME: translate modifier states
    //
    HidParser_DispatchKey((PCHAR)&ScanCode, KeyAction, InsertCodesProcedure, InsertCodesContext);

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}
