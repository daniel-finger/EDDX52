// EDDX52.cpp : Defines the exported functions for the DLL application.
// Direct Output Pages are VERY buggy, or better say they don't work. So I had to implement the system by myself (MFD only)
#pragma once

//#ifdef _USRDLL 
//#define EDD_API __declspec(dllexport)
//#else
//#define EDD_API __declspec(dllimport)
//#endif  

#include "stdafx.h"
#include "EDDInterface64.h"
#include <fstream>
#include <thread>
#include <stdlib.h>
#include <comdef.h>

extern "C" {
	typedef void __stdcall Pfn_DirectOutput_Device_Callback(void* hDevice, bool bAdded, void* pvContext);
	typedef void __stdcall Pfn_DirectOutput_EnumerateCallback(void* hDevice, void* pvContext);
	typedef void __stdcall Pfn_DirectOutput_SoftButton_Callback(void* hDevice, DWORD dwButtons, void* pvContext);
	typedef void __stdcall Pfn_DirectOutput_Page_Callback(void* hDevice, DWORD dwPage, bool bActivated, void* pvContext);

	const DWORD SoftButton_Select = 0x00000001; // X52Pro ScrollClick
	const DWORD SoftButton_Up = 0x00000002; // X52Pro ScrollUp, FIP RightScrollClockwize
	const DWORD SoftButton_Down = 0x00000004; // X52Pro ScrollDown, FIP RightScrollAnticlockwize

	typedef HRESULT (__stdcall *DirectOutput_Initialize)(const wchar_t * wszAppName);
	const char * do_initFun = "DirectOutput_Initialize";
	typedef HRESULT (__stdcall *DirectOutput_Deinitialize)();
	const char * do_deInitFun = "DirectOutput_Deinitialize";

	typedef HRESULT (__stdcall *DirectOutput_RegisterDeviceCallback)(Pfn_DirectOutput_Device_Callback pfnCallback, void* pvParam);
	const char * do_regDevCallbackFun = "DirectOutput_RegisterDeviceCallback";
	typedef HRESULT (*DirectOutput_Enumerate)(Pfn_DirectOutput_EnumerateCallback pfnCallback, void* pvParam);
	const char * do_enumCallbackFun = "DirectOutput_Enumerate";

	typedef HRESULT (*DirectOutput_RegisterSoftButtonCallback)(void* hDevice, Pfn_DirectOutput_SoftButton_Callback pfnCallback, void* pvContext);
	const char * do_regSoftBtnCallbackFun = "DirectOutput_RegisterSoftButtonCallback";
	typedef HRESULT (*DirectOutput_RegisterPageCallback)(void* hDevice, Pfn_DirectOutput_Page_Callback pfnCallback, void* pvContext);
	const char * do_regPageCallback = "DirectOutput_RegisterPageCallback";

	typedef HRESULT (*DirectOutput_AddPage)(void* hDevice, DWORD dwPage, DWORD dwFlags);
	const char * do_addPageFun = "DirectOutput_AddPage";
	typedef HRESULT (*DirectOutput_RemovePage)(void* hDevice, DWORD dwPage);
	const char * do_removePageFun = "DirectOutput_RemovePage";

	typedef HRESULT (*DirectOutput_SetLed)(void* hDevice, DWORD dwPage, DWORD dwIndex, DWORD dwValue);
	const char * do_setLedFun = "DirectOutput_SetLed";
	typedef HRESULT (*DirectOutput_SetString)(void* hDevice, DWORD dwPage, DWORD dwIndex, DWORD cchValue, const wchar_t* wszValue);
	const char * do_setStrFun = "DirectOutput_SetString";
}

HINSTANCE directOutputDll;
DirectOutput_AddPage addPage;
DirectOutput_RegisterSoftButtonCallback registerSoftButtonCallback;
DirectOutput_RegisterPageCallback registerPageCallback;
DirectOutput_SetLed setLed;
DirectOutput_SetString setString;
DirectOutput_RemovePage removePage;
wchar_t * callbackActionName = nullptr;

EDDCallBacks callbacks;

const DWORD FLAG_SET_AS_ACTIVE = 0x00000001;

const int ledCount = 20;

const int simpleLedCount = 11;
struct ButtonLed {
	const wchar_t * name;
	DWORD redCompName;
	DWORD greenCompName;
};
const ButtonLed simpleLeds[simpleLedCount] = {
	{ L"FIRE", 0, 0 },
	{ L"FIRE_A", 1, 2 },
	{ L"FIRE_B", 3, 4 },
	{ L"FIRE_D", 5, 6 },
	{ L"FIRE_E", 7, 8 },
	{ L"TOGGLE_1_2", 9, 10 },
    { L"TOGGLE_3_4", 11, 12 },
    { L"TOGGLE_5_6", 13, 14 },
    { L"POV_2", 15, 16 },
    { L"CLUTCH", 17, 18 },
    { L"THROTTLE", 19, 19 }
};

const int simpleLedStatesCount = 5;
struct LedState {
	const wchar_t * name;
	DWORD stateRed;
	DWORD stateGreen;
};
const LedState simpleLedStates[simpleLedStatesCount]{
	{L"ON", 1, 1},
    {L"OFF", 0, 0},
    {L"RED", 1, 0},
    {L"GREEN", 0, 1},
    {L"YELLOW", 1, 1}
};

const int linesCount = 3;
const wchar_t * lines[linesCount] = {
	L"TOP",
	L"MIDDLE",
	L"BOTTOM"
};

const BSTR setLEDCommand = ::SysAllocString(L"setLED");
const BSTR setLEDAdvancedCommand = ::SysAllocString(L"advSetLED");
const BSTR setStringCommand = ::SysAllocString(L"setString");
const BSTR getLinesCountCommand = ::SysAllocString(L"getLinesCount");
const BSTR resetMFDCommand = ::SysAllocString(L"resetMFD");
const BSTR setLineCommand = ::SysAllocString(L"setLine");
const BSTR resetLedCommand = ::SysAllocString(L"resetLED");
const BSTR resetCommand = ::SysAllocString(L"reset");
const BSTR turnAllLedOffCommand = ::SysAllocString(L"turnAllLEDOff");

const BSTR callbackEventName = ::SysAllocString(L"EDDX52_SoftButtonSelect");

const BSTR errorTooFewParam = ::SysAllocString(L"TOO_FEW_PARAM");
const BSTR errorInvalidAct = ::SysAllocString(L"INVALID_ACTION ");
const BSTR errorLedNotFound = ::SysAllocString(L"LED_NOT_FOUND");
const BSTR errorStateNotFound = ::SysAllocString(L"LED_STATE_NOT_FOUND");
const BSTR success = ::SysAllocString(L"SUCCESS");

const char * version = "1.0";
const BSTR bsVersion = ::SysAllocString(L"1.0");
const wchar_t * name = L"EDDX52";

const _bstr_t libDependencies = "\\DirectOutput.dll";

const int workPage = 0;
int mfd_line = 0;
void * activeDevice;

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

DWORD ledStates[ledCount] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};

struct mfd_entry {
	BSTR name;
	BSTR callbackName;
};
std::vector<mfd_entry> mfd_text;

void turnAllLedOff() {
	for (int i = 0; i < ledCount; i++) {
		setLed(activeDevice, workPage, i, 0);
	}
}

void WriteASCII(const char* str)		// ASCII file..
{
	std::fstream fstream("c:\\code\\eddif.txt", std::ios::app);         // open the file
	fstream << str;
	fstream.close();
}
void WriteUnicode(LPCTSTR buffer)
{
	char buffer2[30000];
	WideCharToMultiByte(CP_ACP, 0, buffer, -1, buffer2, sizeof(buffer2), 0, 0);		// need to convert back to ASCII
	WriteASCII((const char*)buffer2);
}

//void changeVirtualPage(int newPage) {
//	if (newPage >= 0 && newPage < mfd_text.size()) {
//		//activeVirtualPage = newPage;
//
//		for (int i = 0; i < linesCount; i++) {
//			//setString(activeDevice, workPage, i, (DWORD)wcslen(mfd_text[activeVirtualPage][i]), mfd_text[activeVirtualPage][i]);
//		}
//	}
//}

void scrollMfd(int newLine) {
	if (newLine >= 0 && newLine < (int) (mfd_text.size())) {
		mfd_line = newLine;

		for (int i = 0; i < 3; i++) {
			if (mfd_line + i >= 0 && mfd_line + i < mfd_text.size())
			    setString(activeDevice, workPage, i, (DWORD)wcslen(mfd_text[mfd_line + i].name), mfd_text[mfd_line + i].name);
			else
				setString(activeDevice, workPage, i, 0, L"");
		}
	}
}

void __stdcall DirectOutput_Device_Callback(void* hDevice, bool bAdded, void* pvContext) {
	if (bAdded) {
		activeDevice = hDevice;
	}
	else {
		activeDevice = nullptr;
	}
}

void __stdcall DirectOutput_SoftButton_Callback(void* hDevice, DWORD dwButtons, void* pvContext) {
	if (dwButtons == SoftButton_Up) {
		scrollMfd(mfd_line - 1);
		//changeVirtualPage(activeVirtualPage + 1);
	}
	else if (dwButtons == SoftButton_Down) {
		scrollMfd(mfd_line + 1);
		//changeVirtualPage(activeVirtualPage - 1);
	}
	else if (dwButtons == SoftButton_Select) {
		callbacks.RunAction(mfd_text[mfd_line].callbackName, ::SysAllocString(L""));
	}
}

void __stdcall DirectOutput_Page_Callback(void* hDevice, DWORD dwPage, bool bActivated, void* pvContext) {
	if (bActivated && workPage == dwPage) {
		//Stuff needs to be implemented manually, because DirectOutput does not work properly, yet forces the use of pages. The problem is, when you change page, everything gets erased.
		for (int i = 0; i < ledCount; i++) {
			setLed(activeDevice, dwPage, i, ledStates[i]);
		}

		for (int i = 0; i < linesCount; i++) {
			//setString(activeDevice, workPage, i, (DWORD)wcslen(mfd_text[activeVirtualPage][i]), mfd_text[activeVirtualPage][i]);
		}
	}
}

void __stdcall DirectOutput_Enumerate_Callback(void* hDevice, void* pvContext) {
	activeDevice = hDevice;

	registerPageCallback(hDevice, *DirectOutput_Page_Callback, nullptr);
	registerSoftButtonCallback(hDevice, *DirectOutput_SoftButton_Callback, nullptr);
}

EDD_API BSTR __cdecl EDDInitialise(BSTR ver, BSTR folder, EDDCallBacks pCallbacks) {
	WriteASCII("EDDX52 Version: ");
	WriteASCII(version);
	WriteASCII("\n");

	HINSTANCE directOutputDll = LoadLibrary(folder + libDependencies);
	if (directOutputDll) {
		DirectOutput_Initialize init = (DirectOutput_Initialize)GetProcAddress(directOutputDll, do_initFun);
		init(name);

		registerSoftButtonCallback = (DirectOutput_RegisterSoftButtonCallback)GetProcAddress(directOutputDll, do_regSoftBtnCallbackFun);
		registerPageCallback = (DirectOutput_RegisterPageCallback)GetProcAddress(directOutputDll, do_regPageCallback);

		DirectOutput_RegisterDeviceCallback regDev = (DirectOutput_RegisterDeviceCallback)GetProcAddress(directOutputDll, do_regDevCallbackFun);
		regDev(*DirectOutput_Device_Callback, nullptr);
		DirectOutput_Enumerate enumerate = (DirectOutput_Enumerate)GetProcAddress(directOutputDll, do_enumCallbackFun);
		enumerate(*DirectOutput_Enumerate_Callback, nullptr);

		addPage = (DirectOutput_AddPage)GetProcAddress(directOutputDll, do_addPageFun);
		addPage(activeDevice, workPage, FLAG_SET_AS_ACTIVE);

		removePage = (DirectOutput_RemovePage)GetProcAddress(directOutputDll, do_removePageFun);

		
		setLed = (DirectOutput_SetLed)GetProcAddress(directOutputDll, do_setLedFun);
		setString = (DirectOutput_SetString)GetProcAddress(directOutputDll, do_setStrFun);

		turnAllLedOff();

		callbacks = pCallbacks;
	}
	else {
		return NULL;
	}
    
	return bsVersion;
}

EDD_API BSTR __cdecl EDDActionCommand(BSTR action, SAFEARRAY& args) {
	     if (0 == wcscmp(action, setLEDAdvancedCommand)) {
		wchar_t * led;
		wchar_t * state;
		long index1 = 0;
		long index2 = 1;
		if (::SafeArrayGetElement(&args, &index1, &led) != S_OK ||
			::SafeArrayGetElement(&args, &index2, &state) != S_OK) {
			return errorTooFewParam;
		}

		DWORD dwLed = _wtol(led);
		DWORD dwState = _wtol(state);
		
		ledStates[dwLed] = dwState;
		setLed(activeDevice, workPage, dwLed, dwState);

		return success;
	}
	else if (0 == wcscmp(action, setLEDCommand)) {
		wchar_t * chLed;
		wchar_t * chState;
		long index1 = 0;
		long index2 = 1;
		if (::SafeArrayGetElement(&args, &index1, &chLed) != S_OK ||
			::SafeArrayGetElement(&args, &index2, &chState) != S_OK) {
			return errorTooFewParam;
		}

		const ButtonLed * led = nullptr;
		const LedState * state = nullptr;

		bool ledFound = false;
		bool stateFound = false;

		for (int i = 0; i < simpleLedStatesCount; i++) {
			if (0 == wcscmp(simpleLedStates[i].name, chState)) {
				state = &simpleLedStates[i];
				stateFound = true;
				break;
			}
		}

		for (int i = 0; i < simpleLedCount; i++) {
			if (0 == wcscmp(simpleLeds[i].name, chLed)) {
				led = &simpleLeds[i];
				ledFound = true;
				break;
			}
		}

		if (!stateFound)
			return errorStateNotFound;
		if (!ledFound)
			return errorLedNotFound;

		ledStates[led->greenCompName] = state->stateGreen;
		ledStates[led->redCompName] = state->stateRed;
		setLed(activeDevice, workPage, led->redCompName, state->stateRed);
		setLed(activeDevice, workPage, led->greenCompName, state->stateGreen);

		return success;
	}
	else if (0 == wcscmp(action, setStringCommand)) {
		wchar_t * line;
		wchar_t * text;
		wchar_t * callback;
		long index1 = 0;
		long index2 = 1;
		long index3 = 2;
		if (::SafeArrayGetElement(&args, &index1, &line) != S_OK ||
			::SafeArrayGetElement(&args, &index2, &text) != S_OK ||
		    ::SafeArrayGetElement(&args, &index3, &callback) != S_OK) {
			return errorTooFewParam;
		}

		DWORD dwLine = _wtol(line);

		int x = (dwLine - (int)mfd_text.size());

		for (int i = 0; i <= x; i++) {
			mfd_text.push_back(mfd_entry { BSTR(L""), BSTR(L"") });
		}

		//for (int i = 0; i < linesCount; i++) {
		//	if (lines[i] == line) {
		//		dwLine = i;
		//		break;
		//}

		if (dwLine >= mfd_line && dwLine < mfd_line + 3)
			setString(activeDevice, workPage, dwLine - mfd_line, (DWORD) wcslen(text), text);
		mfd_text[dwLine].name = text;
		mfd_text[dwLine].callbackName = callback;
		//scroll(mfd_line);

		return success;
	}
	else if (0 == wcscmp(action, getLinesCountCommand)) {
		OLECHAR size = (OLECHAR) mfd_text.size();
		return ::SysAllocString(&size);
	}
	else if (0 == wcscmp(action, resetMFDCommand)) {
		mfd_line = 0;
		mfd_text.clear();
		setString(activeDevice, workPage, 0, 0, L"");
		setString(activeDevice, workPage, 1, 0, L"");
		setString(activeDevice, workPage, 2, 0, L"");

		return success;
	}
	else if (0 == wcscmp(action, setLineCommand)) {
		wchar_t * line;
		long index1 = 0;
		if (::SafeArrayGetElement(&args, &index1, &line) != S_OK)
			return errorTooFewParam;
		int dwLine = _wtol(line);
		scrollMfd(dwLine);
		return success;
	}
	else if (0 == wcscmp(action, resetLedCommand)) {
		turnAllLedOff();
		return success;
	}
	else if (0 == wcscmp(action, resetCommand)) {
		mfd_line = 0;
		mfd_text.clear();
		setString(activeDevice, workPage, 0, 0, L"");
		setString(activeDevice, workPage, 1, 0, L"");
		setString(activeDevice, workPage, 2, 0, L"");

		turnAllLedOff();

		return success;
	}
	else if (0 == wcscmp(action, turnAllLedOffCommand)) {
		turnAllLedOff();
		return success;
	}

	return errorInvalidAct;
}

//EDD_API void __cdecl EDDTerminate()
//{
//	WriteASCII("Goodbye from EDDX52");
//	DirectOutput_Deinitialize deinit = (DirectOutput_Deinitialize)GetProcAddress(directOutputDll, do_deInitFun);
//	deinit();
//}