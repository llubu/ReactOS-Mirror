/*
* PROJECT:     ReactOS Device Manager
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/win32/devmgr/devmgr/RootNode.cpp
* PURPOSE:     Root object for
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*
*/

#include "stdafx.h"
#include "devmgmt.h"
#include "RootNode.h"


CRootNode::CRootNode(_In_ PSP_CLASSIMAGELIST_DATA ImageListData) :
    CNode(RootNode, ImageListData)
{
}


CRootNode::~CRootNode()
{
}


bool
CRootNode::SetupNode()
{

    // Load the bitmap we'll be using as the root image
    HBITMAP hRootImage;
    hRootImage = LoadBitmapW(g_hInstance,
                                MAKEINTRESOURCEW(IDB_ROOT_IMAGE));
    if (hRootImage == NULL) return FALSE;

    // Add this bitmap to the device image list. This is a bit hacky, but it's safe
    m_ClassImage = ImageList_Add(m_ImageListData->ImageList,
                                 hRootImage,
                                 NULL);
    DeleteObject(hRootImage);


    // Get the root instance 
    CONFIGRET cr;
    cr = CM_Locate_DevNodeW(&m_DevInst,
                            NULL,
                            CM_LOCATE_DEVNODE_NORMAL);
    if (cr != CR_SUCCESS)
    {
        return false;
    }

    // The root name is the computer name 
    DWORD Size = DISPLAY_NAME_LEN;
    if (GetComputerNameW(m_DisplayName, &Size))
        _wcslwr_s(m_DisplayName);

    return true;


}