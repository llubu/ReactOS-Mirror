#include "../vgaddi.h"

ULONG oldx, oldy;
PUCHAR behindCursor;

void vgaHideCursor(PPDEV ppdev);
void vgaShowCursor(PPDEV ppdev);

BOOL InitPointer(PPDEV ppdev)
{
  ULONG CursorWidth = 16, CursorHeight = 16;

  // Determine the size of the pointer attributes
  ppdev->PointerAttributes = sizeof(VIDEO_POINTER_ATTRIBUTES) +
    (CursorWidth * CursorHeight) * 2; // space for two cursors (data and mask); we assume 4bpp.. but use 8bpp for speed

  ppdev->pPointerAttributes = EngAllocMem(0, 512, ALLOC_TAG);

  // Allocate memory for pointer attributes
  ppdev->pPointerAttributes = EngAllocMem(0, ppdev->PointerAttributes, ALLOC_TAG);

  ppdev->pPointerAttributes->Flags = 0; // FIXME: Do this right
  ppdev->pPointerAttributes->Width = CursorWidth;
  ppdev->pPointerAttributes->Height = CursorHeight;
  ppdev->pPointerAttributes->WidthInBytes = CursorWidth / 2;
  ppdev->pPointerAttributes->Enable = 0;
  ppdev->pPointerAttributes->Column = 0;
  ppdev->pPointerAttributes->Row = 0;

  // Allocate memory for the pixels behind the cursor
  behindCursor = EngAllocMem(0, ppdev->pPointerAttributes->WidthInBytes * ppdev->pPointerAttributes->Height, ALLOC_TAG);

  return TRUE;
}

VOID VGADDIMovePointer(PSURFOBJ pso, LONG x, LONG y, PRECTL prcl)
{
  PPDEV ppdev = (PPDEV)pso->dhpdev;

  if(x == -1)
  {
    // x == -1 and y == -1 indicates we must hide the cursor
    vgaHideCursor(ppdev);
    return;
  }

  ppdev->xyCursor.x = x;
  ppdev->xyCursor.y = y;

  vgaShowCursor(ppdev);

  // Give feedback on the new cursor rectangle
//  if (prcl != NULL) ComputePointerRect(ppdev, prcl);
}

ULONG VGADDISetPointerShape(PSURFOBJ pso, PSURFOBJ psoMask, PSURFOBJ psoColor, PXLATEOBJ pxlo,
			    LONG xHot, LONG yHot, LONG x, LONG y,
			    PRECTL prcl, ULONG fl)
{
  PPDEV ppdev = (PPDEV)pso->dhpdev;
  PCHAR DFBTmp;
  ULONG DFBAllocSize;

  // Hide the cursor
  if(ppdev->pPointerAttributes->Enable != 0) vgaHideCursor(ppdev);

  // Copy the mask and color bitmaps into the PPDEV
  RtlCopyMemory(ppdev->pPointerAttributes->Pixels, psoMask->pvBits, psoMask->cjBits);
  if(psoColor != NULL) RtlCopyMemory(ppdev->pPointerAttributes->Pixels + 256, psoColor->pvBits, psoColor->cjBits);
  ppdev->pPointerAttributes->WidthInBytes = psoMask->lDelta;

  EngFreeMem(behindCursor);
  behindCursor = EngAllocMem(0, ppdev->pPointerAttributes->WidthInBytes * ppdev->pPointerAttributes->Height, ALLOC_TAG);

  // Set the new cursor position
  ppdev->xyCursor.x = x;
  ppdev->xyCursor.y = y;

  // Convert the cursor DIB into a DFB
  DFBAllocSize = psoMask->cjBits;
  DFBTmp = EngAllocMem(0, DFBAllocSize, ALLOC_TAG);
  DIB_BltToDFB(0, 0,
               ppdev->pPointerAttributes->Width,
               ppdev->pPointerAttributes->Height,
               DFBTmp, ppdev->pPointerAttributes->WidthInBytes,
               ppdev->pPointerAttributes->Pixels, ppdev->pPointerAttributes->WidthInBytes);
  RtlCopyMemory(ppdev->pPointerAttributes->Pixels, DFBTmp, psoMask->cjBits);
  EngFreeMem(DFBTmp);

  // Show the cursor
  vgaShowCursor(ppdev);
}

void vgaHideCursor(PPDEV ppdev)
{
  ULONG i, j, cx, cy, bitpos;

  // Display what was behind cursor
  DFB_BltToVGA(oldx, oldy,
               ppdev->pPointerAttributes->Width,
               ppdev->pPointerAttributes->Height,
               behindCursor,
               ppdev->pPointerAttributes->WidthInBytes);

  ppdev->pPointerAttributes->Enable = 0;
}

void vgaShowCursor(PPDEV ppdev)
{
  ULONG i, j, cx, cy;

  if(ppdev->pPointerAttributes->Enable != 0) vgaHideCursor(ppdev);

  // Capture pixels behind the cursor
  cx = ppdev->xyCursor.x;
  cy = ppdev->xyCursor.y;

  // Used to repaint background
  DFB_BltFromVGA(ppdev->xyCursor.x, ppdev->xyCursor.y,
                 ppdev->pPointerAttributes->Width, ppdev->pPointerAttributes->Height,
                 behindCursor, ppdev->pPointerAttributes->WidthInBytes);

  // Display the cursor
  DFB_BltToVGA_Transparent(ppdev->xyCursor.x, ppdev->xyCursor.y,
                           ppdev->pPointerAttributes->Width,
                           ppdev->pPointerAttributes->Height,
                           ppdev->pPointerAttributes->Pixels,
                           ppdev->pPointerAttributes->WidthInBytes, 5);

  oldx = ppdev->xyCursor.x;
  oldy = ppdev->xyCursor.y;

  ppdev->pPointerAttributes->Enable = 1;
}
