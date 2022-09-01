#define NTDDI_VERSION NTDDI_WIN7

#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <windows.h>
#include <dshow.h>
#include <comdef.h>
#include <combaseapi.h>
#include <ks.h>
#include <ksproxy.h>


#pragma comment(lib, "strmiids.lib")

DEFINE_GUIDSTRUCT("f6acc829-acdb-e511-8424-f39068f75511", PROPSETID_HUD_XU_V1);
#define PROPSETID_HUD_XU_V1 DEFINE_GUIDNAMED(PROPSETID_HUD_XU_V1)

DEFINE_GUIDSTRUCT("a8bd5df2-1a98-474e-8dd0-d92672d194fa", PROPSETID_HUD_XU_V2);
#define PROPSETID_HUD_XU_V2 DEFINE_GUIDNAMED(PROPSETID_HUD_XU_V2)

#define NODEID_HUD_XU_V1 2
#define NODEID_HUD_XU_V2 3

struct XU_CONTROL
{
    GUID extension_guid;
    int node_id;
    int control_number;
    int size;
public:
    XU_CONTROL(const GUID guid, int node_id, int control_number, int size) :
        extension_guid(guid), node_id(node_id), control_number(control_number), size(size) {}
};

#define XU_SOFTWARE_VERSION XU_CONTROL(PROPSETID_HUD_XU_V1, NODEID_HUD_XU_V1, 19, 8);
#define XU_GENIUS_FRAMING_CONTROL XU_CONTROL(PROPSETID_HUD_XU_V2, NODEID_HUD_XU_V2, 2, 1);
#define XU_PEOPLE_COUNT_CONTROL XU_CONTROL(PROPSETID_HUD_XU_V2, NODEID_HUD_XU_V2, 7, 1);
#define XU_PEOPLE_COUNT_READ XU_CONTROL(PROPSETID_HUD_XU_V2, NODEID_HUD_XU_V2, 8, 1);
#define XU_AUTOZOOM_AVAILABLE    XU_CONTROL(PROPSETID_HUD_XU_V1, NODEID_HUD_XU_V1, 0x9, 1);
#define XU_AUTOZOOM_CONTROL      XU_CONTROL(PROPSETID_HUD_XU_V1, NODEID_HUD_XU_V1, 0xa, 1);
#define XU_AUTOZOOM_MODE_CONTROL XU_CONTROL(PROPSETID_HUD_XU_V1, NODEID_HUD_XU_V1, 0xb, 1);


#define AZ_MODE_NORMAL       0
#define AZ_MODE_GALLERYVIEW  1
#define AZ_MODE_GV_DUPLICATE 2 // this mode is only for testing and will duplicate a single person on both frames

class UVCException : public std::exception {
public:
    HRESULT hr;
    UVCException(std::string message, HRESULT hr) : message(message), hr(hr)
    {
    }
    const std::string message;
};

/*
vid_([^&#]+)       // vendor id
pid_([^&#]+)      // product id
&[^#]*#([^&#]+)    // serial number
*/
static std::string get_property_from_win_path(const std::string& win_path, const std::string& reg_exp)
{
    try {
        std::regex re(reg_exp, std::regex_constants::ECMAScript | std::regex_constants::icase);
        std::smatch m;
        if (std::regex_search(win_path, m, re) && m.size() > 1) {
            std::string res = m.str(1);
            return res;
        }
        else {
            return std::string("");
        }
    }
    catch (std::regex_error& e) {
        return std::string("");
    }
}


static IKsControl* get_one_huddly_control_interface()
{
    IKsControl* huddly_ks_control = NULL;
    HRESULT hr;
    ICreateDevEnum* dev_enum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dev_enum));
    if (FAILED(hr))
    {
        throw UVCException("Failed to create System Device Enum library instance", hr);
    }

    IEnumMoniker* class_enum = NULL;
    hr = dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &class_enum, 0);

    if (FAILED(hr)) 
    {
        throw UVCException("Failed to CreateClassEnumerator", hr);
    }

    if (class_enum != nullptr) 
    {
        IMoniker* moniker = NULL;
        while (class_enum->Next(1, &moniker, NULL) == S_OK) {
            IPropertyBag* prop_bag;
            hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&prop_bag);
            if (SUCCEEDED(hr))
            {
                VARIANT var_name;
                VariantInit(&var_name);
                hr = prop_bag->Read(L"DevicePath", &var_name, 0);
                if (SUCCEEDED(hr))
                {
                    auto win_path = std::string((char*)(_bstr_t)var_name);

                    auto _pid = get_property_from_win_path(win_path, "pid_([^&#]+)");
                    auto _vid = get_property_from_win_path(win_path, "vid_([^&#]+)");

                    if (_vid.empty() || stoul(_vid, nullptr, 16) != 0x2bd9)
                    {
                        //Non-Huddly device
                        continue;
                    }

                    IBaseFilter* base_filter;
                    hr = moniker->BindToObject(0, 0, IID_PPV_ARGS(&base_filter));
                    if (FAILED(hr))
                    {
                        continue;
                    }

                    IKsControl* ks_control;
                    hr = base_filter->QueryInterface(IID_PPV_ARGS(&ks_control));
                    if (SUCCEEDED(hr))
                    {
                        huddly_ks_control = ks_control;
                    }

                }
                VariantClear(&var_name);
                prop_bag->Release();
            }
            moniker->Release();
        }
        class_enum->Release();
    }

    dev_enum->Release();

    return huddly_ks_control;
}


std::vector<uint8_t> get_xu_control(IKsControl* ks_control, XU_CONTROL xu_control)
{
    KSP_NODE extension_prop;

    extension_prop.Property.Set = xu_control.extension_guid;
    extension_prop.Property.Id = xu_control.control_number;
    extension_prop.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    extension_prop.NodeId = xu_control.node_id;

    std::vector<uint8_t> retval(xu_control.size);

    ULONG bytes = xu_control.size;
    HRESULT hr = ks_control->KsProperty(
        (PKSPROPERTY)&extension_prop,
        sizeof(extension_prop),
        &retval[0], //pointer to first element
        static_cast<ULONG>(retval.size()),
        &bytes);
    if (FAILED(hr))
    {
        throw UVCException("Failed to get XU", hr);
    }

    return retval;
}

void set_xu_control(IKsControl* ks_control, XU_CONTROL xu_control, std::vector<uint8_t> data)
{
    KSP_NODE extension_prop;

    extension_prop.Property.Set = xu_control.extension_guid;
    extension_prop.Property.Id = xu_control.control_number;
    extension_prop.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    extension_prop.NodeId = xu_control.node_id;

    ULONG bytes_returned = 0;

    HRESULT hr = ks_control->KsProperty(
        (PKSPROPERTY)&extension_prop,
        sizeof(extension_prop),
        &data[0],
        xu_control.size,
        &bytes_returned);

    if (FAILED(hr))
    {
        throw UVCException("set_xu_control failed!", hr);
    }
}

static std::string get_firmware_version(IKsControl* ksControl)
{
    auto xu = XU_SOFTWARE_VERSION;
    auto ver = get_xu_control(ksControl, xu);

    std::ostringstream version_string;
    version_string << (int)ver[3] << "." << (int)ver[2] << "." << (int)ver[1];
    return version_string.str();
}

static bool get_autozoom_available(IKsControl* ksControl)
{
    auto xu = XU_GENIOUS_FRAMING_AVAILABLE;
    auto available = get_xu_control(ksControl, xu);
    return (bool)available[0];
}

static void set_autozoom(IKsControl* ksControl, bool on)
{
    auto xu = XU_AUTOZOOM_CONTROL;
    uint8_t mode = 1;
    if (!on)
        mode = 0;
    std::vector<uint8_t> m(1, mode);

    set_xu_control(ksControl, xu, m);
}

static bool get_autozoom(IKsControl* ksControl)
{
    auto xu = XU_AUTOZOOM_CONTROL;
    auto people_count = get_xu_control(ksControl, xu);
    return (bool)people_count[0];
}

static void set_autozoom_mode(IKsControl* ksControl, uint8_t mode)
{
    auto xu = XU_AUTOZOOM_MODE_CONTROL;
    std::vector<uint8_t> m(1, mode);

    set_xu_control(ksControl, xu, m);
}

static int get_autozoom_mode(IKsControl* ksControl)
{
    auto xu = XU_AUTOZOOM_MODE_CONTROL;
    auto az_mode = get_xu_control(ksControl, xu);
    return (int)az_mode[0];
}

static void set_people_count_mode(IKsControl* ksControl, uint8_t mode)
{
    auto xu = XU_PEOPLE_COUNT_CONTROL;
    std::vector<uint8_t> m(1, mode);

    set_xu_control(ksControl, xu, m);
}

static int get_people_count(IKsControl* ksControl)
{
    auto xu = XU_PEOPLE_COUNT_READ;
    auto people_count = get_xu_control(ksControl, xu);
    return (int)people_count[0];
}

int main()
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    std::cout << "Enabling people count and printing information from Huddly IQ.." << std::endl;
    IKsControl* huddly_ks_control = get_one_huddly_control_interface();
    if (huddly_ks_control == NULL)
    {
        std::cout << "Ooops. I wasnt able to find any connected Huddly IQ camera" << std::endl;
        return -1;
    }
    auto software_version = get_firmware_version(huddly_ks_control);

    auto az_avail = get_autozoom_available(huddly_ks_control);
    if(!az_available) {
        std::cout << "Genious Framing features unavailable. " << std::endl;
        return -2;
    }

    auto az_enabled = get_autozoom(huddly_ks_control);
    if (!az_enabled) {
        std::cout << "Enabling Genious Framing... " << std::endl;
        set_autozoom(huddly_ks_control, true);
    }

    auto mode = get_autozoom_mode(huddly_ks_control);
    if (mode == AZ_MODE_NORMAL) {
        std::cout << "Genious Framing in Normal mode, changing to Gallery View. " << std::endl;
        set_autozoom_mode(huddly_ks_control, AZ_MODE_GALLERYVIEW);
    } else if (mode == AZ_MODE_GALLERYVIEW) {
        std::cout << "Genious Framing in Gallery View mode, changing to Normal. " << std::endl;
        set_autozoom_mode(huddly_ks_control, AZ_MODE_NORMAL);
    }

    set_people_count_mode(huddly_ks_control, 1);
    Sleep(1000); // Give the camera some time to do its first detection
    auto people_count = get_people_count(huddly_ks_control);

    std::cout << "Software version: " << software_version << std::endl;
    std::cout << "People counted: " << people_count << std::endl;

}
