#include <napi.h>
#include <uv.h>

#define INITGUID
#include <cfgmgr32.h>
#include <devguid.h>
#include <bluetoothapis.h>
#include <SetupAPI.h>
#include <sstream>
#include <initguid.h>

#pragma comment(lib, "setupapi")

typedef BOOL (WINAPI *FN_SetupDiGetDevicePropertyW)(
  __in       HDEVINFO DeviceInfoSet,
  __in       PSP_DEVINFO_DATA DeviceInfoData,
  __in       const DEVPROPKEY *PropertyKey,
  __out      DEVPROPTYPE *PropertyType,
  __out_opt  PBYTE PropertyBuffer,
  __in       DWORD PropertyBufferSize,
  __out_opt  PDWORD RequiredSize,
  __in       DWORD Flags
);

// not sure if the association is right, maybe LMP version is 6 and HCIversion is 4
DEFINE_DEVPROPKEY(DEVPKEY_DeviceContainer_BluetoothRadioLMPVersion, 0xa92f26ca, 0xeda7, 0x4b1d,
    0x9d, 0xb2, 0x27, 0xb6, 0x8a, 0xa5, 0xa2, 0xeb, 4); // BYTE
DEFINE_DEVPROPKEY(DEVPKEY_DeviceContainer_BluetoothRadioHCIVersion, 0xa92f26ca, 0xeda7, 0x4b1d,
    0x9d, 0xb2, 0x27, 0xb6, 0x8a, 0xa5, 0xa2, 0xeb, 6); // BYTE

BYTE GetByteProperty(HDEVINFO hDevInfo, PSP_DEVINFO_DATA devInfo, DEVPROPKEY key, BYTE defaultValue)
{
    FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
        GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");

    DEVPROPTYPE devicePropertyType;
    DWORD required = 0;
    BOOL success = fn_SetupDiGetDevicePropertyW && fn_SetupDiGetDevicePropertyW(hDevInfo, devInfo, &key, &devicePropertyType, nullptr,
        0, &required, 0);
    if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return defaultValue;
    }

    std::vector<WCHAR> data(required);
    if (!fn_SetupDiGetDevicePropertyW || !fn_SetupDiGetDevicePropertyW(hDevInfo, devInfo, &key, &devicePropertyType,
        reinterpret_cast<BYTE*>(&data[0]), required, nullptr, 0))
    {
        return defaultValue;
    }
    if (devicePropertyType == DEVPROP_TYPE_BYTE)
    {
        return *reinterpret_cast<BYTE*>(&data[0]);
    }
    return defaultValue;
}

static std::string GetProperty(HDEVINFO hDevInfo, PSP_DEVINFO_DATA deviceInfoData, DWORD property)
{
    DWORD buffersize = 0;
    DWORD dataType;

    BOOL result = SetupDiGetDeviceRegistryProperty(hDevInfo, deviceInfoData, property, nullptr,
        nullptr, 0, &buffersize);
    // we should get an insufficent buffer error here in order for buffersize to be set
    if (result == TRUE || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        return std::string();
    }

    std::vector<char> buffer(buffersize);
    result =
        SetupDiGetDeviceRegistryProperty(hDevInfo, deviceInfoData, property, &dataType,
            reinterpret_cast<PBYTE>(&buffer[0]), buffersize, nullptr);
    if (result == FALSE)
    {
        return std::string();
    }
    std::string ret;
    switch (dataType)
    {
    case REG_SZ:
    {
        ret = std::string(&buffer[0]);
        break;
    }
    case REG_MULTI_SZ:
    {
        TCHAR* start = &buffer[0];
        std::stringstream wss1;
        wss1.write(start, lstrlen(start));
        start += lstrlen(start) + 1;
        while (*start != '\0')
        {
            wss1 << std::endl;
            wss1.write(start, lstrlen(start));
            start += lstrlen(start) + 1;
        }
        ret = wss1.str();
        break;
    }
    case REG_DWORD:
    {
        std::stringstream wss2;
        wss2 << *reinterpret_cast<PDWORD>(&buffer[0]);
        ret = wss2.str();
        break;
    }
    case REG_QWORD:
    {
        std::stringstream wss3;
        wss3 << *reinterpret_cast<uint64_t*>(&buffer[0]);
        ret = wss3.str();
        break;
    }
    default:
        ret = "not handled";
        break;
    }
    return ret;
}

static std::string GetProperty(DEVINST devInst, DWORD property)
{
    DWORD buffersize = 0;
    DWORD dataType;

    CONFIGRET result = CM_Get_DevNode_Registry_Property(devInst, property, &dataType, nullptr, &buffersize, 0);
    // we should get an insufficent buffer error here in order for buffersize to be set
    if (result != CR_BUFFER_SMALL)
    {
        return std::string();
    }

    std::vector<char> buffer(buffersize);
    result = CM_Get_DevNode_Registry_Property(devInst, property, &dataType, &buffer[0], &buffersize, 0);
    if (result != CR_SUCCESS)
    {
        return std::string();
    }
    std::string ret;
    switch (dataType)
    {
        case REG_SZ:
        {
            ret = std::string(&buffer[0]);
            break;
        }
        case REG_MULTI_SZ:
        {
            TCHAR* start = &buffer[0];
            std::stringstream wss1;
            wss1.write(start, lstrlen(start));
            start += lstrlen(start) + 1;
            while (*start != '\0')
            {
                wss1 << std::endl;
                wss1.write(start, lstrlen(start));
                start += lstrlen(start) + 1;
            }
            ret = wss1.str();
            break;
        }
        case REG_DWORD:
        {
            std::stringstream wss2;
            wss2 << *reinterpret_cast<PDWORD>(&buffer[0]);
            ret = wss2.str();
            break;
        }
        case REG_QWORD:
        {
            std::stringstream wss3;
            wss3 << *reinterpret_cast<uint64_t*>(&buffer[0]);
            ret = wss3.str();
            break;
        }
        default:
            ret = "not handled";
            break;
    }
    return ret;
}

bool IsBLECapable(HDEVINFO hDevInfo, PSP_DEVINFO_DATA devInfo)
{
    DEVINST devInst = 0;
    DEVINST devNext = 0;
    CONFIGRET cr = CM_Get_Child(&devNext, devInfo->DevInst, 0);
    // first check if the bluetooth le enumerator is available
    if (cr == CR_SUCCESS)
    {
        devInst = devNext;
        while (true)
        {
            std::string hardwareID = GetProperty(devInst, CM_DRP_HARDWAREID);
            std::string service = GetProperty(devInst, CM_DRP_SERVICE);
            if (hardwareID == "BTH\\MS_BTHLE" && service == "BthLEEnum")
            {
                return true;
            }
            cr = CM_Get_Sibling(&devNext, devInst, 0);
            if (cr != CR_SUCCESS)
            {
                break;
            }
            devInst = devNext;
        }
    }

    BYTE lmpVersion =
        GetByteProperty(hDevInfo, devInfo, DEVPKEY_DeviceContainer_BluetoothRadioLMPVersion, 0);
    BYTE hciVersion =
        GetByteProperty(hDevInfo, devInfo, DEVPKEY_DeviceContainer_BluetoothRadioHCIVersion, 0);
    return lmpVersion >= 6 && hciVersion >= 6;
}

bool IsConnected(const SP_DEVINFO_DATA& devInfo, bool* hasProblem)
{
    *hasProblem = false;

    DWORD status = 0; // DN_ eg. DN_HAS_PROBLEM
    DWORD problemNumber = 0; // CM_PROB_ eg. CM_PROB_FAILED_START
    DWORD ret = CM_Get_DevNode_Status(&status, &problemNumber, devInfo.DevInst, 0);
    if ((status & DN_HAS_PROBLEM) != 0)
    {
        *hasProblem = true;
    }
    return ret == CR_SUCCESS;
}

Napi::Value list(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::Array radioList = Napi::Array::New(env);
    int radioIndex = 0;
    HDEVINFO hDevInfo = SetupDiGetClassDevsEx(&GUID_BTHPORT_DEVICE_INTERFACE, nullptr, nullptr,
                                              DIGCF_DEVICEINTERFACE, nullptr, nullptr, nullptr);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiGetClassDevsEx failed with error %d\n", GetLastError());
        return radioList;
    }
    SP_DEVINFO_DATA devInfo;
    devInfo.cbSize = sizeof(devInfo);
    for (DWORD devIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devIndex, &devInfo) != FALSE; devIndex++)
    {
        bool hasProblem = false;
        bool isConnected = IsConnected(devInfo, &hasProblem);
        std::string name = GetProperty(hDevInfo, &devInfo, SPDRP_DEVICEDESC);
        std::string manufacturer = GetProperty(hDevInfo, &devInfo, SPDRP_MFG);
        bool isBLECapable = IsBLECapable(hDevInfo, &devInfo);
        Napi::Object adapter = Napi::Object::New(env);
        adapter.Set("name", name);
        adapter.Set("manufacturer", manufacturer);
        adapter.Set("bleCapable", isBLECapable);
        adapter.Set("isConnected", isConnected);
        adapter.Set("hasProblem", hasProblem);
        (radioList).Set(radioIndex, adapter);
        radioIndex++;
    }
    // clean up the device information set
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return radioList;
}


Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "list"),
        Napi::Function::New(env, list));
    return exports;
}

NODE_API_MODULE(method, Init);
