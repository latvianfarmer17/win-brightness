#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include <CommCtrl.h>
#include <HighLevelMonitorConfigurationAPI.h>

#define LB_MONITORS_ID 101
#define LB_BRIGHTNESS_ID 102
#define CB_MONITORS_ID 103
#define TB_BRIGHTNESS_ID 104

DWORD numMonitors;
LPPHYSICAL_MONITOR monitors;
LPWORD brightnesses;

inline BOOL FindMonitors(HMONITOR hMonitor) {
	if (hMonitor == NULL) {
		MessageBox(NULL, "Could not get monitor from window!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numMonitors)) {
		MessageBox(NULL, "Could not get the number of physical monitors!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	monitors = (LPPHYSICAL_MONITOR)malloc(numMonitors * sizeof(PHYSICAL_MONITOR));
	brightnesses = (LPDWORD)malloc(numMonitors * sizeof(DWORD));

	if (monitors == NULL) {
		MessageBox(NULL, "Failed to alloc memory for monitors array!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numMonitors, monitors)) {
		MessageBox(NULL, "Failed to get physical monitor handles!", "Error!", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static DWORD activeMonitor = 0;
	static HMONITOR hMonitor;
	static HFONT hFont;
	static HWND hLabelMonitors;
	static HWND hLabelBrightness;
	static HWND hComboBox;
	static HWND hTrackBar;

	switch (uMsg) {
	case WM_CREATE: {
		RECT cr;
		INITCOMMONCONTROLSEX icex;

		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_BAR_CLASSES;

		InitCommonControlsEx(&icex);
		GetClientRect(hWnd, &cr);

		hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		hLabelMonitors = CreateWindowEx(0, WC_STATIC, "Monitors", WS_CHILD | WS_VISIBLE | SS_LEFT, 25, 27, 75, 25, hWnd, (HMENU)LB_MONITORS_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
		hLabelBrightness = CreateWindowEx(0, WC_STATIC, "Brightness (N/A)", WS_CHILD | WS_VISIBLE | SS_LEFT, 25, 77, 85, 25, hWnd, (HMENU)LB_BRIGHTNESS_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
		hComboBox = CreateWindowEx(0, WC_COMBOBOX, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_HASSTRINGS, cr.right - 25 - 133, 25, 133, 25, hWnd, (HMENU)CB_MONITORS_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
		hTrackBar = CreateWindowEx(0, TRACKBAR_CLASS, NULL, WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS, cr.right - 17 - 150, 75, 150, 25, hWnd, (HMENU)TB_BRIGHTNESS_ID, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

		hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);

		if (!FindMonitors(hMonitor)) {
			DestroyWindow(hWnd);
		}

		for (DWORD i = 0; i < numMonitors; i++) {
			char buffer[64];
			wsprintf(buffer, "Monitor %d", i);
			SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)buffer);

			DWORD currentBrightness, a, b;
			GetMonitorBrightness(monitors[i].hPhysicalMonitor, &a, &currentBrightness, &b);
			brightnesses[i] = currentBrightness;
		}

		SendMessage(hLabelMonitors, WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(hLabelBrightness, WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(hComboBox, WM_SETFONT, (WPARAM)hFont, TRUE);

		SendMessage(hComboBox, CB_SETCURSEL, 0, 0);

		SendMessage(hTrackBar, TBM_SETRANGEMIN, TRUE, 0);
		SendMessage(hTrackBar, TBM_SETRANGEMAX, TRUE, 100);
		SendMessage(hTrackBar, TBM_SETTICFREQ, 5, 0);
		SendMessage(hTrackBar, TBM_SETPOS, TRUE, brightnesses[0]);

		char newLabelText[64];
		wsprintf(newLabelText, "Brightness (%lu%%)", brightnesses[0]);
		SetWindowText(hLabelBrightness, newLabelText);

		return 0;
	}
	case WM_COMMAND: {
		if (HIWORD(wParam) == CBN_SELCHANGE) {
			activeMonitor = (DWORD)SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
			SendMessage(hTrackBar, TBM_SETPOS, TRUE, brightnesses[activeMonitor]);
		}

		return 0;
	}
	case WM_HSCROLL: {
		if ((HWND)lParam == hTrackBar) {
			int position = (int)SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
			SetMonitorBrightness(monitors[activeMonitor].hPhysicalMonitor, position);
			brightnesses[activeMonitor] = position;

			char newLabelText[64];
			wsprintf(newLabelText, "Brightness (%lu%%)", brightnesses[activeMonitor]);
			SetWindowText(hLabelBrightness, newLabelText);
		}

		return 0;
	}
	case WM_DESTROY: {
		DestroyPhysicalMonitors(numMonitors, monitors);
		free(monitors);
		free(brightnesses);
		PostQuitMessage(0);
		return 0;
	}
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hDc = BeginPaint(hWnd, &ps);
		FillRect(hDc, &ps.rcPaint, (HBRUSH)COLOR_WINDOW);
		EndPaint(hWnd, &ps);
		return 0;
	}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc = { 0 };
	MSG msg = { 0 };
	HWND hWnd = NULL;
	HMODULE hImageRes = NULL;

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "brightness-root";

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, "Could not register window class!", "Error!", MB_OK | MB_ICONERROR);
		return 1;
	}

	hWnd = CreateWindowEx(0, "brightness-root", "Brightness", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 300, 175, NULL, NULL, hInstance, NULL);
	hImageRes = LoadLibrary("imageres.dll");

	if (hWnd == INVALID_HANDLE_VALUE) {
		MessageBox(NULL, "Could not create window!", "Error!", MB_OK | MB_ICONERROR);
		return 1;
	}

	if (hImageRes != INVALID_HANDLE_VALUE) {
		HICON hIcon = LoadIcon(hImageRes, MAKEINTRESOURCE(101));
		
		if (hIcon != INVALID_HANDLE_VALUE) {
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		}
		else {
			MessageBox(NULL, "Could not load the icon - not applying icon", "Warning", MB_OK | MB_ICONWARNING);
		}

		FreeLibrary(hImageRes);
	}
	else {
		MessageBox(NULL, "Could not load imageres.dll - not applying icon", "Warning", MB_OK | MB_ICONWARNING);
	}

	ShowWindow(hWnd, nCmdShow);

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}