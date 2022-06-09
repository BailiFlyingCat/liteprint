#include "gdiplus_container.h"
#include <strsafe.h>
#include <cmath>

gdiplus_container::gdiplus_container(void)
{
	m_fontFamily	= NULL;
	m_font			= NULL;

	InitializeCriticalSection(&m_img_sync);
}

gdiplus_container::~gdiplus_container(void)
{
	clear_images();
	DeleteCriticalSection(&m_img_sync);

	if (m_font)
	{
		delete m_font;
	}
	if (m_fontFamily)
	{
		delete m_fontFamily;
	}
}

litehtml::uint_ptr gdiplus_container::create_font( const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm )
{
	std::wstring fnt_name = L"sans-serif";

	litehtml::string_vector fonts;
	litehtml::split_string(faceName, fonts, _t(","));
	if (!fonts.empty())
	{
		litehtml::trim(fonts[0]);

		fnt_name = fonts[0];
		if (fnt_name.front() == L'"' || fnt_name.front() == L'\'')
		{
			fnt_name.erase(0, 1);
		}
		if (fnt_name.back() == L'"' || fnt_name.back() == L'\'')
		{
			fnt_name.erase(fnt_name.length() - 1, 1);
		}
	}
	const litehtml::tchar_t* fontName = fnt_name.c_str();

	litehtml::tstring strName;
	if (!lstrcmpi(fontName, L"monospace"))
	{
		strName = L"Courier New";
	}
	else if (!lstrcmpi(fontName, L"serif"))
	{
		strName = L"Times New Roman";
	}
	else if (!lstrcmpi(fontName, L"sans-serif"))
	{
		strName = L"Arial";
	}
	else if (!lstrcmpi(fontName, L"fantasy"))
	{
		strName = L"Impact";
	}
	else if (!lstrcmpi(fontName, L"cursive"))
	{
		strName = L"Comic Sans MS";
	}
	else
	{
		strName = fontName;
	}

	int fontStyle = Gdiplus::FontStyleRegular;
	if (italic == litehtml::fontStyleItalic)
	{
		fontStyle = Gdiplus::FontStyleItalic;
	}
	if (weight >= 700)
	{
		fontStyle |= Gdiplus::FontStyleBold;
	}
	if (decoration & litehtml::font_decoration_linethrough)
	{
		fontStyle |= Gdiplus::FontStyleStrikeout;
	}
	if (decoration & litehtml::font_decoration_underline)
	{
		fontStyle |= Gdiplus::FontStyleUnderline;
	}

	m_fontFamily = new Gdiplus::FontFamily(strName.c_str());
	m_font = new Gdiplus::Font(m_fontFamily, size, fontStyle, Gdiplus::UnitPixel);

	if (fm)
	{
		fm->ascent	= round(m_font->GetSize() * m_fontFamily->GetCellAscent(fontStyle) / m_fontFamily->GetEmHeight(fontStyle));
		fm->descent	= round(m_font->GetSize() * m_fontFamily->GetCellDescent(fontStyle) / m_fontFamily->GetEmHeight(fontStyle));
		fm->height	= ceil(m_font->GetHeight(0.0f));

		Gdiplus::GraphicsPath path;
		Gdiplus::StringFormat strFormat(Gdiplus::StringAlignmentNear);
		path.AddString(_t("x"), -1, m_fontFamily, m_font->GetStyle(), m_font->GetSize(), Gdiplus::PointF(0, 0), &strFormat);
		Gdiplus::RectF rcBound;
		path.GetBounds(&rcBound);
		fm->x_height = ceil(rcBound.Width);

		if (italic == litehtml::fontStyleItalic || decoration)
		{
			fm->draw_spaces = true;
		}
		else
		{
			fm->draw_spaces = false;
		}
	}

	return (litehtml::uint_ptr)(m_font);
}

void gdiplus_container::delete_font( litehtml::uint_ptr hFont )
{
	if (m_font)
	{
		delete m_font;
		m_font = nullptr;
	}
	if (m_fontFamily)
	{
		delete m_fontFamily;
		m_fontFamily = nullptr;
	}
}

int gdiplus_container::text_width( const litehtml::tchar_t* text, litehtml::uint_ptr hFont )
{
	if (!hFont || wcslen(text) <= 0)
	{
		return 0;
	}
	Gdiplus::Font* font = (Gdiplus::Font*)hFont;
	Gdiplus::StringFormat strFormat(Gdiplus::StringAlignmentNear);

	int ret = 1;
	if (!wcscmp(text, _t(" ")))
	{
		Gdiplus::RectF	rect;
		Gdiplus::PointF point(0, 0);

		HDC hdc = GetDC(NULL);
		Gdiplus::Graphics graphics(hdc);
		graphics.MeasureString(text, wcslen(text), font, point, &strFormat, &rect);
		ret = ceil(rect.Width);
		DeleteDC(hdc);
	}
	else
	{
		Gdiplus::FontFamily fontFamily;
		font->GetFamily(&fontFamily);

		Gdiplus::GraphicsPath path;
		path.AddString(text, wcslen(text), &fontFamily, font->GetStyle(), font->GetSize(), Gdiplus::PointF(0, 0), &strFormat);
		Gdiplus::RectF rcBound;
		path.GetBounds(&rcBound);

		ret = ceil(rcBound.Width);
	}

	/* output_debug_string("text_width->");
	if (!wcscmp(text, _t(" ")))
	{
		output_debug_string(_t("¿Õ¸ñ"));
	}
	else
	{
		output_debug_string(text);
	}	
	output_debug_string(": ");
	output_debug_string(ret);
	output_debug_string("\n"); */
	
	return ret;
}

void gdiplus_container::draw_text( litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	if (!hdc || !hFont)
	{
		return;
	}

	/* output_debug_string("draw_text->");
	output_debug_string(text);
	output_debug_string(": ");
	output_debug_string(pos.x);
	output_debug_string(",");
	output_debug_string(pos.y);
	output_debug_string("\n"); */

	Gdiplus::Graphics*	graphics	= (Gdiplus::Graphics*)hdc;
	Gdiplus::Font*		font		= (Gdiplus::Font*)hFont;
	Gdiplus::PointF		pointF(pos.x, pos.y);
	Gdiplus::SolidBrush	solidBrush(Gdiplus::Color(color.alpha, color.red, color.green, color.blue));

	// graphics->DrawString(text, -1, font, pointF, &solidBrush);
	// Gdiplus::StringFormat strFormat(Gdiplus::StringAlignmentNear);
	graphics->DrawString(text, -1, font, pointF, Gdiplus::StringFormat::GenericTypographic(), &solidBrush);
}

int gdiplus_container::pt_to_px( int pt ) const
{
	HDC dc = GetDC(NULL);
	int ret = MulDiv(pt, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(NULL, dc);
	return ret;
}

int gdiplus_container::get_default_font_size() const
{
	return 16;
}

void gdiplus_container::draw_list_marker( litehtml::uint_ptr hdc, const litehtml::list_marker& marker )
{

}

void gdiplus_container::load_image( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready )
{
	std::wstring url;
	t_make_url(src, baseurl, url);
	lock_images_cache();
	if (m_images.find(url.c_str()) == m_images.end())
	{
		unlock_images_cache();
		litehtml::uint_ptr img = get_image(url.c_str(), redraw_on_ready);
		lock_images_cache();
		m_images[url] = img;
		unlock_images_cache();
	} else
	{
		unlock_images_cache();
	}
}

void gdiplus_container::get_image_size( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz )
{
	std::wstring url;
	t_make_url(src, baseurl, url);

	sz.width	= 0;
	sz.height	= 0;

	lock_images_cache();
	images_map::iterator imgmap = m_images.find(url.c_str());
	if (imgmap != m_images.end() && imgmap->second)
	{
		Gdiplus::Bitmap* bmp = (Gdiplus::Bitmap*)imgmap->second;
		if (bmp)
		{
			sz.width = bmp->GetWidth();
			sz.height = bmp->GetHeight();
		}
	}
	unlock_images_cache();
}

void gdiplus_container::draw_image( litehtml::uint_ptr hdc, const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, const litehtml::position& pos )
{
	apply_clip(hdc);

	std::wstring url;
	make_url(src, baseurl, url);

	lock_images_cache();
	images_map::iterator imgmap = m_images.find(url.c_str());
	if (imgmap != m_images.end())
	{
		Gdiplus::Bitmap* bmp = (Gdiplus::Bitmap*)imgmap->second;
		if (bmp)
		{
			Gdiplus::Graphics* graphics = (Gdiplus::Graphics*)hdc;
			graphics->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
			graphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
			graphics->DrawImage(bmp, pos.x, pos.y, pos.width, pos.height);
		}
	}
	lock_images_cache();

	release_clip(hdc);
}

void gdiplus_container::draw_background( litehtml::uint_ptr hdc, const litehtml::background_paint& bg )
{
	apply_clip(hdc);

	std::wstring url;
	make_url(bg.image.c_str(), bg.baseurl.c_str(), url);

	lock_images_cache();
	images_map::iterator imgmap = m_images.find(url.c_str());
	if (imgmap != m_images.end() && imgmap->second)
	{
		litehtml::position draw_pos = bg.clip_box;
		litehtml::position pos = draw_pos;
		pos.x = bg.position_x;
		pos.y = bg.position_y;

		Gdiplus::Bitmap* bgbmp = (Gdiplus::Bitmap*)imgmap->second;
		int img_width  = bgbmp->GetWidth();
		int img_height = bgbmp->GetHeight();

		/* output_debug_string("draw_background(");
		output_debug_string(url.c_str());
		output_debug_string(")-> draw_pos: ");
		output_debug_string(draw_pos.left());
		output_debug_string(",");
		output_debug_string(draw_pos.top());
		output_debug_string(",");
		output_debug_string(draw_pos.width);
		output_debug_string(",");
		output_debug_string(draw_pos.height);
		output_debug_string(";");
		output_debug_string(img_width);
		output_debug_string(",");
		output_debug_string(img_height);
		output_debug_string("\n"); */

		Gdiplus::Graphics* graphics = (Gdiplus::Graphics*)hdc;
		graphics->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
		graphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

		Gdiplus::Region reg(Gdiplus::Rect(draw_pos.left(), draw_pos.top(), draw_pos.width, draw_pos.height));
		graphics->SetClip(&reg);

		switch (bg.repeat)
		{
		    case litehtml::background_repeat_no_repeat:
		    {
			    graphics->DrawImage(bgbmp, pos.x, pos.y, bgbmp->GetWidth(), bgbmp->GetHeight());
		    }
		    break;
		    case litehtml::background_repeat_repeat_x:
		    {
			    Gdiplus::CachedBitmap bmp(bgbmp, graphics);
			    for (int x = pos.left(); x < pos.right(); x += bgbmp->GetWidth())
			    {
				    graphics->DrawCachedBitmap(&bmp, x, pos.top());
			    }

			    for (int x = pos.left() - bgbmp->GetWidth(); x + (int)bgbmp->GetWidth() > draw_pos.left(); x -= bgbmp->GetWidth())
			    {
				    graphics->DrawCachedBitmap(&bmp, x, pos.top());
			    }
		    }
		    break;
		    case litehtml::background_repeat_repeat_y:
		    {
			    Gdiplus::CachedBitmap bmp(bgbmp, graphics);
			    for (int y = pos.top(); y < pos.bottom(); y += bgbmp->GetHeight())
			    {
				    graphics->DrawCachedBitmap(&bmp, pos.left(), y);
			    }

			    for (int y = pos.top() - bgbmp->GetHeight(); y + (int)bgbmp->GetHeight() > draw_pos.top(); y -= bgbmp->GetHeight())
			    {
				    graphics->DrawCachedBitmap(&bmp, pos.left(), y);
			    }
		    }
		    break;
		    case litehtml::background_repeat_repeat:
		    {
			    Gdiplus::CachedBitmap bmp(bgbmp, graphics);
			    if (bgbmp->GetHeight() >= 0)
			    {
				    for (int x = pos.left(); x < pos.right(); x += bgbmp->GetWidth())
				    {
					    for (int y = pos.top(); y < pos.bottom(); y += bgbmp->GetHeight())
					    {
						graphics->DrawCachedBitmap(&bmp, x, y);
					    }
				    }
			    }
		    }
		    break;
		}
	}
	unlock_images_cache();

	release_clip(hdc);
}

void gdiplus_container::draw_borders( litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root )
{
	if (!hdc)
	{
		return;
	}
	Gdiplus::Graphics* graphics = (Gdiplus::Graphics*)hdc;

	/* output_debug_string("draw_borders->draw_pos: ");
	output_debug_string(draw_pos.left());
	output_debug_string(",");
	output_debug_string(borders.left.width);
	output_debug_string(";");
	output_debug_string(draw_pos.top());
	output_debug_string(",");
	output_debug_string(borders.top.width);
	output_debug_string(";");
	output_debug_string(draw_pos.right());
	output_debug_string(",");
	output_debug_string(borders.right.width);
	output_debug_string(";");
	output_debug_string(draw_pos.bottom());
	output_debug_string(",");
	output_debug_string(borders.bottom.width);
	output_debug_string("\n"); */


	apply_clip(hdc);

	// draw left border
	if (borders.left.width != 0 && borders.left.style > litehtml::border_style_hidden)
	{
		Gdiplus::Pen pen(Gdiplus::Color(borders.left.color.alpha, borders.left.color.red, borders.left.color.green, borders.left.color.blue));
		for (int x = 0; x < borders.left.width; x++)
		{
			Gdiplus::PointF pointStart(draw_pos.left() + x, draw_pos.top());
			Gdiplus::PointF pointEnd(draw_pos.left() + x, draw_pos.bottom());
			graphics->DrawLine(&pen, pointStart, pointEnd);
		}
	}
	// draw right border
	if (borders.right.width != 0 && borders.right.style > litehtml::border_style_hidden)
	{
		Gdiplus::Pen pen(Gdiplus::Color(borders.right.color.alpha, borders.right.color.red, borders.right.color.green, borders.right.color.blue));
		for (int x = 0; x < borders.right.width; x++)
		{
			Gdiplus::PointF pointStart(draw_pos.right() - x, draw_pos.top());
			Gdiplus::PointF pointEnd(draw_pos.right() - x, draw_pos.bottom());
			graphics->DrawLine(&pen, pointStart, pointEnd);
		}
	}
	// draw top border
	if (borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden)
	{
		Gdiplus::Pen pen(Gdiplus::Color(borders.top.color.alpha, borders.top.color.red, borders.top.color.green, borders.top.color.blue));
		for (int y = 0; y < borders.top.width; y++)
		{
			Gdiplus::PointF pointStart(draw_pos.left(), draw_pos.top() + y);
			Gdiplus::PointF pointEnd(draw_pos.right(), draw_pos.top() + y);
			graphics->DrawLine(&pen, pointStart, pointEnd);
		}
	}
	// draw bottom border
	if (borders.bottom.width != 0 && borders.bottom.style > litehtml::border_style_hidden)
	{
		Gdiplus::Pen pen(Gdiplus::Color(borders.bottom.color.alpha, borders.bottom.color.red, borders.bottom.color.green, borders.bottom.color.blue));
		for (int y = 0; y < borders.bottom.width; y++)
		{
			Gdiplus::PointF pointStart(draw_pos.left(), draw_pos.bottom() - y);
			Gdiplus::PointF pointEnd(draw_pos.right(), draw_pos.bottom() - y);
			graphics->DrawLine(&pen, pointStart, pointEnd);
		}
	}

	release_clip(hdc);
}


void gdiplus_container::fill_rect(litehtml::uint_ptr hdc, int x, int y, int width, int height, const litehtml::web_color& color)
{
	if (!hdc)
	{
		return;
	}
	Gdiplus::Graphics* graphics = (Gdiplus::Graphics*)hdc;

	Gdiplus::SolidBrush brush(Gdiplus::Color(color.alpha, color.red, color.green, color.blue));
	graphics->FillRectangle(&brush, x, y, width, height);
}


void gdiplus_container::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y)
{
	litehtml::position clip_pos = pos;
	litehtml::position client_pos;
	get_client_rect(client_pos);
	if (!valid_x)
	{
		clip_pos.x		= client_pos.x;
		clip_pos.width	= client_pos.width;
	}
	if(!valid_y)
	{
		clip_pos.y		= client_pos.y;
		clip_pos.height	= client_pos.height;
	}
	m_clips.emplace_back(clip_pos, bdr_radius);
}

void gdiplus_container::del_clip()
{
	if(!m_clips.empty())
	{
		m_clips.pop_back();
	}
}

void gdiplus_container::apply_clip(litehtml::uint_ptr hdc)
{
	if (!hdc)
	{
		return;
	}
	Gdiplus::Graphics* graphics = (Gdiplus::Graphics*)hdc;

	for (const auto& clip_box : m_clips)
	{
		Gdiplus::Region reg(Gdiplus::Rect(clip_box.box.left(), clip_box.box.top(), clip_box.box.width, clip_box.box.height));
		graphics->SetClip(&reg, Gdiplus::CombineModeIntersect);
	}
}

void gdiplus_container::release_clip(litehtml::uint_ptr hdc)
{
	if (!hdc)
	{
		return;
	}
	Gdiplus::Graphics* graphics = (Gdiplus::Graphics*)hdc;
	graphics->ResetClip();
}

/* void myGdiplus_container::draw_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, double line_width)
{
}

void myGdiplus_container::fill_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color )
{
} */

void gdiplus_container::clear_images()
{
	lock_images_cache();
	m_images.clear();
	unlock_images_cache();
}

const litehtml::tchar_t* gdiplus_container::get_default_font_name() const
{
	return _t("Times New Roman");
}

void gdiplus_container::remove_image( std::wstring& url )
{
	lock_images_cache();
	images_map::iterator i = m_images.find(url);
	if(i != m_images.end())
	{
		m_images.erase(i);
	}
	unlock_images_cache();
}

void gdiplus_container::add_image(std::wstring& url, litehtml::uint_ptr& img)
{
	lock_images_cache();
	images_map::iterator i = m_images.find(url);
	if(i != m_images.end())
	{
		if (img)
		{
			i->second = img;
		} else
		{
			m_images.erase(i);
		}
	}
	unlock_images_cache();
}

void gdiplus_container::lock_images_cache()
{
	EnterCriticalSection(&m_img_sync);
}

void gdiplus_container::unlock_images_cache()
{
	LeaveCriticalSection(&m_img_sync);
}

std::shared_ptr<litehtml::element> gdiplus_container::create_element(const litehtml::tchar_t* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc)
{
	return 0;
}

void gdiplus_container::get_media_features(litehtml::media_features& media)  const
{
	litehtml::position client;
	get_client_rect(client);
	HDC hdc = GetDC(NULL);

	media.type			= litehtml::media_type_screen;
	media.width			= client.width;
	media.height		= client.height;
	media.color			= 8;
	media.monochrome	= 0;
	media.color_index	= 256;
	media.resolution	= GetDeviceCaps(hdc, LOGPIXELSX);
	media.device_width	= GetDeviceCaps(hdc, HORZRES);
	media.device_height	= GetDeviceCaps(hdc, VERTRES);

	ReleaseDC(NULL, hdc);
}

void gdiplus_container::get_language(litehtml::tstring& language, litehtml::tstring & culture) const
{
	language = _t("en");
	culture = _t("");
}

void gdiplus_container::make_url_utf8( const char* url, const char* basepath, std::wstring& out )
{

}

void gdiplus_container::transform_text( litehtml::tstring& text, litehtml::text_transform tt )
{
	if(text.empty()) return;

#ifndef LITEHTML_UTF8
	switch(tt)
	{
	case litehtml::text_transform_capitalize:
		if(!text.empty())
		{
			text[0] = (WCHAR) CharUpper((LPWSTR) text[0]);
		}
		break;
	case litehtml::text_transform_uppercase:
		for(size_t i = 0; i < text.length(); i++)
		{
			text[i] = (WCHAR) CharUpper((LPWSTR) text[i]);
		}
		break;
	case litehtml::text_transform_lowercase:
		for(size_t i = 0; i < text.length(); i++)
		{
			text[i] = (WCHAR) CharLower((LPWSTR) text[i]);
		}
		break;
	}
#else
	LPWSTR txt = cairo_font::utf8_to_wchar(text.c_str());
	switch(tt)
	{
	case litehtml::text_transform_capitalize:
		CharUpperBuff(txt, 1);
		break;
	case litehtml::text_transform_uppercase:
		CharUpperBuff(txt, lstrlen(txt));
		break;
	case litehtml::text_transform_lowercase:
		CharLowerBuff(txt, lstrlen(txt));
		break;
	}
	LPSTR txtA = cairo_font::wchar_to_utf8(txt);
	text = txtA;
	delete txtA;
	delete txt;
#endif
}

void gdiplus_container::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el)
{
}

litehtml::tstring gdiplus_container::resolve_color(const litehtml::tstring& color) const
{
	struct custom_color
	{
		litehtml::tchar_t*	name;
		int					color_index;
	};

	static custom_color colors[] = {
		{ _t("ActiveBorder"),          COLOR_ACTIVEBORDER},
		{ _t("ActiveCaption"),         COLOR_ACTIVECAPTION},
		{ _t("AppWorkspace"),          COLOR_APPWORKSPACE },
		{ _t("Background"),            COLOR_BACKGROUND },
		{ _t("ButtonFace"),            COLOR_BTNFACE },
		{ _t("ButtonHighlight"),       COLOR_BTNHIGHLIGHT },
		{ _t("ButtonShadow"),          COLOR_BTNSHADOW },
		{ _t("ButtonText"),            COLOR_BTNTEXT },
		{ _t("CaptionText"),           COLOR_CAPTIONTEXT },
        { _t("GrayText"),              COLOR_GRAYTEXT },
		{ _t("Highlight"),             COLOR_HIGHLIGHT },
		{ _t("HighlightText"),         COLOR_HIGHLIGHTTEXT },
		{ _t("InactiveBorder"),        COLOR_INACTIVEBORDER },
		{ _t("InactiveCaption"),       COLOR_INACTIVECAPTION },
		{ _t("InactiveCaptionText"),   COLOR_INACTIVECAPTIONTEXT },
		{ _t("InfoBackground"),        COLOR_INFOBK },
		{ _t("InfoText"),              COLOR_INFOTEXT },
		{ _t("Menu"),                  COLOR_MENU },
		{ _t("MenuText"),              COLOR_MENUTEXT },
		{ _t("Scrollbar"),             COLOR_SCROLLBAR },
		{ _t("ThreeDDarkShadow"),      COLOR_3DDKSHADOW },
		{ _t("ThreeDFace"),            COLOR_3DFACE },
		{ _t("ThreeDHighlight"),       COLOR_3DHILIGHT },
		{ _t("ThreeDLightShadow"),     COLOR_3DLIGHT },
		{ _t("ThreeDShadow"),          COLOR_3DSHADOW },
		{ _t("Window"),                COLOR_WINDOW },
		{ _t("WindowFrame"),           COLOR_WINDOWFRAME },
		{ _t("WindowText"),            COLOR_WINDOWTEXT }
	};

    if (color == _t("Highlight"))
    {
        int iii = 0;
        iii++;
    }

    for (auto& clr : colors)
    {
		if (!litehtml::t_strcasecmp(clr.name, color.c_str()))
        {
            litehtml::tchar_t  str_clr[20];
            DWORD rgb_color =  GetSysColor(clr.color_index);
#ifdef LITEHTML_UTF8
            StringCchPrintfA(str_clr, 20, "#%02X%02X%02X", GetRValue(rgb_color), GetGValue(rgb_color), GetBValue(rgb_color));
#else
            StringCchPrintf(str_clr, 20, L"#%02X%02X%02X", GetRValue(rgb_color), GetGValue(rgb_color), GetBValue(rgb_color));
#endif // LITEHTML_UTF8
            return std::move(litehtml::tstring(str_clr));
        }
    }
    return std::move(litehtml::tstring());
}
