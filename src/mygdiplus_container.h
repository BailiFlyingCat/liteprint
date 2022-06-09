#pragma once

#include <windows.h>
#include <litehtml.h>
#include <gdiplus.h>

#ifdef LITEHTML_UTF8
#define t_make_url	make_url_utf8
#else
#define t_make_url	make_url
#endif

struct gdiplus_clip_box
{
	typedef std::vector<gdiplus_clip_box> vector;
	litehtml::position	box;
	litehtml::border_radiuses radius;

	gdiplus_clip_box(const litehtml::position& vBox, litehtml::border_radiuses vRad)
	{
		box = vBox;
		radius = vRad;
	}

	gdiplus_clip_box(const gdiplus_clip_box& val)
	{
		box = val.box;
		radius = val.radius;
	}

	gdiplus_clip_box& operator=(const gdiplus_clip_box& val)
	{
		box = val.box;
		radius = val.radius;
		return *this;
	}
};

class mygdiplus_container :	public litehtml::document_container
{
public:
	typedef std::map<std::wstring, litehtml::uint_ptr>	images_map;

protected:
	images_map					m_images;
	gdiplus_clip_box::vector	m_clips;
	CRITICAL_SECTION			m_img_sync;

private:
	Gdiplus::FontFamily*		m_fontFamily;
	Gdiplus::Font*				m_font;

public:
	mygdiplus_container(void);
	virtual ~mygdiplus_container(void);

	virtual litehtml::uint_ptr			create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
	virtual void						delete_font(litehtml::uint_ptr hFont) override;
	virtual int							text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont) override;
	virtual void						draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;

	virtual int							pt_to_px(int pt) const override;
	virtual int							get_default_font_size() const override;
	virtual const litehtml::tchar_t*	get_default_font_name() const override;
	virtual void						draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
	virtual void						load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready) override;
	virtual void						get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz) override;
	virtual void						draw_image(litehtml::uint_ptr hdc, const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, const litehtml::position& pos);
	virtual void						draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg) override;
	virtual void						draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;
	virtual void						fill_rect(litehtml::uint_ptr hdc, int x, int y, int width, int height, const litehtml::web_color& color);

	virtual	void						transform_text(litehtml::tstring& text, litehtml::text_transform tt) override;
	virtual void						set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y) override;
	virtual void						del_clip() override;
	virtual std::shared_ptr<litehtml::element>	create_element(const litehtml::tchar_t* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) override;
	virtual void						get_media_features(litehtml::media_features& media) const override;
	virtual void						get_language(litehtml::tstring& language, litehtml::tstring & culture) const override;
	virtual void						link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    virtual litehtml::tstring			resolve_color(const litehtml::tstring& color) const override;


	virtual void						make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out ) = 0;
	virtual litehtml::uint_ptr			get_image(LPCWSTR url, bool redraw_on_ready) = 0;
	void								clear_images();
	void								add_image(std::wstring& url, litehtml::uint_ptr& img);
	void								remove_image(std::wstring& url);
	void								make_url_utf8( const char* url, const char* basepath, std::wstring& out );

	virtual void output_debug_string(int value) = 0;
	virtual void output_debug_string(const char* str) = 0;
	virtual void output_debug_string(const wchar_t* str) = 0;

protected:
	virtual void						draw_ellipse(litehtml::uint_ptr hdc, int x, int y, int width, int height, const litehtml::web_color& color, double line_width);
	virtual void						fill_ellipse(litehtml::uint_ptr hdc, int x, int y, int width, int height, const litehtml::web_color& color);

private:
	void								apply_clip(litehtml::uint_ptr hdc);
	void								release_clip(litehtml::uint_ptr hdc);

	void								lock_images_cache();
	void								unlock_images_cache();
};
