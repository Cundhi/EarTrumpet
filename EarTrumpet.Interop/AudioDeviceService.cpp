#include "common.h"
#include "Mmdeviceapi.h"
#include "AudioDeviceService.h"
#include "Functiondiscoverykeys_devpkey.h"
#include "PolicyConfig.h"
#include "Propidl.h"

using namespace std;
using namespace std::tr1;
using namespace EarTrumpet::Interop;

AudioDeviceService* AudioDeviceService::__instance = nullptr;

void AudioDeviceService::CleanUpAudioDevices()
{
    for (auto device = _devices.begin(); device != _devices.end(); device++)
    {
		CoTaskMemFree(device->Id);
		CoTaskMemFree(device->DisplayName);
    }

    _devices.clear();
}

HRESULT AudioDeviceService::RefreshAudioDevices()
{
    CleanUpAudioDevices();

    CComPtr<IMMDeviceEnumerator> deviceEnumerator;
    FAST_FAIL(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&deviceEnumerator)));

    CComPtr<IMMDeviceCollection> deviceCollection;
    FAST_FAIL(deviceEnumerator->EnumAudioEndpoints(EDataFlow::eRender, ERole::eConsole, &deviceCollection));

    UINT numDevices;
    FAST_FAIL(deviceCollection->GetCount(&numDevices));

    for (UINT i = 0; i < numDevices; i++)
    {
        CComPtr<IMMDevice> device;
        if (FAILED(deviceCollection->Item(i, &device)))
        {
            continue;
        }

		CComHeapPtr<wchar_t> deviceId;
		FAST_FAIL(device->GetId(&deviceId));

		CComPtr<IPropertyStore> propertyStore;
		FAST_FAIL(device->OpenPropertyStore(STGM_READ, &propertyStore));

		PROPVARIANT friendlyName;
		PropVariantInit(&friendlyName);
		FAST_FAIL(propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName));
		
		EarTrumpetAudioDevice audioDevice = {};
		FAST_FAIL(SHStrDup(friendlyName.pwszVal, &audioDevice.DisplayName));
		FAST_FAIL(SHStrDup(deviceId, &audioDevice.Id));
		
		_devices.push_back(audioDevice);

		PropVariantClear(&friendlyName);
    }

    return S_OK;
}

HRESULT AudioDeviceService::SetDefaultAudioDevice(LPWSTR deviceId)
{
	CComPtr<IPolicyConfig> policyConfig;
	FAST_FAIL(CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&policyConfig)));
	FAST_FAIL(policyConfig->SetDefaultEndpoint(deviceId, ERole::eConsole));
	return policyConfig->SetDefaultEndpoint(deviceId, ERole::eMultimedia);
}

HRESULT AudioDeviceService::GetAudioDevices(void** audioDevices)
{
	if (_devices.size() == 0)
	{
		return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
	}

	*audioDevices = &_devices[0];
	return S_OK;
}

int AudioDeviceService::GetAudioDeviceCount()
{
	return _devices.size();
}