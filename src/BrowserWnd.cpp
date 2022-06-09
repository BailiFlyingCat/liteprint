#include "globals.h"
#include "resource.h"
#include "BrowserWnd.h"
#include "HtmlViewWnd.h"
#include "ToolbarWnd.h"

CBrowserWnd::CBrowserWnd(HINSTANCE hInst)
{
	m_hInst		= hInst;
	m_hWnd		= NULL;
	m_view		= new CHTMLViewWnd(hInst, &m_browser_context, this);
#ifndef NO_TOOLBAR
	m_toolbar	= new CToolbarWnd(hInst, this);
#endif

	m_printViewWidth   = 0;
	m_printViewHeight  = 0;
	m_printPixelWidth  = 0;
	m_printPixelHeight = 0;
	m_scale            = 1.0;
	m_printHdc         = nullptr;

	WNDCLASS wc;
	if(!GetClassInfo(m_hInst, BROWSERWND_CLASS, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.style          = CS_DBLCLKS /*| CS_HREDRAW | CS_VREDRAW*/;
		wc.lpfnWndProc    = (WNDPROC)CBrowserWnd::WndProc;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hInstance      = m_hInst;
		wc.hIcon          = NULL;
		wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground  = NULL;
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = BROWSERWND_CLASS;

		RegisterClass(&wc);
	}

#ifndef LITEHTML_UTF8
	LPWSTR css = NULL;
	HRSRC hResource = ::FindResource(m_hInst, L"master.css", L"CSS");
	if(hResource)
	{
		DWORD imageSize = ::SizeofResource(m_hInst, hResource);
		if(imageSize)
		{
			LPCSTR pResourceData = (LPCSTR) ::LockResource(::LoadResource(m_hInst, hResource));
			if(pResourceData)
			{
				css = new WCHAR[imageSize * 3];
				int ret = MultiByteToWideChar(CP_UTF8, 0, pResourceData, imageSize, css, imageSize * 3);
				css[ret] = 0;
			}
		}
	}
#else
	LPSTR css = NULL;
	HRSRC hResource = ::FindResource(m_hInst, L"master.css", L"CSS");
	if(hResource)
	{
		DWORD imageSize = ::SizeofResource(m_hInst, hResource);
		if(imageSize)
		{
			LPCSTR pResourceData = (LPCSTR) ::LockResource(::LoadResource(m_hInst, hResource));
			if(pResourceData)
			{
				css = new CHAR[imageSize + 1];
				lstrcpynA(css, pResourceData, imageSize);
				css[imageSize] = 0;
			}
		}
	}
#endif
	if(css)
	{
		m_browser_context.load_master_stylesheet(css);
		delete css;
	}
}

CBrowserWnd::~CBrowserWnd(void)
{
	if(m_view)		delete m_view;
#ifndef NO_TOOLBAR
	if(m_toolbar)	delete m_toolbar;
#endif

	if (m_printHdc)
	{
		DeleteDC(m_printHdc);
		m_printHdc = nullptr;
	}
}

LRESULT CALLBACK CBrowserWnd::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	CBrowserWnd* pThis = NULL;
	if(IsWindow(hWnd))
	{
		pThis = (CBrowserWnd*)GetProp(hWnd, TEXT("browser_this"));
		if(pThis && pThis->m_hWnd != hWnd)
		{
			pThis = NULL;
		}
	}

	if(pThis || uMessage == WM_CREATE)
	{
		switch (uMessage)
		{
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE:
			{
				LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
				pThis = (CBrowserWnd*)(lpcs->lpCreateParams);
				SetProp(hWnd, TEXT("browser_this"), (HANDLE) pThis);
				pThis->m_hWnd = hWnd;
				pThis->OnCreate();
			}
			break;
		case WM_PAINT:
		    {
			    RECT rcClient;
			    GetClientRect(hWnd, &rcClient);

			    PAINTSTRUCT ps;
			    HDC hdc = BeginPaint(hWnd, &ps);
			    FillRect(hdc, &ps.rcPaint, (HBRUSH)LTGRAY_BRUSH);
			    EndPaint(hWnd, &ps);

			    InvalidateRect(pThis->m_view->wnd(), NULL, TRUE);
		    }
		    return 0;
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemoveProp(hWnd, TEXT("browser_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		case WM_CLOSE:
			Shell_NotifyIcon(NIM_DELETE, &(pThis->m_nid));
			PostQuitMessage(0);
			return 0;
		case WM_ACTIVATE:
			if(LOWORD(wParam) != WA_INACTIVE)
			{
				SetFocus(pThis->m_view->wnd());
			}
			return 0;
		case WM_TIMER:
			pThis->OnTimer(hWnd, wParam);
			break;
		case WM_TRAY:
		{
			if (lParam == WM_RBUTTONDOWN)
			{
				POINT pt;
				GetCursorPos(&pt);

				HMENU hMenu;
				hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING, IDM_ABOUT, _t("关于"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hMenu, MF_STRING, IDM_QUIT, _t("退出"));
				TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, NULL, pThis->m_hWnd, NULL);
			}
		}
		break;
		case WM_COMMAND:
			switch (wParam)
			{
			case IDM_ABOUT:
				MessageBox(NULL, _t("HTML静默打印程序"), _t("关于 liteprint"), MB_ICONINFORMATION);
				break;
			case IDM_QUIT:
				PostMessage(pThis->m_hWnd, WM_CLOSE, 0, 0);
				break;
			default:
				break;
			}
			break;
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void CBrowserWnd::OnCreate()
{
#ifdef HTML_VIEW
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);

	int viewWidth = rcClient.right - rcClient.left;
	int x = rcClient.left;
	if (m_printViewWidth > 0 && viewWidth > m_printViewWidth)
	{
		x = (viewWidth - m_printViewWidth) / 2 + rcClient.left;
		viewWidth = m_printViewWidth;
	}
#ifndef NO_TOOLBAR
	m_toolbar->create(rcClient.left, rcClient.top, rcClient.right - rcClient.left, m_hWnd);
	m_view->create(x, rcClient.top + m_toolbar->height(), viewWidth, rcClient.bottom - rcClient.top - m_toolbar->height(), m_hWnd);
#else
	m_view->create(x, rcClient.top, viewWidth, rcClient.bottom - rcClient.top, m_hWnd);
#endif
	SetFocus(m_view->wnd());
#else
	m_view->set_print(m_printViewWidth, m_printViewHeight, m_printHdc);
#endif

	// websocket 监听定时器
	SetTimer(m_hWnd, 1, 100, NULL);
}

void CBrowserWnd::OnSize( int width, int height )
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	int viewWidth = rcClient.right - rcClient.left;
	int x = rcClient.left;
	if (m_printViewWidth > 0 && viewWidth > m_printViewWidth)
	{
		x = (viewWidth - m_printViewWidth) / 2 + rcClient.left;
		viewWidth = m_printViewWidth;
	}
#ifndef NO_TOOLBAR
	int toolbar_height = m_toolbar->set_width(rcClient.right - rcClient.left);
#else
	int toolbar_height = 0;
#endif
	SetWindowPos(m_view->wnd(), NULL, x, rcClient.top + toolbar_height, viewWidth, rcClient.bottom - rcClient.top - toolbar_height, SWP_NOZORDER);
	UpdateWindow(m_view->wnd());
#ifndef NO_TOOLBAR
	SetWindowPos(m_toolbar->wnd(), NULL, rcClient.left, rcClient.top, rcClient.right - rcClient.left, toolbar_height, SWP_NOZORDER);
	UpdateWindow(m_toolbar->wnd());
#endif
}

void CBrowserWnd::OnDestroy()
{
	mg_mgr_free(&m_mgr);
}

void CBrowserWnd::create()
{
	initDefaultPrinter();

	m_hWnd = CreateWindow(BROWSERWND_CLASS, L"Light HTML Print", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInst, (LPVOID) this);

#ifdef HTML_VIEW
	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
#endif

	// 创建系统托盘图标并初始化websocket
	createNotify();
	initWebSocket();
}

void CBrowserWnd::open( LPCWSTR path )
{
	if(m_view)
	{
		m_view->open(path, false);
	}
}

void CBrowserWnd::back()
{
	if(m_view)
	{
		m_view->back();
	}
}

void CBrowserWnd::forward()
{
	if(m_view)
	{
		m_view->forward();
	}
}

void CBrowserWnd::reload()
{
	if(m_view)
	{
		m_view->refresh();
	}
}

void CBrowserWnd::calc_time(int calc_repeat)
{
	if(m_view)
	{
		m_view->render(TRUE, TRUE, calc_repeat);
	}
}

void CBrowserWnd::on_page_loaded(int htmlHeight, LPCWSTR url)
{
#ifdef HTML_VIEW
	if (m_view)
	{
		SetFocus(m_view->wnd());
	}
#ifndef NO_TOOLBAR
	m_toolbar->on_page_loaded(url);
#endif
#else
	DOCINFO di = { sizeof(DOCINFO), TEXT("liteprint") };
	if (StartDoc(m_printHdc, &di) > 0)
	{
		int left = 0, top = 0;
		while (htmlHeight > 0)
		{
			StartPage(m_printHdc);
			m_view->draw_to_print(left, top, m_printPixelWidth, m_printPixelHeight, m_scale);
			EndPage(m_printHdc);

			htmlHeight -= m_printViewHeight;
			top += m_printViewHeight;
		}
		EndDoc(m_printHdc);
	}
	else
	{
		OutputDebugString(TEXT("打印失败...\r\n"));
	}
#endif
}

void CBrowserWnd::createNotify()
{
	m_nid.cbSize = (DWORD)sizeof(NOTIFYICONDATA);
	m_nid.hWnd = m_hWnd;
	m_nid.uID = IDR_MAINFRAME;
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_nid.uCallbackMessage = WM_TRAY;
	m_nid.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_SMALL));
	wcscpy_s(m_nid.szTip, _T("HTML静默打印程序"));
	Shell_NotifyIcon(NIM_ADD, &m_nid);
}

void CBrowserWnd::wsfn(struct mg_connection* mc, int ev, void* ev_data, void* fn_data)
{
	if (ev == MG_EV_OPEN) {
		// c->is_hexdumping = 1;
	}
	else if (ev == MG_EV_HTTP_MSG)
	{
		struct mg_http_message* hm = (struct mg_http_message*)ev_data;
		if (mg_http_match_uri(hm, "/websocket"))
		{
			// Upgrade to websocket. From now on, a connection is a full-duplex
			// Websocket connection, which will receive MG_EV_WS_MSG events.
			mg_ws_upgrade(mc, hm, NULL);
		}
		else
		{
			OutputDebugStringA("todo this...\r\n");
		}
	}
	else if (ev == MG_EV_WS_MSG)
	{
		// Got websocket frame. Received data is wm->data. Echo it back!
		struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;

		int length = MultiByteToWideChar(CP_UTF8, 0, wm->data.ptr, -1, NULL, 0);
		LPWSTR html_str = new WCHAR[length + 1];
		MultiByteToWideChar(CP_UTF8, 0, wm->data.ptr, -1, html_str, length + 1);

		// mg_ws_send(mc, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);

		CBrowserWnd* pThis = NULL;
		HWND hWnd = (HWND)fn_data;
		if (IsWindow(hWnd))
		{
			pThis = (CBrowserWnd*)GetProp(hWnd, TEXT("browser_this"));
			if (pThis && pThis->m_view)
			{
				pThis->m_view->open(html_str, true, true);
#ifdef HTML_VIEW
				SetForegroundWindow(hWnd);
#endif
			}
		}
	}
}

void CBrowserWnd::initDefaultPrinter()
{
	DWORD length;
	GetDefaultPrinter(NULL, &length);
	if (length)
	{
		TCHAR* szPrinter = (TCHAR*)malloc(length * sizeof(TCHAR));
		GetDefaultPrinter(szPrinter, &length);
		if (length)
		{
			OutputDebugString(TEXT("默认打印机: "));
			OutputDebugString(szPrinter);
			OutputDebugStringA("\n");

			m_printHdc = CreateDC(NULL, szPrinter, NULL, NULL);
			if (!m_printHdc)
			{
				return;
			}
			m_printViewWidth = GetDeviceCaps(m_printHdc, HORZSIZE);
			m_printViewHeight = GetDeviceCaps(m_printHdc, VERTSIZE);
			m_printPixelWidth = GetDeviceCaps(m_printHdc, HORZRES);
			m_printPixelHeight = GetDeviceCaps(m_printHdc, VERTRES);
			m_scale = GetDeviceCaps(m_printHdc, LOGPIXELSX);

			HDC hdc = GetDC(NULL);
			int logPixel = GetDeviceCaps(hdc, LOGPIXELSX);
			m_printViewWidth = (m_printViewWidth / 25.4) * logPixel;
			m_printViewHeight = (m_printViewHeight / 25.4) * logPixel;
			DeleteDC(hdc);

			m_scale /= logPixel;
		}
	}
}

void CBrowserWnd::initWebSocket()
{
	mg_mgr_init(&m_mgr);

	const char* strListen = "ws://127.0.0.1:20506";
	mg_http_listen(&m_mgr, strListen, wsfn, m_hWnd);
	/* while (true)
	{
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr); */
}

void CBrowserWnd::OnTimer(HWND hWnd, WPARAM wParam)
{
	mg_mgr_poll(&m_mgr, 50);
}
