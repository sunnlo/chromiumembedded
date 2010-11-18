// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/cef.h"
#include "include/cef_wrapper.h"
#include "cefclient.h"
#include "binding_test.h"
#include "extension_test.h"
#include "plugin_test.h"
#include "resource.h"
#include "resource_util.h"
#include "scheme_test.h"
#include "string_util.h"
#include "uiplugin_test.h"
#include <sstream>
#include <commdlg.h>


#define MAX_LOADSTRING 100
#define MAX_URL_LENGTH  255
#define BUTTON_WIDTH 72
#define URLBAR_HEIGHT  24

// Define this value to run CEF with messages processed using the current
// application's message loop.
#define TEST_SINGLE_THREADED_MESSAGE_LOOP

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szWorkingDir[MAX_PATH];   // The current working directory
UINT uFindMsg;  // Message identifier for find events.
HWND hFindDlg = NULL; // Handle for the find dialog.

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

#ifdef _WIN32
// Add Common Controls to the application manifest because it's required to
// support the default tooltip implementation.
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// Program entry point function.
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // Retrieve the current working directory.
  if(_wgetcwd(szWorkingDir, MAX_PATH) == NULL)
    szWorkingDir[0] = 0;

  CefSettings settings;
  CefBrowserSettings browserDefaults;

#ifdef TEST_SINGLE_THREADED_MESSAGE_LOOP
  // Initialize the CEF with messages processed using the current application's
  // message loop.
  settings.multi_threaded_message_loop = false;
#else
  // Initialize the CEF with messages processed using a separate UI thread.
  settings.multi_threaded_message_loop = true;
#endif

  CefInitialize(settings, browserDefaults);

  // Register the internal client plugin.
  InitPluginTest();

  // Register the internal UI client plugin.
  InitUIPluginTest();

  // Register the V8 extension handler.
  InitExtensionTest();

  // Register the scheme handler.
  InitSchemeTest();
  
  MSG msg;
  HACCEL hAccelTable;

  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_CEFCLIENT, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization
  if (!InitInstance (hInstance, nCmdShow))
  {
    return FALSE;
  }

  hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CEFCLIENT));

  // Register the find event message.
  uFindMsg = RegisterWindowMessage(FINDMSGSTRING);

  // Main message loop
  while (GetMessage(&msg, NULL, 0, 0))
  {
#ifdef TEST_SINGLE_THREADED_MESSAGE_LOOP
    // Allow the CEF to do its message loop processing.
    CefDoMessageLoopWork();
#endif

    // Allow processing of find dialog messages.
    if(hFindDlg && IsDialogMessage(hFindDlg, &msg))
      continue;

    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // Shut down the CEF
  CefShutdown();

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this
//    function so that the application will get 'well formed' small icons
//    associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CEFCLIENT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CEFCLIENT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle,
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT,
      0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HWND backWnd = NULL, forwardWnd = NULL, reloadWnd = NULL,
      stopWnd = NULL, editWnd = NULL;
  static WNDPROC editWndOldProc = NULL;
  
  // Static members used for the find dialog.
  static FINDREPLACE fr;
  static WCHAR szFindWhat[80] = {0};
  static WCHAR szLastFindWhat[80] = {0};
  static bool findNext = false;
  static bool lastMatchCase = false;
  
  int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

  if(hWnd == editWnd)
  {
    // Callback for the edit window
    switch (message)
    {
    case WM_CHAR:
      if (wParam == VK_RETURN && g_handler.get())
      {
        // When the user hits the enter key load the URL
        CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
        wchar_t strPtr[MAX_URL_LENGTH] = {0};
        *((LPWORD)strPtr) = MAX_URL_LENGTH; 
        LRESULT strLen = SendMessage(hWnd, EM_GETLINE, 0, (LPARAM)strPtr);
        if (strLen > 0) {
          strPtr[strLen] = 0;
          browser->GetMainFrame()->LoadURL(strPtr);
        }

        return 0;
      }
    }

    return (LRESULT)CallWindowProc(editWndOldProc, hWnd, message, wParam, lParam);
  }
  else if (message == uFindMsg)
  { 
    // Find event.
    LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;

    if (lpfr->Flags & FR_DIALOGTERM)
    { 
      // The find dialog box has been dismissed so invalidate the handle and
      // reset the search results.
      hFindDlg = NULL; 
      if(g_handler.get())
      {
        g_handler->GetBrowser()->StopFinding(true);
        szLastFindWhat[0] = 0;
        findNext = false;
      }
      return 0; 
    } 

    if ((lpfr->Flags & FR_FINDNEXT) && g_handler.get()) 
    {
      // Search for the requested string.
      bool matchCase = (lpfr->Flags & FR_MATCHCASE?true:false);
      if(matchCase != lastMatchCase ||
        (matchCase && wcsncmp(szFindWhat, szLastFindWhat,
          sizeof(szLastFindWhat)/sizeof(WCHAR)) != 0) ||
        (!matchCase && _wcsnicmp(szFindWhat, szLastFindWhat,
          sizeof(szLastFindWhat)/sizeof(WCHAR)) != 0))
      {
        // The search string has changed, so reset the search results.
        if(szLastFindWhat[0] != 0) {
          g_handler->GetBrowser()->StopFinding(true);
          findNext = false;
        }
        lastMatchCase = matchCase;
        wcscpy_s(szLastFindWhat, sizeof(szLastFindWhat)/sizeof(WCHAR),
            szFindWhat);
      }

      g_handler->GetBrowser()->Find(0, lpfr->lpstrFindWhat,
          (lpfr->Flags & FR_DOWN)?true:false, matchCase, findNext);
      if(!findNext)
        findNext = true;
    }

    return 0; 
  }
  else
  {
    // Callback for the main window
	  switch (message)
	  {
    case WM_CREATE:
      {
        // Create the single static handler class instance
        g_handler = new ClientHandler();
        g_handler->SetMainHwnd(hWnd);

        // Create the child windows used for navigation
        RECT rect;
        int x = 0;
        
        GetClientRect(hWnd, &rect);
        
        backWnd = CreateWindow(L"BUTTON", L"Back",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                               | WS_DISABLED, x, 0, BUTTON_WIDTH, URLBAR_HEIGHT,
                               hWnd, (HMENU) IDC_NAV_BACK, hInst, 0);
        x += BUTTON_WIDTH;

        forwardWnd = CreateWindow(L"BUTTON", L"Forward",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                                  | WS_DISABLED, x, 0, BUTTON_WIDTH,
                                  URLBAR_HEIGHT, hWnd, (HMENU) IDC_NAV_FORWARD,
                                  hInst, 0);
        x += BUTTON_WIDTH;

        reloadWnd = CreateWindow(L"BUTTON", L"Reload",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                                 | WS_DISABLED, x, 0, BUTTON_WIDTH,
                                 URLBAR_HEIGHT, hWnd, (HMENU) IDC_NAV_RELOAD,
                                 hInst, 0);
        x += BUTTON_WIDTH;

        stopWnd = CreateWindow(L"BUTTON", L"Stop",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON
                               | WS_DISABLED, x, 0, BUTTON_WIDTH, URLBAR_HEIGHT,
                               hWnd, (HMENU) IDC_NAV_STOP, hInst, 0);
        x += BUTTON_WIDTH;

        editWnd = CreateWindow(L"EDIT", 0,
                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT |
                               ES_AUTOVSCROLL | ES_AUTOHSCROLL| WS_DISABLED, 
                               x, 0, rect.right - BUTTON_WIDTH * 4,
                               URLBAR_HEIGHT, hWnd, 0, hInst, 0);
        
        // Assign the edit window's WNDPROC to this function so that we can
        // capture the enter key
        editWndOldProc =
            reinterpret_cast<WNDPROC>(GetWindowLongPtr(editWnd, GWLP_WNDPROC));
        SetWindowLongPtr(editWnd, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(WndProc)); 
        g_handler->SetEditHwnd(editWnd);
        
        rect.top += URLBAR_HEIGHT;
         
        CefWindowInfo info;

        // Initialize window info to the defaults for a child window
        info.SetAsChild(hWnd, rect);

        // Creat the new child child browser window
        CefBrowser::CreateBrowser(info, false,
            static_cast<CefRefPtr<CefHandler> >(g_handler),
            L"http://www.google.com");

        // Start the timer that will be used to update child window state
        SetTimer(hWnd, 1, 250, NULL);
      }
      return 0;

    case WM_TIMER:
      if(g_handler.get() && g_handler->GetBrowserHwnd())
      {
        // Retrieve the current navigation state
        bool isLoading, canGoBack, canGoForward;
        g_handler->GetNavState(isLoading, canGoBack, canGoForward);

        // Update the status of child windows
        EnableWindow(editWnd, TRUE);
        EnableWindow(backWnd, canGoBack);
        EnableWindow(forwardWnd, canGoForward);
        EnableWindow(reloadWnd, !isLoading);
        EnableWindow(stopWnd, isLoading);
      }
      return 0;

	  case WM_COMMAND:
      {
        CefRefPtr<CefBrowser> browser;
        if(g_handler.get())
          browser = g_handler->GetBrowser();

		    wmId    = LOWORD(wParam);
		    wmEvent = HIWORD(wParam);
		    // Parse the menu selections:
		    switch (wmId)
		    {
		    case IDM_ABOUT:
          DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			    return 0;
		    case IDM_EXIT:
			    DestroyWindow(hWnd);
			    return 0;
        case ID_WARN_CONSOLEMESSAGE:
          if(g_handler.get()) {
            std::wstringstream ss;
            ss << L"Console messages will be written to "
               << g_handler->GetLogFile();
            MessageBoxW(hWnd, ss.str().c_str(), L"Console Messages",
                MB_OK | MB_ICONINFORMATION);
          }
          return 0;
        case ID_WARN_DOWNLOADCOMPLETE:
        case ID_WARN_DOWNLOADERROR:
          if(g_handler.get()) {
            std::wstringstream ss;
            ss << L"File \"" << g_handler->GetLastDownloadFile() << L"\" ";

            if(wmId == ID_WARN_DOWNLOADCOMPLETE)
              ss << L"downloaded successfully.";
            else
              ss << L"failed to download.";

            MessageBoxW(hWnd, ss.str().c_str(), L"File Download",
                MB_OK | MB_ICONINFORMATION);
          }
          return 0;
        case ID_FIND:
          if(!hFindDlg)
          {
            // Create the find dialog.
            ZeroMemory(&fr, sizeof(fr));
            fr.lStructSize = sizeof(fr);
            fr.hwndOwner = hWnd;
            fr.lpstrFindWhat = szFindWhat;
            fr.wFindWhatLen = sizeof(szFindWhat);
            fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;

            hFindDlg = FindText(&fr);
          }
          else
          {
            // Give focus to the existing find dialog.
            ::SetFocus(hFindDlg);
          }
          return 0;
        case ID_PRINT:
          if(browser.get())
            browser->GetMainFrame()->Print();
          return 0;
        case IDC_NAV_BACK:  // Back button
          if(browser.get())
            browser->GoBack();
          return 0;
        case IDC_NAV_FORWARD: // Forward button
          if(browser.get())
            browser->GoForward();
          return 0;
        case IDC_NAV_RELOAD:  // Reload button
          if(browser.get())
            browser->Reload();
          return 0;
        case IDC_NAV_STOP:  // Stop button
		     if(browser.get())
            browser->StopLoad();
          return 0;
        case ID_TESTS_GETSOURCE: // Test the GetSource function
          if(browser.get()) {
#ifdef TEST_SINGLE_THREADED_MESSAGE_LOOP
            RunGetSourceTest(browser->GetMainFrame());
#else // !TEST_SINGLE_THREADED_MESSAGE_LOOP
            // Execute the GetSource() call on the FILE thread to avoid blocking
            // the UI thread when using a multi-threaded message loop
            // (issue #79).
            class ExecTask : public CefThreadSafeBase<CefTask>
            {
            public:
              ExecTask(CefRefPtr<CefFrame> frame) : m_Frame(frame) {}
              virtual void Execute(CefThreadId threadId)
              {
                RunGetSourceTest(m_Frame);
              }
            private:
              CefRefPtr<CefFrame> m_Frame;
            };
            CefPostTask(TID_FILE, new ExecTask(browser->GetMainFrame()));
#endif // !TEST_SINGLE_THREADED_MESSAGE_LOOP
          }
          return 0;
        case ID_TESTS_GETTEXT: // Test the GetText function
          if(browser.get()) {
#ifdef TEST_SINGLE_THREADED_MESSAGE_LOOP
            RunGetTextTest(browser->GetMainFrame());
#else // !TEST_SINGLE_THREADED_MESSAGE_LOOP
            // Execute the GetText() call on the FILE thread to avoid blocking
            // the UI thread when using a multi-threaded message loop
            // (issue #79).
            class ExecTask : public CefThreadSafeBase<CefTask>
            {
            public:
              ExecTask(CefRefPtr<CefFrame> frame) : m_Frame(frame) {}
              virtual void Execute(CefThreadId threadId)
              {
                RunGetTextTest(m_Frame);
              }
            private:
              CefRefPtr<CefFrame> m_Frame;
            };
            CefPostTask(TID_FILE, new ExecTask(browser->GetMainFrame()));
#endif // !TEST_SINGLE_THREADED_MESSAGE_LOOP
          }
          return 0;
        case ID_TESTS_JAVASCRIPT_BINDING: // Test the V8 binding handler
          if(browser.get())
            RunBindingTest(browser);
          return 0;
        case ID_TESTS_JAVASCRIPT_EXTENSION: // Test the V8 extension handler
          if(browser.get())
            RunExtensionTest(browser);
          return 0;
        case ID_TESTS_JAVASCRIPT_EXECUTE: // Test execution of javascript
          if(browser.get())
            RunJavaScriptExecuteTest(browser);
          return 0;
        case ID_TESTS_PLUGIN: // Test the custom plugin
          if(browser.get())
            RunPluginTest(browser);
          return 0;
        case ID_TESTS_POPUP: // Test a popup window
          if(browser.get())
            RunPopupTest(browser);
          return 0;
        case ID_TESTS_REQUEST: // Test a request
          if(browser.get())
            RunRequestTest(browser);
          return 0;
        case ID_TESTS_SCHEME_HANDLER: // Test the scheme handler
          if(browser.get())
            RunSchemeTest(browser);
          return 0;
        case ID_TESTS_UIAPP: // Test the UI app
          if(browser.get())
            RunUIPluginTest(browser);
          return 0;
        case ID_TESTS_LOCALSTORAGE: // Test localStorage
          if(browser.get())
            RunLocalStorageTest(browser);
          return 0;
        }
      }
		  break;

	  case WM_PAINT:
		  hdc = BeginPaint(hWnd, &ps);
		  EndPaint(hWnd, &ps);
		  return 0;

    case WM_SETFOCUS:
      if(g_handler.get() && g_handler->GetBrowserHwnd())
      {
        // Pass focus to the browser window
        PostMessage(g_handler->GetBrowserHwnd(), WM_SETFOCUS, wParam, NULL);
      }
      return 0;

    case WM_SIZE:
      if(g_handler.get() && g_handler->GetBrowserHwnd())
      {
        // Resize the browser window and address bar to match the new frame
        // window size
        RECT rect;
        GetClientRect(hWnd, &rect);
        rect.top += URLBAR_HEIGHT;

        int urloffset = rect.left + BUTTON_WIDTH * 4;

        HDWP hdwp = BeginDeferWindowPos(1);
        hdwp = DeferWindowPos(hdwp, editWnd, NULL, urloffset,
          0, rect.right - urloffset, URLBAR_HEIGHT, SWP_NOZORDER);
        hdwp = DeferWindowPos(hdwp, g_handler->GetBrowserHwnd(), NULL,
          rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
          SWP_NOZORDER);
        EndDeferWindowPos(hdwp);
      }
      break;

    case WM_ERASEBKGND:
      if(g_handler.get() && g_handler->GetBrowserHwnd())
      {
        // Dont erase the background if the browser window has been loaded
        // (this avoids flashing)
		    return 0;
      }
      break;
    
	  case WM_DESTROY:
      // The frame window has exited
      KillTimer(hWnd, 1);
		  PostQuitMessage(0);
		  return 0;
    }
  	
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// ClientHandler implementation

CefHandler::RetVal ClientHandler::HandleAddressChange(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
    const std::wstring& url)
{
  if(m_BrowserHwnd == browser->GetWindowHandle() && frame->IsMain())
  {
    // Set the edit window text
    SetWindowText(m_EditHwnd, url.c_str());
  }
  return RV_CONTINUE;
}

CefHandler::RetVal ClientHandler::HandleTitleChange(
    CefRefPtr<CefBrowser> browser, const std::wstring& title)
{
  // Set the frame window title bar
  CefWindowHandle hwnd = browser->GetWindowHandle();
  if(!browser->IsPopup())
  {
    // The frame window will be the parent of the browser window
    hwnd = GetParent(hwnd);
  }
  SetWindowText(hwnd, title.c_str());
  return RV_CONTINUE;
}

CefHandler::RetVal ClientHandler::HandleBeforeResourceLoad(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefRequest> request,
    std::wstring& redirectUrl, CefRefPtr<CefStreamReader>& resourceStream,
    std::wstring& mimeType, int loadFlags)
{
  DWORD dwSize;
  LPBYTE pBytes;
  
  std::wstring url = request->GetURL();
  if(url == L"http://tests/request") {
    // Show the request contents
    std::wstring dump;
    DumpRequestContents(request, dump);
    resourceStream = CefStreamReader::CreateForData(
        (void*)dump.c_str(), dump.size() * sizeof(wchar_t));
    mimeType = L"text/plain";
  } else if(url == L"http://tests/uiapp") {
    // Show the uiapp contents
    if(LoadBinaryResource(IDS_UIPLUGIN, dwSize, pBytes)) {
      resourceStream = CefStreamReader::CreateForHandler(
          new CefByteReadHandler(pBytes, dwSize, NULL));
      mimeType = L"text/html";
    }
  } else if(url == L"http://tests/localstorage") {
    // Show the localstorage contents
    if(LoadBinaryResource(IDS_LOCALSTORAGE, dwSize, pBytes)) {
      resourceStream = CefStreamReader::CreateForHandler(
          new CefByteReadHandler(pBytes, dwSize, NULL));
      mimeType = L"text/html";
    }
  } else if(wcsstr(url.c_str(), L"/ps_logo2.png") != NULL) {
    // Any time we find "ps_logo2.png" in the URL substitute in our own image
    if(LoadBinaryResource(IDS_LOGO, dwSize, pBytes)) {
      resourceStream = CefStreamReader::CreateForHandler(
          new CefByteReadHandler(pBytes, dwSize, NULL));
      mimeType = L"image/png";
    }
  } else if(wcsstr(url.c_str(), L"/logoball.png") != NULL) {
    // Load the "logoball.png" image resource.
    if(LoadBinaryResource(IDS_LOGOBALL, dwSize, pBytes)) {
      resourceStream = CefStreamReader::CreateForHandler(
          new CefByteReadHandler(pBytes, dwSize, NULL));
      mimeType = L"image/png";
    }
  }
  return RV_CONTINUE;
}

void ClientHandler::SendNotification(NotificationType type)
{
  UINT id;
  switch(type)
  {
  case NOTIFY_CONSOLE_MESSAGE:
    id = ID_WARN_CONSOLEMESSAGE;
    break;
  case NOTIFY_DOWNLOAD_COMPLETE:
    id = ID_WARN_DOWNLOADCOMPLETE;
    break;
  case NOTIFY_DOWNLOAD_ERROR:
    id = ID_WARN_DOWNLOADERROR;
    break;
  default:
    return;
  }
  PostMessage(m_MainHwnd, WM_COMMAND, id, 0);
}


// Global functions

std::wstring AppGetWorkingDirectory()
{
	return szWorkingDir;
}