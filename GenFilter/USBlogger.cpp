#include "USBlogger.h"

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS          status;


    WDF_DRIVER_CONFIG_INIT(&config,
                           USBloggerEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);


    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = STATUS_SUCCESS;
    return status;
}

_Use_decl_annotations_
NTSTATUS USBloggerEvtDeviceAdd(WDFDRIVER       Driver,
                               PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS                  status;
    WDF_OBJECT_ATTRIBUTES     wdfObjectAttr;
    WDFDEVICE                 wdfDevice;
    PGENFILTER_DEVICE_CONTEXT devContext;
    WDF_IO_QUEUE_CONFIG       ioQueueConfig;

    UNREFERENCED_PARAMETER(Driver);

    WdfFdoInitSetFilter(DeviceInit);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfObjectAttr,
                                            GENFILTER_DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit,
                             &wdfObjectAttr,
                             &wdfDevice);

    if (!NT_SUCCESS(status)) {
        goto done;
    }

    DbgPrint("Add device!\n");

    devContext = USBloggerGetDeviceContext(wdfDevice);
    devContext->WdfDevice = wdfDevice;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                           WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoInternalDeviceControl = USBloggerEvtIntDeviceControl;

    status = WdfIoQueueCreate(devContext->WdfDevice,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        goto done;
    }

    status = STATUS_SUCCESS;

done:
    return status;
}


_Use_decl_annotations_
VOID USBloggerEvtIntDeviceControl(WDFQUEUE   Queue,
    WDFREQUEST Request,
    size_t     OutputBufferLength,
    size_t     InputBufferLength,
    ULONG      IoControlCode)
{
    PGENFILTER_DEVICE_CONTEXT devContext;
    PIO_STACK_LOCATION pStack = NULL;
    PIRP Irp = NULL;
    PURB pUrb = NULL;

    devContext = USBloggerGetDeviceContext(WdfIoQueueGetDevice(Queue));

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    if (IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB) {
        DbgPrint("SUBMIT URB:\r\n");
        Irp = WdfRequestWdmGetIrp(Request);
        if (NT_SUCCESS(Irp->IoStatus.Status))
        {
            pStack = IoGetCurrentIrpStackLocation(Irp);
            pUrb = (PURB)pStack->Parameters.Others.Argument1;
            if (pUrb != NULL)
            {
                DbgPrintEx(0, 0, "%x: ", pUrb->UrbHeader.Function);
                switch (pUrb->UrbHeader.Function)
                {
                case URB_FUNCTION_SELECT_INTERFACE:
                {
                    DbgPrintEx(0, 0, "URB_FUNCTION_SELECT_INTERFACE\r\n");
                    break;
                }
                case URB_FUNCTION_ABORT_PIPE:
                {
                    DbgPrintEx(0, 0, "URB_FUNCTION_ABORT_PIPE\r\n");
                    break;
                }
                case URB_FUNCTION_ISOCH_TRANSFER:
                {
                    _URB_ISOCH_TRANSFER isoch = pUrb->UrbIsochronousTransfer;
                    int packets = isoch.NumberOfPackets;
                    DbgPrintEx(0, 0, "URB_FUNCTION_ISOCH_TRANSFER, Number of packets: %d\r\n", packets);
                    break;
                }
                case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
                {
                    DbgPrintEx(0, 0, "URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL\r\n");
                    break;
                }
                case URB_FUNCTION_CLASS_INTERFACE:
                {
                    DbgPrintEx(0, 0, "URB_FUNCTION_CLASS_INTERFACE\r\n");
                    break;
                }
                default:
                {
                    DbgPrintEx(0, 0, "Other USB function: %x\r\n", pUrb->UrbHeader.Function);
                    break;
                }
                }
            }
        }
        //10 1 2 30
        URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
        USBloggerSendWithCallback(Request, devContext);
        return;
    }


    USBloggerSendAndForget(Request,
        devContext);
}

_Use_decl_annotations_
VOID
USBloggerSendAndForget(WDFREQUEST                Request,
                       PGENFILTER_DEVICE_CONTEXT DevContext)
{
    NTSTATUS status;

    WDF_REQUEST_SEND_OPTIONS sendOpts;

    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOpts,
                                  WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    if (!WdfRequestSend(Request,
                        WdfDeviceGetIoTarget(DevContext->WdfDevice),
                        &sendOpts)) {

        status = WdfRequestGetStatus(Request);
        WdfRequestComplete(Request,
                           status);
    }
}

_Use_decl_annotations_
VOID
USBloggerCompletionCallback(WDFREQUEST                     Request,
                            WDFIOTARGET                    Target,
                            PWDF_REQUEST_COMPLETION_PARAMS Params,
                            WDFCONTEXT                     Context)
{
    NTSTATUS status;
    auto*    devContext = (PGENFILTER_DEVICE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(devContext);

    status = Params->IoStatus.Status;

    WdfRequestComplete(Request,
                       status);
}

_Use_decl_annotations_
VOID
USBloggerSendWithCallback(WDFREQUEST                Request,
                          PGENFILTER_DEVICE_CONTEXT DevContext)
{
    NTSTATUS status;

    WdfRequestFormatRequestUsingCurrentType(Request);

    WdfRequestSetCompletionRoutine(Request,
                                   USBloggerCompletionCallback,
                                   DevContext);
    if (!WdfRequestSend(Request,
                        WdfDeviceGetIoTarget(DevContext->WdfDevice),
                        WDF_NO_SEND_OPTIONS)) {

        status = WdfRequestGetStatus(Request);

        WdfRequestComplete(Request,
                           status);
    }
}
