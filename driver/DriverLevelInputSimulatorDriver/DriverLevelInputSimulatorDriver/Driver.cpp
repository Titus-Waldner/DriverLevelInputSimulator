#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>

#include "../../../shared/include/DriverLevelInputSimulator/DeviceProtocol.hpp"

extern "C"
{
    DRIVER_INITIALIZE DriverEntry;
}

EVT_WDF_DRIVER_DEVICE_ADD DriverEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP DeviceEvtCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DeviceEvtIoDeviceControl;

struct DeviceContext
{
    VHFHANDLE vhfHandle;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(
    DeviceContext,
    GetDeviceContext
);

namespace
{

constexpr UCHAR MouseReportId = 1;

UCHAR InputReportDescriptor[] =
{
    //
    // Report ID 1: Five-button relative mouse.
    //
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       //   Report ID (1)

    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)

    0x05, 0x09,       //     Usage Page (Button)
    0x19, 0x01,       //     Usage Minimum (Button 1)
    0x29, 0x05,       //     Usage Maximum (Button 5)
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x75, 0x01,       //     Report Size (1)
    0x95, 0x05,       //     Report Count (5)
    0x81, 0x02,       //     Input (Data, Variable, Absolute)

    0x75, 0x03,       //     Report Size (3)
    0x95, 0x01,       //     Report Count (1)
    0x81, 0x03,       //     Input (Constant, Variable, Absolute)

    0x05, 0x01,       //     Usage Page (Generic Desktop)
    0x09, 0x30,       //     Usage (X)
    0x09, 0x31,       //     Usage (Y)
    0x09, 0x38,       //     Usage (Wheel)
    0x15, 0x81,       //     Logical Minimum (-127)
    0x25, 0x7F,       //     Logical Maximum (127)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x03,       //     Report Count (3)
    0x81, 0x06,       //     Input (Data, Variable, Relative)

    0x05, 0x0C,       //     Usage Page (Consumer)
    0x0A, 0x38, 0x02, //     Usage (AC Pan)
    0x15, 0x81,       //     Logical Minimum (-127)
    0x25, 0x7F,       //     Logical Maximum (127)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x01,       //     Report Count (1)
    0x81, 0x06,       //     Input (Data, Variable, Relative)

    0xC0,             //   End Physical Collection
    0xC0,             // End Mouse Application Collection

    //
    // Report ID 2: Standard six-key keyboard.
    //
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID (2)

    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,       //   Usage Minimum (Left Control)
    0x29, 0xE7,       //   Usage Maximum (Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)

    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x03,       //   Input (Constant)

    0x05, 0x08,       //   Usage Page (LEDs)
    0x19, 0x01,       //   Usage Minimum (Num Lock)
    0x29, 0x05,       //   Usage Maximum (Kana)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x05,       //   Report Count (5)
    0x91, 0x02,       //   Output (Data, Variable, Absolute)

    0x75, 0x03,       //   Report Size (3)
    0x95, 0x01,       //   Report Count (1)
    0x91, 0x03,       //   Output (Constant)

    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x65,       //   Usage Maximum (Keyboard Application)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x65,       //   Logical Maximum (101)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x00,       //   Input (Data, Array, Absolute)

    0xC0              // End Keyboard Application Collection
};

#pragma pack(push, 1)

struct HidMouseReport
{
    UCHAR reportId;
    UCHAR buttons;
    CHAR movementX;
    CHAR movementY;
    CHAR verticalWheel;
    CHAR horizontalWheel;
};

struct HidKeyboardReport
{
    UCHAR reportId;
    UCHAR modifiers;
    UCHAR reserved;
    UCHAR keys[6];
};

#pragma pack(pop)

static_assert(
    sizeof(HidMouseReport) == 6,
    "HidMouseReport must contain exactly six bytes."
);

constexpr UCHAR KeyboardReportId = 2;

NTSTATUS CreateControlQueue(
    WDFDEVICE device
)
{
    WDF_IO_QUEUE_CONFIG queueConfig;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoDeviceControl = DeviceEvtIoDeviceControl;

    return WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
    );
}

NTSTATUS CreateControllerSymbolicLink(
    WDFDEVICE device
)
{
    DECLARE_CONST_UNICODE_STRING(
        symbolicLinkName,
        L"\\DosDevices\\DriverLevelInputSimulator"
    );

    return WdfDeviceCreateSymbolicLink(
        device,
        &symbolicLinkName
    );
}

}

extern "C"
NTSTATUS DriverEntry(
    PDRIVER_OBJECT driverObject,
    PUNICODE_STRING registryPath
)
{
    WDF_DRIVER_CONFIG driverConfig;

    WDF_DRIVER_CONFIG_INIT(
        &driverConfig,
        DriverEvtDeviceAdd
    );

    return WdfDriverCreate(
        driverObject,
        registryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &driverConfig,
        WDF_NO_HANDLE
    );
}

NTSTATUS DriverEvtDeviceAdd(
    WDFDRIVER driver,
    PWDFDEVICE_INIT deviceInit
)
{
    UNREFERENCED_PARAMETER(driver);

    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDFDEVICE device;
    DeviceContext* deviceContext;
    VHF_CONFIG vhfConfig;
    NTSTATUS status;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &deviceAttributes,
        DeviceContext
    );

    deviceAttributes.EvtCleanupCallback = DeviceEvtCleanup;
    deviceAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    status = WdfDeviceCreate(
        &deviceInit,
        &deviceAttributes,
        &device
    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    deviceContext = GetDeviceContext(device);
    deviceContext->vhfHandle = nullptr;

    status = CreateControllerSymbolicLink(device);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = CreateControlQueue(device);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    VHF_CONFIG_INIT(
        &vhfConfig,
        WdfDeviceWdmGetDeviceObject(device),
        static_cast<USHORT>(sizeof(InputReportDescriptor)),
		InputReportDescriptor
    );

    vhfConfig.VendorID = 0x1209;
    vhfConfig.ProductID = 0x0001;
    vhfConfig.VersionNumber = 0x0001;

    status = VhfCreate(
        &vhfConfig,
        &deviceContext->vhfHandle
    );

    if (!NT_SUCCESS(status))
    {
        deviceContext->vhfHandle = nullptr;
        return status;
    }

    status = VhfStart(deviceContext->vhfHandle);

    if (!NT_SUCCESS(status))
    {
        VhfDelete(
            deviceContext->vhfHandle,
            TRUE
        );

        deviceContext->vhfHandle = nullptr;
        return status;
    }

    return STATUS_SUCCESS;
}

VOID DeviceEvtIoDeviceControl(
    WDFQUEUE queue,
    WDFREQUEST request,
    size_t outputBufferLength,
    size_t inputBufferLength,
    ULONG ioControlCode
)
{
    UNREFERENCED_PARAMETER(outputBufferLength);

    WDFDEVICE device;
    DeviceContext* deviceContext;
    HID_XFER_PACKET transferPacket;
    NTSTATUS status;

    device = WdfIoQueueGetDevice(queue);
    deviceContext = GetDeviceContext(device);

    if (deviceContext->vhfHandle == nullptr)
    {
        WdfRequestComplete(
            request,
            STATUS_DEVICE_NOT_READY
        );

        return;
    }

    if (ioControlCode ==
        DriverLevelInputSimulator::IoctlSubmitMouseReport)
    {
        DriverLevelInputSimulator::MouseCommand* command;
        HidMouseReport report;

        if (inputBufferLength !=
            sizeof(DriverLevelInputSimulator::MouseCommand))
        {
            WdfRequestComplete(
                request,
                STATUS_INVALID_BUFFER_SIZE
            );

            return;
        }

        status = WdfRequestRetrieveInputBuffer(
            request,
            sizeof(DriverLevelInputSimulator::MouseCommand),
            reinterpret_cast<PVOID*>(&command),
            nullptr
        );

        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(request, status);
            return;
        }

        if ((command->buttons & 0xE0U) != 0)
        {
            WdfRequestComplete(
                request,
                STATUS_INVALID_PARAMETER
            );

            return;
        }

        report.reportId = MouseReportId;
        report.buttons = command->buttons;
        report.movementX = command->movementX;
        report.movementY = command->movementY;
        report.verticalWheel = command->verticalWheel;
        report.horizontalWheel = command->horizontalWheel;

        transferPacket.reportBuffer =
            reinterpret_cast<PUCHAR>(&report);

        transferPacket.reportBufferLen =
            static_cast<ULONG>(sizeof(report));

        transferPacket.reportId = MouseReportId;

        status = VhfReadReportSubmit(
            deviceContext->vhfHandle,
            &transferPacket
        );

        WdfRequestComplete(request, status);
        return;
    }

    if (ioControlCode ==
        DriverLevelInputSimulator::IoctlSubmitKeyboardReport)
    {
        DriverLevelInputSimulator::KeyboardCommand* command;
        HidKeyboardReport report;

        if (inputBufferLength !=
            sizeof(DriverLevelInputSimulator::KeyboardCommand))
        {
            WdfRequestComplete(
                request,
                STATUS_INVALID_BUFFER_SIZE
            );

            return;
        }

        status = WdfRequestRetrieveInputBuffer(
            request,
            sizeof(DriverLevelInputSimulator::KeyboardCommand),
            reinterpret_cast<PVOID*>(&command),
            nullptr
        );

        if (!NT_SUCCESS(status))
        {
            WdfRequestComplete(request, status);
            return;
        }

        report.reportId = KeyboardReportId;
        report.modifiers = command->modifiers;
        report.reserved = 0;

        for (ULONG index = 0; index < 6; ++index)
        {
            report.keys[index] = command->keys[index];
        }

        transferPacket.reportBuffer =
            reinterpret_cast<PUCHAR>(&report);

        transferPacket.reportBufferLen =
            static_cast<ULONG>(sizeof(report));

        transferPacket.reportId = KeyboardReportId;

        status = VhfReadReportSubmit(
            deviceContext->vhfHandle,
            &transferPacket
        );

        WdfRequestComplete(request, status);
        return;
    }

    WdfRequestComplete(
        request,
        STATUS_INVALID_DEVICE_REQUEST
    );
}
VOID DeviceEvtCleanup(
    WDFOBJECT deviceObject
)
{
    WDFDEVICE device;
    DeviceContext* deviceContext;

    device = reinterpret_cast<WDFDEVICE>(deviceObject);
    deviceContext = GetDeviceContext(device);

    if (deviceContext->vhfHandle != nullptr)
    {
        VhfDelete(
            deviceContext->vhfHandle,
            TRUE
        );

        deviceContext->vhfHandle = nullptr;
    }
}