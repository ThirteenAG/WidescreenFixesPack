//-----------------------------------------------------------------------------
// File: D3DRes.h
//
// Desc: Resource definitions required by the CD3DApplication class.
//       Any application using the CD3DApplication class must include resources
//       with the following identifiers.
//
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef D3DRES_H
#define D3DRES_H


#define IDI_MAIN_ICON          101 // Application icon
#define IDR_MAIN_ACCEL         113 // Keyboard accelerator
#define IDR_MENU               141 // Application menu
#define IDR_POPUP              142 // Popup menu
#define IDD_SELECTDEVICE       144 // "Change Device" dialog box

#define IDC_ADAPTER_COMBO         1002 // Adapter combobox for "SelectDevice" dlg
#define IDC_DEVICE_COMBO          1000 // Device combobox for "SelectDevice" dlg
#define IDC_FULLSCREENMODES_COMBO 1003 // Mode combobox for "SelectDevice" dlg
#define IDC_MULTISAMPLE_COMBO     1005 // MultiSample combobox for "SelectDevice" dlg
#define IDC_WINDOW                1016 // Radio button for windowed-mode
#define IDC_FULLSCREEN            1018 // Radio button for fullscreen-mode

#define IDM_CHANGEDEVICE     40002 // Command to invoke "Change Device" dlg
#define IDM_TOGGLEFULLSCREEN 40003 // Command to toggle fullscreen mode
#define IDM_TOGGLESTART      40004 // Command to toggle frame animation
#define IDM_SINGLESTEP       40005 // Command to single step frame animation
#define IDM_EXIT             40006 // Command to exit the application


#endif // D3DRES_H
