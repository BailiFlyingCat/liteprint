#pragma once

#include "mongoose\mongoose.h"

#define BROWSERWND_CLASS	L"BROWSER_WINDOW"

#define WM_TRAY		(WM_USER + 3000)
#define IDM_ABOUT	(WM_USER + 3001)
#define IDM_QUIT	(WM_USER + 3002)

class CHTMLViewWnd;
class CToolbarWnd;

class CBrowserWnd
{
	HWND				m_hWnd;
	HINSTANCE			m_hInst;
	CHTMLViewWnd*		m_view;
#ifndef NO_TOOLBAR
	CToolbarWnd*		m_toolbar;
#endif
	litehtml::context	m_browser_context;
public:
	CBrowserWnd(HINSTANCE hInst);
	virtual ~CBrowserWnd(void);

	void create();
	void open(LPCWSTR path);

	void back();
	void forward();
	void reload();
	void calc_time(int calc_repeat = 1);
	void on_page_loaded(int htmlHeight, LPCWSTR url);

protected:
	virtual void OnCreate();
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	void initDefaultPrinter();
	void initWebSocket();
	static void wsfn(struct mg_connection* mc, int ev, void* ev_data, void* fn_data);
	void OnTimer(HWND hWnd, WPARAM wParam);
	void createNotify();
private:
	struct mg_mgr	m_mgr;
	NOTIFYICONDATA	m_nid;
	int				m_printViewWidth;
	int				m_printViewHeight;
	int				m_printPixelWidth;
	int				m_printPixelHeight;
	double			m_scale;
	HDC				m_printHdc;
};
