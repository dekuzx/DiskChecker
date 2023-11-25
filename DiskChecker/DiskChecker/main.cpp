#include <iostream>
#include <windows.h>
#include <wbemidl.h>
#include <comutil.h> 
#include <tchar.h>

#pragma comment(lib, "wbemuuid.lib")

using namespace std;

void UsnJrnl_Date_Created(IWbemServices* pSvc);

int main() {

    try {

        IWbemLocator* pLoc = NULL;

        IWbemServices* pSvc = NULL;


        HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);

        if (FAILED(hres)) {

            throw std::runtime_error("Failed to initialize COM library");

        }


        hres = CoInitializeSecurity(

            NULL,

            -1,

            NULL,

            NULL,

            RPC_C_AUTHN_LEVEL_DEFAULT,

            RPC_C_IMP_LEVEL_IMPERSONATE,

            NULL,

            EOAC_NONE,

            NULL
        
        );

        if (FAILED(hres)) {

            CoUninitialize();

            throw std::runtime_error("Failed to initialize security");

        }


        hres = CoCreateInstance(

            CLSID_WbemLocator,

            0,

            CLSCTX_INPROC_SERVER,

            IID_IWbemLocator,

            (LPVOID*)&pLoc

        );

        if (FAILED(hres)) {

            CoUninitialize();

            throw std::runtime_error("Failed to create IWbemLocator object");
        }


        hres = pLoc->ConnectServer(

            _bstr_t(L"ROOT\\CIMV2"),

            NULL,

            NULL,

            0,

            NULL,

            0,

            0,

            &pSvc

        );

        if (FAILED(hres)) {

            pLoc->Release();

            CoUninitialize();

            throw std::runtime_error("Could not connect to WMI server");

        }


        hres = CoSetProxyBlanket(

            pSvc,

            RPC_C_AUTHN_WINNT,

            RPC_C_AUTHZ_NONE,

            NULL,

            RPC_C_AUTHN_LEVEL_CALL,

            RPC_C_IMP_LEVEL_IMPERSONATE,

            NULL,

            EOAC_NONE

        );

        if (FAILED(hres)) {

            pSvc->Release();

            pLoc->Release();

            CoUninitialize();

            throw std::runtime_error("Could not set proxy blanket");

        }


        UsnJrnl_Date_Created(pSvc);


        pSvc->Release();

        pLoc->Release();

        CoUninitialize();

    }
    catch (std::exception& e) {

        std::cerr << "Error: " << e.what() << std::endl;

        return 1;

    }

    return 0;

}

void UsnJrnl_Date_Created(IWbemServices* pSvc) {

    IEnumWbemClassObject* pEnumerator = NULL;


    HRESULT hres = pSvc->ExecQuery(

        bstr_t("WQL"),

        bstr_t("SELECT * FROM Win32_LogicalDisk"),

        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,

        NULL,

        &pEnumerator

    );

    if (FAILED(hres)) {

        pSvc->Release();

        CoUninitialize();

        throw std::runtime_error("Query for Win32_LogicalDisk failed");

    }

    IWbemClassObject* pclsObj = NULL;

    ULONG uReturn = 0;


    while (pEnumerator) {

        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);


        if (0 == uReturn) {

            break;

        }

        VARIANT vtProp;



        hres = pclsObj->Get(L"DeviceID", 0, &vtProp, NULL, NULL);

        wcout << L"DeviceID : " << vtProp.bstrVal << endl;



        hres = pclsObj->Get(L"FileSystem", 0, &vtProp, NULL, NULL);

        wcout << L"FileSystem : " << vtProp.bstrVal << endl;


        if (wcscmp(vtProp.bstrVal, L"NTFS") == 0) {

            std::wstring command = L"fsutil file layout ";

            command += vtProp.bstrVal;

            command += L"\\$Extend\\$UsnJrnl:$J";


            FILE* fp = _wpopen(command.c_str(), L"rt");

            if (fp) {

                wchar_t buffer[256];

                while (fgetws(buffer, sizeof(buffer) / sizeof(wchar_t), fp)) {

                    std::wstring line(buffer);

                    size_t found = line.find(L"Creation Time");

                    if (found != std::wstring::npos) {

                        std::wstring creationTime = line.substr(found + 14);

                        wcout << L"Creation Time : " << creationTime << L" (UTC)" << endl;

                        break;

                    }

                }

                _pclose(fp);

            }

        }

        else {

            wcout << L"File system is not NTFS" << endl;

        }

        VariantClear(&vtProp);


        pclsObj->Release();

    }


    pEnumerator->Release();

    std::cin.get();

}
