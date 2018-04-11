#include <nan.h>

#include <cfgmgr32.h>
#include <devguid.h>
#include <bluetoothapis.h>
#include <SetupAPI.h>
#include <sstream>

#pragma comment(lib, "setupapi")

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

bool IsBLECapable(DEVINST deviceInstance) 
{
    DEVINST devInst = 0;
    DEVINST devNext = 0;
    CONFIGRET cr = CM_Get_Child(&devNext, deviceInstance, 0);
    if (cr != CR_SUCCESS)
    {
        return false;
    }
    devInst = devNext;

    TCHAR buf[512];
    DWORD dataType;
    DWORD size = 512;
    while (true)
    {
        cr = CM_Get_DevNode_Registry_Property(devInst, CM_DRP_DEVICEDESC, &dataType, buf, &size, 0);
        if (cr == CR_SUCCESS && strcmp(buf, "Microsoft Bluetooth LE Enumerator") == 0)
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
    return false;
}

#define PROP_STRING(obj, type, name, value) \
        do \
        { \
           v8::Local<v8::String> prop_name = Nan::New<v8::String>(name).ToLocalChecked(); \
           v8::Local<type> prop_value = Nan::New<type>(value).ToLocalChecked(); \
           Nan::Set(obj, prop_name, prop_value); \
        } while(0)


#define PROP(obj, type, name, value) \
        do \
        { \
           v8::Local<v8::String> prop_name = Nan::New<v8::String>(name).ToLocalChecked(); \
           v8::Local<type> prop_value = Nan::New<type>(value); \
           Nan::Set(obj, prop_name, prop_value); \
        } while(0)

NAN_METHOD(list) 
{
    v8::Local<v8::Array> radioList = Nan::New<v8::Array>();
    int radioIndex = 0;
    HDEVINFO hDevInfo = SetupDiGetClassDevsEx(&GUID_BTHPORT_DEVICE_INTERFACE, nullptr, nullptr,
                                              DIGCF_DEVICEINTERFACE | DIGCF_PRESENT, nullptr, nullptr, nullptr);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        info.GetReturnValue().Set(radioList);
        return;
    }
    SP_DEVINFO_DATA devInfo;
    devInfo.cbSize = sizeof(devInfo);
    for (DWORD devIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devIndex, &devInfo) != FALSE; devIndex++)
    {
        std::string name = GetProperty(hDevInfo, &devInfo, SPDRP_DEVICEDESC);
        std::string manufacturer = GetProperty(hDevInfo, &devInfo, SPDRP_MFG);
        bool isBLECapable = IsBLECapable(devInfo.DevInst);
        v8::Local<v8::Object> adapter = Nan::New<v8::Object>();
        PROP_STRING(adapter, v8::String, "name", name);
        PROP_STRING(adapter, v8::String, "manufacturer", manufacturer);
        PROP(adapter, v8::Boolean, "bleCapable", isBLECapable);
        Nan::Set(radioList, radioIndex, adapter);
        radioIndex++;
    }
    // clean up the device information set
    SetupDiDestroyDeviceInfoList(hDevInfo);
    info.GetReturnValue().Set(radioList);
}

NAN_MODULE_INIT(Init) 
{
  NAN_EXPORT(target, list);
}

NODE_MODULE(method, Init);
