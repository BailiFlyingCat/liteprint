#pragma once
#include "gdiplus_container.h"
#include "dib.h"
#include "el_omnibox.h"

#define TOOLBARWND_CLASS	L"TOOLBAR_WINDOW"

class CBrowserWnd;

class CToolbarWnd : public gdiplus_container
{
	HWND					m_hWnd;
	HINSTANCE				m_hInst;
	litehtml::context		m_context;
	litehtml::document::ptr	m_doc;
	CBrowserWnd*			m_parent;
	std::shared_ptr<el_omnibox>	m_omnibox;
	litehtml::tstring		m_cursor;
	BOOL					m_inCapture;
	Gdiplus::Graphics*		m_graphics;
public:
	CToolbarWnd(HINSTANCE hInst, CBrowserWnd* parent);
	virtual ~CToolbarWnd(void);

	void create(int x, int y, int width, HWND parent);
	HWND wnd()	{ return m_hWnd; }
	int height()
	{
		if(m_doc)
		{
			return m_doc->height();
		}
		return 0;
	}
	int set_width(int width);
	void on_page_loaded(LPCWSTR url);

	// cairo_container members
	virtual void			make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
	virtual litehtml::uint_ptr get_image(LPCWSTR url, bool redraw_on_ready);

	virtual void output_debug_string(int value) override;
	virtual void output_debug_string(const char* str) override;
	virtual void output_debug_string(const wchar_t* str) override;

	// litehtml::document_container members
	virtual	void	set_caption(const litehtml::tchar_t* caption);
	virtual	void	set_base_url(const litehtml::tchar_t* base_url);
	virtual	void	link(std::shared_ptr<litehtml::document>& doc, litehtml::element::ptr el);
	virtual void	import_css(litehtml::tstring& text, const litehtml::tstring& url, litehtml::tstring& baseurl);
	virtual	void	on_anchor_click(const litehtml::tchar_t* url, const litehtml::element::ptr& el);
	virtual	void	set_cursor(const litehtml::tchar_t* cursor);
	virtual std::shared_ptr<litehtml::element> create_element(const litehtml::tchar_t* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc);

protected:
	virtual void	OnCreate();
	virtual void	OnPaint(simpledib::dib* dib, LPRECT rcDraw);
	virtual void	OnSize(int width, int height);
	virtual void	OnDestroy();
	virtual void	OnMouseMove(int x, int y);
	virtual void	OnLButtonDown(int x, int y);
	virtual void	OnLButtonUp(int x, int y);
	virtual void	OnMouseLeave();
	virtual void	OnOmniboxClicked();

	virtual void	get_client_rect(litehtml::position& client) const;

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	void render_toolbar(int width);
	void update_cursor();
};
