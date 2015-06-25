/*
 * PROJECT:     ReactOS Device Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/devmgr/devmgr/DeviceView.cpp
 * PURPOSE:     Implements the tree view which contains the devices
 * COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
 */



#include "stdafx.h"
#include "devmgmt.h"
#include "DeviceView.h"


// DATA ********************************************/

#define CLASS_NAME_LEN      256
#define CLASS_DESC_LEN      256
#define ROOT_NAME_SIZE      MAX_COMPUTERNAME_LENGTH + 1

INT_PTR
WINAPI
DevicePropertiesExW(
    IN HWND hWndParent OPTIONAL,
    IN LPCWSTR lpMachineName OPTIONAL,
    IN LPCWSTR lpDeviceID OPTIONAL,
    IN DWORD dwFlags OPTIONAL,
    IN BOOL bShowDevMgr
);

typedef INT_PTR(WINAPI *pDevicePropertiesExW)(HWND,LPCWSTR,LPCWSTR,DWORD,BOOL);

struct RefreshThreadData
{
    CDeviceView *This;
    BOOL ScanForChanges;
    BOOL UpdateView;
};


// PUBLIC METHODS ************************************/

CDeviceView::CDeviceView(
    HWND hMainWnd
    ) :
    m_hMainWnd(hMainWnd),
    m_hTreeView(NULL),
    m_hPropertyDialog(NULL),
    m_hMenu(NULL),
    m_hContextMenu(NULL),
    m_ViewType(DevicesByType),
    m_ShowHidden(FALSE),
    m_RootClassImage(-1),
    m_RootDevInst(0)
{
    ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));
}

CDeviceView::~CDeviceView(void)
{
}

bool
CDeviceView::Initialize()
{
    // Get the device image list
    m_ImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    BOOL bSuccess = SetupDiGetClassImageList(&m_ImageListData);
    if (bSuccess == FALSE) return false;

    // Create the main treeview
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                  WC_TREEVIEW,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES |
                                  TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT,
                                  0, 0, 0, 0,
                                  m_hMainWnd,
                                  (HMENU)IDC_TREEVIEW,
                                  g_hInstance,
                                  NULL);
    if (m_hTreeView)
    {
        // Set the image list against the treeview
        (void)TreeView_SetImageList(m_hTreeView,
                                    m_ImageListData.ImageList,
                                    TVSIL_NORMAL);

        // Give the treeview arrows instead of +/- boxes (on Win7)
        SetWindowTheme(m_hTreeView, L"explorer", NULL);
    }

    // Create the context menu and make properties the default item
    m_hContextMenu = CreatePopupMenu();
    SetMenuDefaultItem(m_hContextMenu, IDC_PROPERTIES, FALSE);

    return !!(m_hTreeView);
}

bool
CDeviceView::Uninitialize()
{
    EmptyDeviceView();

    if (m_ImageListData.ImageList != NULL)
    {
        SetupDiDestroyClassImageList(&m_ImageListData);
        ZeroMemory(&m_ImageListData, sizeof(SP_CLASSIMAGELIST_DATA));
    }

    DestroyMenu(m_hContextMenu);

    return true;
}

LRESULT
CDeviceView::OnSize(
    _In_ int x,
    _In_ int y,
    _In_ int cx,
    _In_ int cy
    )
{
    // Resize the treeview
    SetWindowPos(m_hTreeView,
                 NULL,
                 x,
                 y,
                 cx,
                 cy,
                 SWP_NOZORDER);

    return 0;
}

LRESULT
CDeviceView::OnRightClick(
    _In_ LPNMHDR NmHdr
    )
{
    HTREEITEM hItem = TreeView_GetNextItem(NmHdr->hwndFrom, 0, TVGN_DROPHILITE);
    if (hItem)
    {
        TreeView_SelectItem(NmHdr->hwndFrom, hItem);
    }

    return 0;
}

LRESULT
CDeviceView::OnContextMenu(
    _In_ LPARAM lParam
    )
{
    HTREEITEM hSelected = TreeView_GetSelection(m_hTreeView);

    RECT rc;
    if (TreeView_GetItemRect(m_hTreeView,
                             hSelected,
                             &rc,
                             TRUE))
    {
        POINT pt;
        if (GetCursorPos(&pt) &&
            ScreenToClient(m_hTreeView, &pt) &&
            PtInRect(&rc, pt))
        {

            CNode *Node = GetSelectedNode();
            if (Node && Node->HasProperties())
            {

            }






            INT xPos = GET_X_LPARAM(lParam);
            INT yPos = GET_Y_LPARAM(lParam);

            TrackPopupMenuEx(m_hContextMenu,
                             TPM_RIGHTBUTTON,
                             xPos,
                             yPos,
                             m_hMainWnd,
                             NULL);
        }
    }

    return 0;
}


void
CDeviceView::Refresh(
    _In_ ViewType Type,
    _In_ bool ScanForChanges,
    _In_ bool UpdateView
    )
{
    // Enum devices on a seperate thread to keep the gui responsive

    m_ViewType = Type;

    RefreshThreadData *ThreadData;
    ThreadData = new RefreshThreadData();
    ThreadData->This = this;
    ThreadData->ScanForChanges = ScanForChanges;
    ThreadData->UpdateView = UpdateView;

    HANDLE hThread;
    hThread = (HANDLE)_beginthreadex(NULL,
                                     0,
                                     &RefreshThread,
                                     ThreadData,
                                     0,
                                     NULL);

    if (hThread) CloseHandle(hThread);
}

void
CDeviceView::DisplayPropertySheet()
{
    //
    // In ReactOS we can link to DevicePropertiesEx but
    // not in windows as it's not part of the SDK 

#ifndef __REACTOS__
    HMODULE hModule = LoadLibraryW(L"devmgr.dll");
    if (hModule == NULL) return;

    pDevicePropertiesExW DevicePropertiesExW;
    DevicePropertiesExW = (pDevicePropertiesExW)GetProcAddress(hModule,
                                                               "DevicePropertiesExW");
    if (DevicePropertiesExW == NULL)
    {
        FreeLibrary(hModule);
        return;
    }
#endif

    CNode *Node = GetSelectedNode();
    if (Node && Node->HasProperties())
    {
        DevicePropertiesExW(m_hTreeView,
                            NULL,
                            Node->GetDeviceId(),
                            1,//DPF_EXTENDED,
                            FALSE);
    }

#ifndef __REACTOS__
    FreeLibrary(hModule);
#endif
}

void
CDeviceView::SetFocus()
{
}

bool
CDeviceView::HasProperties(
    _In_ LPTV_ITEMW TvItem
    )
{
    CNode *Node = GetNode(TvItem);
    if (Node)
    {
        return Node->HasProperties();
    }
    return false;
}

bool
CDeviceView::IsDisabled(
    _In_ LPTV_ITEMW TvItem
    )
{
    CDeviceNode *Node = dynamic_cast<CDeviceNode *>(GetNode(TvItem));
    if (Node)
    {
        return Node->IsDisabled();
    }
    return false;
}

bool
CDeviceView::CanDisable(
    _In_ LPTV_ITEMW TvItem
    )
{
    CDeviceNode *Node = dynamic_cast<CDeviceNode *>(GetNode(TvItem));
    if (Node)
    {
        return Node->CanDisable();
    }
    return false;
}

bool
CDeviceView::EnableSelectedDevice(
    _In_ bool Enable,
    _Out_ bool &NeedsReboot
    )
{
    CDeviceNode *Node = dynamic_cast<CDeviceNode *>(GetSelectedNode());
    if (Node)
    {
        if (Node->EnableDevice(Enable, NeedsReboot))
        {
            Refresh(m_ViewType, true, true);
            return true;
        }
    }
    return false;
}


// PRIVATE METHODS *******************************************/

bool
CDeviceView::AddRootDevice()
{
    // Check whether we've loaded the root bitmap into the imagelist (done on first run)
    if (m_RootClassImage == -1)
    {
        // Load the bitmap we'll be using as the root image
        HBITMAP hRootImage;
        hRootImage = LoadBitmapW(g_hInstance,
                                 MAKEINTRESOURCEW(IDB_ROOT_IMAGE));
        if (hRootImage == NULL) return FALSE;

        // Add this bitmap to the device image list. This is a bit hacky, but it's safe
        m_RootClassImage = ImageList_Add(m_ImageListData.ImageList,
                                   hRootImage,
                                   NULL);
        DeleteObject(hRootImage);
    }

    // Get the root instance 
    CONFIGRET cr;
    cr = CM_Locate_DevNodeW(&m_RootDevInst,
                            NULL,
                            CM_LOCATE_DEVNODE_NORMAL);
    if (cr != CR_SUCCESS)
    {
        return false;
    }

    // The root name is the computer name 
    WCHAR RootDeviceName[ROOT_NAME_SIZE];
    DWORD Size = ROOT_NAME_SIZE;
    if (GetComputerNameW(RootDeviceName, &Size))
        _wcslwr_s(RootDeviceName);

    TV_ITEMW tvi;
    TV_INSERTSTRUCT tvins;
    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    // Insert the root / parent item into our treeview
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = RootDeviceName;
    tvi.cchTextMax = wcslen(RootDeviceName);
    tvi.iImage = m_RootClassImage;
    tvi.iSelectedImage = m_RootClassImage;
    tvins.item = tvi;
    m_hTreeRoot = TreeView_InsertItem(m_hTreeView, &tvins);

    return (m_hTreeRoot != NULL);

}

bool
CDeviceView::GetNextClass(
    _In_ ULONG ClassIndex,
    _Out_ LPGUID ClassGuid,
    _Out_ HDEVINFO *hDevInfo
    )
{
    CONFIGRET cr;

    // Get the next class in the list
    cr = CM_Enumerate_Classes(ClassIndex,
                              ClassGuid,
                              0);
    if (cr != CR_SUCCESS) return false;

    // Check if this is the unknown class
    if (IsEqualGUID(*ClassGuid, GUID_DEVCLASS_UNKNOWN))
    {
        // Get device info for all devices
        *hDevInfo = SetupDiGetClassDevsW(NULL,
                                         NULL,
                                         NULL,
                                         DIGCF_ALLCLASSES);
    }
    else
    {
        // We only want the devices for this class
        *hDevInfo = SetupDiGetClassDevsW(ClassGuid,
                                         NULL,
                                         NULL,
                                         DIGCF_PRESENT);
    }

    return (hDevInfo != INVALID_HANDLE_VALUE);
}

unsigned int __stdcall CDeviceView::RefreshThread(void *Param)
{
    RefreshThreadData *ThreadData = (RefreshThreadData *)Param;
    CDeviceView *This = ThreadData->This;


    // Empty the treeview
    This->EmptyDeviceView();
    This->m_hTreeRoot = NULL;

    // Refresh the devices only if requested. This means
    // switching views uses the cache and remains fast
    if (ThreadData->ScanForChanges)
    {
        This->RefreshDeviceList();
    }

    // display the type of view the user wants
    switch (This->m_ViewType)
    {
        case DevicesByType:
            (void)This->ListDevicesByType();
            break;

        case DevicesByConnection:
            (VOID)This->ListDevicesByConnection();
            break;

        case ResourcesByType:
            break;

        case ResourcesByConnection:
            break;
    }

    delete ThreadData;

    return 0;
}


bool
CDeviceView::ListDevicesByType()
{
    CClassNode *ClassNode;
    CDeviceNode *DeviceNode;
    HDEVINFO hDevInfo;
    HTREEITEM hTreeItem = NULL;
    GUID ClassGuid;
    INT ClassIndex;
    LPTSTR DeviceId = NULL;
    BOOL bClassSuccess, bSuccess;

    // Start by adding the root node to the tree
    bSuccess = AddRootDevice();
    if (bSuccess == false) return false;

    ClassIndex = 0;
    do
    {
        // Loop through all the device classes
        bClassSuccess = GetNextClass(ClassIndex, &ClassGuid, &hDevInfo);
        if (bClassSuccess)
        {
            bool bClassUnknown = false;
            bool AddedParent = false;
            INT DeviceIndex = 0;
            BOOL MoreItems;

            // Get the cached class node
            ClassNode = GetClassNode(&ClassGuid);
            if (ClassNode == NULL)
            {
                ATLASSERT(FALSE);
                ClassIndex++;
                continue;
            }

            // Set a flag is this is the (special case) unknown class
            if (IsEqualGUID(ClassGuid, GUID_DEVCLASS_UNKNOWN))
                bClassUnknown = true;

            do
            {
                // Get a handle to all the devices in this class
                SP_DEVINFO_DATA DeviceInfoData;
                ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
                DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                bSuccess = SetupDiEnumDeviceInfo(hDevInfo,
                                                 DeviceIndex,
                                                 &DeviceInfoData);
                if (bSuccess == FALSE && GetLastError() == ERROR_NO_MORE_ITEMS)
                    MoreItems = FALSE;

                if (bSuccess)
                {
                    MoreItems = TRUE;

                    // The unknown class handle contains all devices on the system,
                    // and we're just looking for the ones with a null GUID
                    if (bClassUnknown)
                    {
                        if (IsEqualGUID(DeviceInfoData.ClassGuid, GUID_NULL) == FALSE)
                        {
                            // This is a known device, we aren't interested in it
                            DeviceIndex++;
                            continue;
                        }
                    }

                    // Get the cached device node
                    DeviceNode = GetDeviceNode(DeviceInfoData.DevInst);
                    if (DeviceNode == NULL)
                    {
                        ATLASSERT(bClassUnknown == true);
                        DeviceIndex++;
                        continue;
                    }

                    // Check if this is a hidden device
                    if (DeviceNode->IsHidden())
                    {
                        // Ignore this device if we aren't displaying hidden devices
                        if (m_ShowHidden == FALSE)
                        {
                            DeviceIndex++;
                            continue;
                        }
                    }

                    // We have a device, we need to add the parent if it hasn't yet been added
                    if (AddedParent == false)
                    {
                        // Insert the new class under the root item
                        hTreeItem = InsertIntoTreeView(m_hTreeRoot,
                                                       ClassNode);
                        AddedParent = true;
                    }

                    // Add the device under the class item node
                    (void)InsertIntoTreeView(hTreeItem, DeviceNode);

                    // Expand the class if it has a problem device
                    if (DeviceNode->HasProblem())
                    {
                        (void)TreeView_Expand(m_hTreeView,
                                              hTreeItem,
                                              TVE_EXPAND);
                    }
                }

                DeviceIndex++;

            } while (MoreItems);

            // If this class has devices, sort them alphabetically
            if (AddedParent == true)
            {
                (void)TreeView_SortChildren(m_hTreeView,
                                            hTreeItem,
                                            0);
            }
        }

        ClassIndex++;

    } while (bClassSuccess);

    // Sort the classes alphabetically
    (void)TreeView_SortChildren(m_hTreeView,
                                m_hTreeRoot,
                                0);

    // Expand the root item
    (void)TreeView_Expand(m_hTreeView,
                          m_hTreeRoot,
                          TVE_EXPAND);

    // Pre-select the root item
    (VOID)TreeView_SelectItem(m_hTreeView,
                              m_hTreeRoot);

    return 0;
}

bool
CDeviceView::ListDevicesByConnection()
{
    bool bSuccess;

    // Start by adding the root node to the tree
    bSuccess = AddRootDevice();
    if (bSuccess == false) return false;

    // Walk the device tree and add all the devices 
    (void)RecurseChildDevices(m_RootDevInst, m_hTreeRoot);

    // Expand the root item 
    (void)TreeView_Expand(m_hTreeView,
                          m_hTreeRoot,
                          TVE_EXPAND);

    return true;
}

bool
CDeviceView::RecurseChildDevices(
    _In_ DEVINST ParentDevice,
    _In_ HTREEITEM hParentTreeItem
    )
{
    HTREEITEM hDevItem = NULL;
    DEVINST Device;
    bool HasProblem = false;
    bool bSuccess;

    // Check if the parent has any child devices 
    if (GetChildDevice(ParentDevice, &Device) == FALSE)
        return true;

    // Get the cached device node
    CDeviceNode *DeviceNode;
    DeviceNode = dynamic_cast<CDeviceNode *>(GetDeviceNode(Device));
    if (DeviceNode == NULL)
    {
        ATLASSERT(FALSE);
        return false;
    }

    // Don't show hidden devices if not requested
    if ((m_ShowHidden == TRUE) || (!(DeviceNode->IsHidden())))
    {
        // Add this device to the tree under its parent 
        hDevItem = InsertIntoTreeView(hParentTreeItem,
                                      DeviceNode);
        if (hDevItem)
        {
            // Check if this child has any children itself 
            if (!RecurseChildDevices(Device, hDevItem))
                HasProblem = true;
        }

        if (DeviceNode->HasProblem())
        {
            HasProblem = true;
        }
    }


    // Check for siblings
    for (;;)
    {
        // Check if the parent device has anything at the same level 
        bSuccess = GetSiblingDevice(Device, &Device);
        if (bSuccess == FALSE) break;

        DeviceNode = dynamic_cast<CDeviceNode *>(GetDeviceNode(Device));
        if (DeviceNode == NULL)
        {
            ATLASSERT(FALSE);
        }

        // Don't show hidden devices if not requested
        if ((m_ShowHidden == TRUE) || (!(DeviceNode->IsHidden())))
        {
            if (DeviceNode->HasProblem())
            {
                HasProblem = true;
            }

            // Add this device to the tree under its parent 
            hDevItem = InsertIntoTreeView(hParentTreeItem,
                                          DeviceNode);
            if (hDevItem)
            {
                // Check if this child has any children itself 
                if (!RecurseChildDevices(Device, hDevItem))
                    HasProblem = true;
            }
        }
    }

    (void)TreeView_SortChildren(m_hTreeView,
                                hParentTreeItem,
                                0);

    // Expand the class if it has a problem device
    if (HasProblem == true)
    {
        (void)TreeView_Expand(m_hTreeView,
                              hParentTreeItem,
                              TVE_EXPAND);
    }

    // If there was a problem, expand the ancestors
    if (HasProblem) return false;

    return true;
}

bool
CDeviceView::GetChildDevice(
    _In_ DEVINST ParentDevInst,
    _Out_ PDEVINST DevInst
)
{
    CONFIGRET cr;
    cr = CM_Get_Child(DevInst,
                      ParentDevInst,
                      0);
    return (cr == CR_SUCCESS);
}

bool
CDeviceView::GetSiblingDevice(
    _In_ DEVINST PrevDevice,
    _Out_ PDEVINST DevInst
)
{
    CONFIGRET cr;
    cr = CM_Get_Sibling(DevInst,
                        PrevDevice,
                        0);
    return (cr == CR_SUCCESS);
}

HTREEITEM
CDeviceView::InsertIntoTreeView(
    _In_ HTREEITEM hParent,
    _In_ CNode *Node
    )
{
    
    LPWSTR lpLabel;
    lpLabel = Node->GetDisplayName();

    TV_ITEMW tvi;
    TV_INSERTSTRUCT tvins;
    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = wcslen(lpLabel);
    tvi.lParam = (LPARAM)Node;
    tvi.iImage = Node->GetClassImage();
    tvi.iSelectedImage = Node->GetClassImage();

    // try to cast it to a device node. This will only suceed if it's the correct type
    CDeviceNode *DeviceNode = dynamic_cast<CDeviceNode *>(Node);
    if (DeviceNode && DeviceNode->GetOverlayImage())
    {
        tvi.mask |= TVIF_STATE;
        tvi.stateMask = TVIS_OVERLAYMASK;
        tvi.state = INDEXTOOVERLAYMASK(DeviceNode->GetOverlayImage());
    }

    tvins.item = tvi;
    tvins.hParent = hParent;

    return TreeView_InsertItem(m_hTreeView, &tvins);
}

void
CDeviceView::RecurseDeviceView(
    _In_ HTREEITEM hParentItem
    )
{
    HTREEITEM hItem;
    TVITEMW tvItem;

    // Check if this node has any children
    hItem = TreeView_GetChild(m_hTreeView, hParentItem);
    if (hItem == NULL) return;

    // The lParam contains the node pointer data
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, &tvItem) &&
        tvItem.lParam != NULL)
    {
        // Delete the node class
        //delete reinterpret_cast<CNode *>(tvItem.lParam);
    }

    // This node may have its own children
    RecurseDeviceView(hItem);

    // Delete all the siblings
    for (;;)
    {
        // Get the next item at this level
        hItem = TreeView_GetNextSibling(m_hTreeView, hItem);
        if (hItem == NULL) break;

        // The lParam contains the node pointer data
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_PARAM;
        if (TreeView_GetItem(m_hTreeView, &tvItem))
        {
            //if (tvItem.lParam != NULL)
            //    delete reinterpret_cast<CNode *>(tvItem.lParam);
        }

        // This node may have its own children 
        RecurseDeviceView(hItem);
    }
}


void
CDeviceView::EmptyDeviceView()
{
    HTREEITEM hItem;

    // Check if there are any items in the tree
    hItem = TreeView_GetRoot(m_hTreeView);
    if (hItem == NULL) return;

    // Free all the class nodes
    //RecurseDeviceView(hItem);

    // Delete all the items
    (VOID)TreeView_DeleteAllItems(m_hTreeView);
}




CClassNode*
CDeviceView::GetClassNode(
    _In_ LPGUID ClassGuid
    )
{
    POSITION Pos;
    CClassNode *Node;

    Pos = m_ClassNodeList.GetHeadPosition();

    do
    {
        Node = m_ClassNodeList.GetNext(Pos);
        if (IsEqualGUID(*Node->GetClassGuid(), *ClassGuid))
        {
            //ATLASSERT(Node->GetType() == NodeClass);
            break;
        }

        Node = NULL;

    } while (Pos != NULL);

    return Node;
}

CDeviceNode*
CDeviceView::GetDeviceNode(
    _In_ DEVINST Device
    )
{
    POSITION Pos;
    CDeviceNode *Node;

    Pos = m_DeviceNodeList.GetHeadPosition();

    do
    {
        Node = m_DeviceNodeList.GetNext(Pos);
        if (Node->GetDeviceInst() == Device)
        {
            //ATLASSERT(Node->GetType() == NodeDevice);
            break;
        }

        Node = NULL;

    } while (Pos != NULL);

    return Node;
}

CNode* CDeviceView::GetNode(
    _In_ LPTV_ITEMW TvItem
    )
{
    TvItem->mask = TVIF_PARAM;
    if (TreeView_GetItem(m_hTreeView, TvItem))
    {
        return (CNode *)TvItem->lParam;
    }
    return NULL;
}

CNode* CDeviceView::GetSelectedNode()
{
    TV_ITEM TvItem;
    TvItem.hItem = TreeView_GetSelection(m_hTreeView);
    return GetNode(&TvItem);
}

void
CDeviceView::EmptyLists()
{
    CNode *Node;

    while (!m_ClassNodeList.IsEmpty())
    {
        Node = m_ClassNodeList.RemoveTail();
        delete Node;
    }

    while (!m_DeviceNodeList.IsEmpty())
    {
        Node = m_DeviceNodeList.RemoveTail();
        delete Node;
    }
}

bool
CDeviceView::RefreshDeviceList()
{
    GUID ClassGuid;
    CClassNode *ClassNode;
    CDeviceNode *DeviceNode;
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i;
    BOOL Success;

    ULONG ClassIndex = 0;

    EmptyLists();

    // Loop through all the classes
    do
    {
        Success = GetNextClass(ClassIndex, &ClassGuid, &hDevInfo);
        if (Success)
        {
            // Create a new class node and add it to the list
            ClassNode = new CClassNode(&ClassGuid, &m_ImageListData);
            if (ClassNode->SetupNode())
            {
                m_ClassNodeList.AddTail(ClassNode);
            }
        }
        ClassIndex++;
    } while (Success);


    // Get all the devices on the local machine
    hDevInfo = SetupDiGetClassDevsW(NULL,
                                    0,
                                    0,
                                    DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // loop though all the devices
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0;; i++)
    {
        // Get the devinst for this device
        Success = SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData);
        if (Success == FALSE) break;

        // create a new device node and add it to the list
        DeviceNode = new CDeviceNode(DeviceInfoData.DevInst, &m_ImageListData);
        if (DeviceNode->SetupNode())
        {
            m_DeviceNodeList.AddTail(DeviceNode);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return TRUE;
}