#pragma once

#include <wdm.h>
#include <wdf.h>
#include <usb.h>
#include <usbioctl.h>

#pragma warning(disable: 26438)     // avoid "goto"
#pragma warning(disable: 26440)     // Function can be delcared "noexcept"
#pragma warning(disable: 26485)     // No array to pointer decay
#pragma warning(disable: 26493)     // C-style casts
#pragma warning(disable: 26494)     // Variable is uninitialized


typedef struct _GENFILTER_DEVICE_CONTEXT { 
    WDFDEVICE       WdfDevice;

} GENFILTER_DEVICE_CONTEXT, *PGENFILTER_DEVICE_CONTEXT;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(GENFILTER_DEVICE_CONTEXT,
                                   USBloggerGetDeviceContext)

extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD USBloggerEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL USBloggerEvtIntDeviceControl;

EVT_WDF_REQUEST_COMPLETION_ROUTINE USBloggerCompletionCallback;

VOID
USBloggerSendAndForget(_In_ WDFREQUEST Request, _In_ PGENFILTER_DEVICE_CONTEXT DevContext);

VOID
USBloggerSendWithCallback(_In_ WDFREQUEST Request, _In_ PGENFILTER_DEVICE_CONTEXT DevContext);
