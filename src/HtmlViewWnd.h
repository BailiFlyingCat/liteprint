#pragma once
#include "web_page.h"
#include "web_history.h"

#define HTMLVIEWWND_CLASS	L"HTMLVIEW_WINDOW"

#define WM_IMAGE_LOADED		(WM_USER + 1000)
#define WM_PAGE_LOADED		(WM_USER + 1001)

using namespace litehtml;
class CBrowserWnd;

class CHTMLViewWnd
{
	HWND						m_hWnd;
	HINSTANCE					m_hInst;
	int							m_top;
	int							m_left;
	int							m_max_top;
	int							m_max_left;
	litehtml::context*			m_context;
	web_history					m_history;
	web_page*					m_page;
	web_page*					m_page_next;
	CRITICAL_SECTION			m_sync;
	simpledib::dib				m_dib;
	CBrowserWnd*				m_parent;

	int							m_viewWidth;
	int							m_viewHeight;
	int							m_htmlHeight;
	HDC							m_printHdc;
public:
	CHTMLViewWnd(HINSTANCE	hInst, litehtml::context* ctx, CBrowserWnd* parent);
	virtual ~CHTMLViewWnd(void);

	void				create(int x, int y, int width, int height, HWND parent);
	void				open(LPCWSTR url, bool reload = FALSE, bool isws = FALSE);
	HWND				wnd()	{ return m_hWnd;	}
	void				refresh();
	void				back();
	void				forward();

	litehtml::context*	get_html_context();
	void				set_caption();
	void				lock();
	void				unlock();
	bool				is_valid_page(bool with_lock = true);
	web_page*			get_page(bool with_lock = true);

	void				render(BOOL calc_time = FALSE, BOOL do_redraw = TRUE, int calc_repeat = 1);
	void				get_client_rect(litehtml::position& client) const;
	void				show_hash(std::wstring& hash);
	void				update_history();

	void				set_print(int viewWidth, int viewHeight, HDC printHdc);
	virtual void		page_loaded(bool isdraw = true);
	virtual void		image_loaded(bool redraw_only);
	virtual void		draw_to_print(int left, int top, int hdcWidth, int hdcHeight, double scale);

protected:
	virtual void		OnCreate();
	virtual void		OnPaint();
	virtual void		OnSize(int width, int height);
	virtual void		OnDestroy();
	virtual void		OnVScroll(int pos, int flags);
	virtual void		OnHScroll(int pos, int flags);
	virtual void		OnMouseWheel(int delta);
	virtual void		OnKeyDown(UINT vKey);
	virtual void		OnMouseMove(int x, int y);
	virtual void		OnLButtonDown(int x, int y);
	virtual void		OnLButtonUp(int x, int y);
	virtual void		OnMouseLeave();
	virtual void		OnPageReady(bool isdraw = true);
	virtual void		OnImageLoaded(bool redraw_only);
	
	void				redraw(LPRECT rcDraw, BOOL update);
	void				update_scroll();
	void				update_cursor();
	void				create_dib(int width, int height);
	void				scroll_to(int new_left, int new_top);
	void				draw(simpledib::dib* dib, LPRECT rcDraw, double scale = 1.0);	

private:
	static LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

inline litehtml::context* CHTMLViewWnd::get_html_context()
{
	return m_context;
}

inline void CHTMLViewWnd::lock()
{
	EnterCriticalSection(&m_sync);
}

inline void CHTMLViewWnd::unlock()
{
	LeaveCriticalSection(&m_sync);
}
