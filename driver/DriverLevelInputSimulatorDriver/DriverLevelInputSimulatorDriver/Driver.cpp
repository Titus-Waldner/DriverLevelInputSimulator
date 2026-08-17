#include <ntddk.h>
#include <wdf.h>

extern "C"
{
    DRIVER_INITIALIZE DriverEntry;
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
        WDF_NO_EVENT_CALLBACK
    );

    return WdfDriverCreate(
        driverObject,
        registryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &driverConfig,
        WDF_NO_HANDLE
    );
}