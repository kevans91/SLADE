
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SAuiTabArt.cpp
 * Description: Custom tab art provider for wxAuiNotebook, based on
 *              wxAuiGenericTabArt. Source copied over from wx and
 *              modified (hence the mess). Also includes custom dock
 *              art class
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "SAuiTabArt.h"
#include "Drawing.h"

#include <wx/image.h>
#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcclient.h>
#include <wx/renderer.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
#if defined( __WXMAC__ )
static const unsigned char close_bits[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFE, 0x03, 0xF8, 0x01, 0xF0, 0x19, 0xF3,
	0xB8, 0xE3, 0xF0, 0xE1, 0xE0, 0xE0, 0xF0, 0xE1, 0xB8, 0xE3, 0x19, 0xF3,
	0x01, 0xF0, 0x03, 0xF8, 0x0F, 0xFE, 0xFF, 0xFF };
#elif defined( __UGLY_CLOSE_BUTTON__ )
static const unsigned char close_bits[] = {
	0xff, 0xff, 0xff, 0xff, 0x07, 0xf0, 0xfb, 0xef, 0xdb, 0xed, 0x8b, 0xe8,
	0x1b, 0xec, 0x3b, 0xee, 0x1b, 0xec, 0x8b, 0xe8, 0xdb, 0xed, 0xfb, 0xef,
	0x07, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
#else
static const unsigned char close_bits[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xf3, 0xcf, 0xf9,
	0x9f, 0xfc, 0x3f, 0xfe, 0x3f, 0xfe, 0x9f, 0xfc, 0xcf, 0xf9, 0xe7, 0xf3,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
#endif

static const unsigned char left_bits[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xfe, 0x3f, 0xfe,
	0x1f, 0xfe, 0x0f, 0xfe, 0x1f, 0xfe, 0x3f, 0xfe, 0x7f, 0xfe, 0xff, 0xfe,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static const unsigned char right_bits[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0x9f, 0xff, 0x1f, 0xff,
	0x1f, 0xfe, 0x1f, 0xfc, 0x1f, 0xfe, 0x1f, 0xff, 0x9f, 0xff, 0xdf, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static const unsigned char list_bits[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x0f, 0xf8, 0xff, 0xff, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f, 0xfe, 0x7f, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

static wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size)
{
	wxCoord x, y;

	// first check if the text fits with no problems
	dc.GetTextExtent(text, &x, &y);
	if (x <= max_size)
		return text;

	size_t i, len = text.Length();
	size_t last_good_length = 0;
	for (i = 0; i < len; ++i) {
		wxString s = text.Left(i);
		s += wxT("...");

		dc.GetTextExtent(s, &x, &y);
		if (x > max_size)
			break;

		last_good_length = i;
	}

	wxString ret = text.Left(last_good_length);
	ret += wxT("...");
	return ret;
}

static void IndentPressedBitmap(wxRect* rect, int button_state)
{
	if (button_state == wxAUI_BUTTON_STATE_PRESSED) {
		rect->x++;
		rect->y++;
	}
}

wxBitmap bitmapFromBits(const unsigned char bits[], int w, int h, const wxColour& color)
{
	wxImage img = wxBitmap((const char*)bits, w, h).ConvertToImage();
	img.Replace(0, 0, 0, 123, 123, 123);
	img.Replace(255, 255, 255, color.Red(), color.Green(), color.Blue());
	img.SetMaskColour(123, 123, 123);
	return wxBitmap(img);
}


/*******************************************************************
 * SAUITABART CLASS FUNCTIONS
 *******************************************************************/

SAuiTabArt::SAuiTabArt(bool close_buttons)
{
	m_normalFont = *wxNORMAL_FONT;
	m_selectedFont = *wxNORMAL_FONT;
	m_measuringFont = m_selectedFont;
	m_closeButtons = close_buttons;

	m_fixedTabWidth = 100;
	m_tabCtrlHeight = 0;

	wxColor baseColour = Drawing::getPanelBGColour();

	m_activeColour = baseColour;
	m_baseColour = baseColour;
	wxColor borderColour = baseColour.ChangeLightness(75);
	m_inactiveTabColour = Drawing::darkColour(m_baseColour, 0.95f);

	m_borderPen = wxPen(borderColour);
	m_baseColourPen = wxPen(m_baseColour);
	m_baseColourBrush = wxBrush(m_baseColour);

	m_activeCloseBmp = bitmapFromBits(close_bits, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	m_disabledCloseBmp = bitmapFromBits(close_bits, 16, 16, wxColour(128, 128, 128));

	m_activeLeftBmp = bitmapFromBits(left_bits, 16, 16, *wxBLACK);
	m_disabledLeftBmp = bitmapFromBits(left_bits, 16, 16, wxColour(128, 128, 128));

	m_activeRightBmp = bitmapFromBits(right_bits, 16, 16, *wxBLACK);
	m_disabledRightBmp = bitmapFromBits(right_bits, 16, 16, wxColour(128, 128, 128));

	m_activeWindowListBmp = bitmapFromBits(list_bits, 16, 16, *wxBLACK);
	m_disabledWindowListBmp = bitmapFromBits(list_bits, 16, 16, wxColour(128, 128, 128));

	m_flags = 0;
}

SAuiTabArt::~SAuiTabArt()
{
}

wxAuiTabArt* SAuiTabArt::Clone()
{
	return new SAuiTabArt(*this);
}

void SAuiTabArt::DrawBorder(wxDC& dc, wxWindow* wnd, const wxRect& rect)
{
	//wxAuiGenericTabArt::DrawBorder(dc, wnd, rect);
	//int i, border_width = GetBorderWidth(wnd);

	int height = ((wxAuiNotebook*)wnd)->GetTabCtrlHeight() - 3;

	wxRect theRect(rect);
	//for (i = 0; i < border_width; ++i)
	//{
	//dc.SetPen(wxPen(m_baseColour));
		//dc.DrawRectangle(theRect.x, theRect.y, theRect.width, theRect.height);


		//dc.SetPen(wxPen(m_baseColour));
		dc.DrawLine(theRect.x, theRect.y + height, theRect.x, theRect.y + theRect.height);
		dc.DrawLine(theRect.x + theRect.width - 1, theRect.y + height, theRect.x + theRect.width - 1, theRect.y + theRect.height);
		dc.DrawLine(theRect.x, theRect.y + theRect.height - 1, theRect.x + theRect.width, theRect.y + theRect.height - 1);

		dc.SetPen(wxPen(m_baseColour));
		dc.DrawLine(theRect.x, theRect.y, theRect.x, theRect.y + height);
		dc.DrawLine(theRect.x + theRect.width - 1, theRect.y, theRect.x + theRect.width - 1, theRect.y + height);
		dc.DrawLine(theRect.x, theRect.y, theRect.x + theRect.width, theRect.y);

//theRect.Deflate(1);
	//}
}

void SAuiTabArt::DrawBackground(wxDC& dc,
	wxWindow* WXUNUSED(wnd),
	const wxRect& rect)
{
	// draw background
	wxColor top_color = m_baseColour;
	wxColor bottom_color = m_baseColour;
	wxRect r;

	if (m_flags &wxAUI_NB_BOTTOM)
		r = wxRect(rect.x, rect.y, rect.width + 2, rect.height);
	else
		r = wxRect(rect.x, rect.y, rect.width + 2, rect.height - 3);

	dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);


	// draw base lines
	dc.SetPen(wxPen(m_baseColour));
	int y = rect.GetHeight();
	int w = rect.GetWidth();

	if (m_flags &wxAUI_NB_BOTTOM)
	{
		dc.SetBrush(wxBrush(bottom_color));
		dc.DrawRectangle(-1, 0, w + 2, 4);
	}
	else
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(wxBrush(m_activeColour));// m_baseColourBrush);
		dc.DrawRectangle(-1, y - 4, w + 2, 4);

		dc.SetPen(m_borderPen);
		dc.DrawLine(-2, y - 4, w + 2, y - 4);
	}
}


// DrawTab() draws an individual tab.
//
// dc       - output dc
// in_rect  - rectangle the tab should be confined to
// caption  - tab's caption
// active   - whether or not the tab is active
// out_rect - actual output rectangle
// x_extent - the advance x; where the next tab should start

void SAuiTabArt::DrawTab(wxDC& dc,
	wxWindow* wnd,
	const wxAuiNotebookPage& page,
	const wxRect& in_rect,
	int close_button_state,
	wxRect* out_tab_rect,
	wxRect* out_button_rect,
	int* x_extent)
{
	wxCoord normal_textx, normal_texty;
	wxCoord selected_textx, selected_texty;
	wxCoord texty;

	// if the caption is empty, measure some temporary text
	wxString caption = page.caption;
	if (caption.empty())
		caption = wxT("Xj");

	dc.SetFont(m_selectedFont);
	dc.GetTextExtent(caption, &selected_textx, &selected_texty);

	dc.SetFont(m_normalFont);
	dc.GetTextExtent(caption, &normal_textx, &normal_texty);

	// figure out the size of the tab
	wxSize tab_size = GetTabSize(dc,
		wnd,
		page.caption,
		page.bitmap,
		page.active,
		close_button_state,
		x_extent);

	wxCoord tab_height = m_tabCtrlHeight - 1;// -3;
	wxCoord tab_width = tab_size.x;
	wxCoord tab_x = in_rect.x;
	wxCoord tab_y = in_rect.y + in_rect.height - tab_height;


	if (!page.active)
	{
		tab_height -= 2;
		tab_y += 2;
	}

	caption = page.caption;


	// select pen, brush and font for the tab to be drawn
	if (page.active)
	{
		dc.SetFont(m_selectedFont);
		texty = selected_texty;
	}
	else
	{
		dc.SetFont(m_normalFont);
		texty = normal_texty;
	}


	// create points that will make the tab outline
	int clip_width = tab_width;
	if (tab_x + clip_width > in_rect.x + in_rect.width)
		clip_width = (in_rect.x + in_rect.width) - tab_x;
	dc.SetClippingRegion(tab_x, tab_y, clip_width + 1, tab_height - 3);

	wxPoint border_points[6];
	if (m_flags &wxAUI_NB_BOTTOM)
	{
		border_points[0] = wxPoint(tab_x, tab_y);
		border_points[1] = wxPoint(tab_x, tab_y + tab_height - 4);
		border_points[2] = wxPoint(tab_x, tab_y + tab_height - 4);
		border_points[3] = wxPoint(tab_x + tab_width, tab_y + tab_height - 4);
		border_points[4] = wxPoint(tab_x + tab_width, tab_y + tab_height - 4);
		border_points[5] = wxPoint(tab_x + tab_width, tab_y);
	}
	else
	{
		border_points[0] = wxPoint(tab_x, tab_y + tab_height - 4);
		border_points[1] = wxPoint(tab_x, tab_y);
		border_points[2] = wxPoint(tab_x + 2, tab_y);
		border_points[3] = wxPoint(tab_x + tab_width - 2, tab_y);
		border_points[4] = wxPoint(tab_x + tab_width, tab_y);
		border_points[5] = wxPoint(tab_x + tab_width, tab_y + tab_height - 4);
	}

	int drawn_tab_yoff = border_points[1].y + 1;
	int drawn_tab_height = border_points[0].y - border_points[1].y;


	wxColour bgcol;
	if (page.active)
	{
		// draw active tab
		bgcol = m_activeColour;

		// draw base background color
		wxRect r(tab_x, tab_y, tab_width, tab_height);
		dc.SetPen(wxPen(m_activeColour));
		dc.SetBrush(wxBrush(m_activeColour));
		dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, r.height - 5);

		// highlight top of tab
		wxColour col_hilight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
		dc.SetPen(*wxTRANSPARENT_PEN);// *wxWHITE_PEN);
		dc.SetBrush(wxBrush(col_hilight));// *wxWHITE_BRUSH);
		dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, 1);
	}
	else
	{
		bgcol = m_inactiveTabColour;

		wxRect r(tab_x, tab_y, tab_width, tab_height);
		wxPoint mouse = wnd->ScreenToClient(wxGetMousePosition());
		/*if (r.Contains(mouse))
		{
			dc.SetPen(wxPen(m_activeColour));
			dc.SetBrush(wxBrush(m_activeColour));
		}
		else
		{*/
			dc.SetPen(wxPen(m_inactiveTabColour));
			dc.SetBrush(wxBrush(m_inactiveTabColour));
		//}
		dc.DrawRectangle(r.x + 1, r.y + 1, r.width - 1, r.height - 4);
	}

	// draw tab outline
	dc.SetPen(m_borderPen);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawPolygon(WXSIZEOF(border_points), border_points);

	// there are two horizontal grey lines at the bottom of the tab control,
	// this gets rid of the top one of those lines in the tab control
	if (page.active)
	{
		if (m_flags &wxAUI_NB_BOTTOM)
			dc.SetPen(wxPen(m_baseColour.ChangeLightness(170)));
		else
			dc.SetPen(wxPen(m_activeColour));
		dc.DrawLine(border_points[0].x + 1,
			border_points[0].y,
			border_points[5].x,
			border_points[5].y);
	}


	int text_offset = tab_x + 8;
	int close_button_width = 0;
	if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
	{
		close_button_width = m_activeCloseBmp.GetWidth();
	}

	int bitmap_offset = 0;
	if (page.bitmap.IsOk())
	{
		bitmap_offset = tab_x + 8;

		// draw bitmap
		dc.DrawBitmap(page.bitmap,
			bitmap_offset,
			drawn_tab_yoff + (drawn_tab_height / 2) - (page.bitmap.GetHeight() / 2),
			true);

		text_offset = bitmap_offset + page.bitmap.GetWidth();
		text_offset += 4; // bitmap padding

	}
	else
	{
		text_offset = tab_x + 8;
	}

	dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	wxString draw_text = wxAuiChopText(dc,
		caption,
		tab_width - (text_offset - tab_x) - close_button_width);

	// draw tab text
	dc.DrawText(draw_text,
		text_offset,
		drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 0);

	// draw focus rectangle
	if (page.active && (wnd->FindFocus() == wnd))
	{
		wxRect focusRectText(text_offset, (drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 1),
			selected_textx, selected_texty);

		wxRect focusRect;
		wxRect focusRectBitmap;

		if (page.bitmap.IsOk())
			focusRectBitmap = wxRect(bitmap_offset, drawn_tab_yoff + (drawn_tab_height / 2) - (page.bitmap.GetHeight() / 2),
				page.bitmap.GetWidth(), page.bitmap.GetHeight());

		if (page.bitmap.IsOk() && draw_text.IsEmpty())
			focusRect = focusRectBitmap;
		else if (!page.bitmap.IsOk() && !draw_text.IsEmpty())
			focusRect = focusRectText;
		else if (page.bitmap.IsOk() && !draw_text.IsEmpty())
			focusRect = focusRectText.Union(focusRectBitmap);

		focusRect.Inflate(2, 2);

		wxRendererNative::Get().DrawFocusRect(wnd, dc, focusRect, 0);
	}

	// draw close button if necessary
	if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
	{
		wxBitmap bmp = m_disabledCloseBmp;

		int offsetY = tab_y;
		if (m_flags & wxAUI_NB_BOTTOM)
			offsetY = 1;

		wxRect rect(tab_x + tab_width - close_button_width - 3,
			offsetY + (tab_height / 2) - (bmp.GetHeight() / 2),
			close_button_width,
			tab_height);

		IndentPressedBitmap(&rect, close_button_state);

		if (close_button_state == wxAUI_BUTTON_STATE_HOVER ||
			close_button_state == wxAUI_BUTTON_STATE_PRESSED)
		{
			/*wxColour bcol = m_borderPen.GetColour();
			float r = ((float)bcol.Red() * 0.2f) + ((float)m_activeColour.Red() * 0.8f);
			float g = ((float)bcol.Green() * 0.2f) + ((float)m_activeColour.Green() * 0.8f);
			float b = ((float)bcol.Blue() * 0.2f) + ((float)m_activeColour.Blue() * 0.8f);*/
			//captionAccentColour = wxColor(r, g, b);

			//dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetPen(wxPen(Drawing::darkColour(bgcol, 2.0f)));
			dc.SetBrush(wxBrush(Drawing::lightColour(bgcol, 1.0f)));
			dc.DrawRectangle(rect.x, rect.y + 1, rect.width - 1, rect.width - 2);

			bmp = m_activeCloseBmp;
			//dc.DrawBitmap(bmp, rect.x + 1, rect.y, true);
			//dc.DrawBitmap(bmp, rect.x - 1, rect.y, true);
			dc.DrawBitmap(bmp, rect.x, rect.y, true);
		}
		else
		{
			bmp = m_disabledCloseBmp;
			dc.DrawBitmap(bmp, rect.x, rect.y, true);
		}

		*out_button_rect = rect;
	}

	*out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

	dc.DestroyClippingRegion();
}

wxSize SAuiTabArt::GetTabSize(wxDC& dc,
	wxWindow* WXUNUSED(wnd),
	const wxString& caption,
	const wxBitmap& bitmap,
	bool WXUNUSED(active),
	int close_button_state,
	int* x_extent)
{
	wxCoord measured_textx, measured_texty, tmp;

	dc.SetFont(m_measuringFont);
	dc.GetTextExtent(caption, &measured_textx, &measured_texty);

	dc.GetTextExtent(wxT("ABCDEFXj"), &tmp, &measured_texty);

	// add padding around the text
	wxCoord tab_width = measured_textx;
	wxCoord tab_height = measured_texty;

	// if close buttons are enabled, add space for one
	if (m_closeButtons)
		tab_width += m_activeCloseBmp.GetWidth();

	// if there's a bitmap, add space for it
	if (bitmap.IsOk())
	{
		tab_width += bitmap.GetWidth();
		tab_width += 3; // right side bitmap padding
		tab_height = wxMax(tab_height, bitmap.GetHeight());
	}

	// add padding
	tab_width += 16;
	tab_height += 10;

	if (m_flags & wxAUI_NB_TAB_FIXED_WIDTH)
	{
		tab_width = m_fixedTabWidth;
	}

	*x_extent = tab_width;

	return wxSize(tab_width, tab_height);
}

void SAuiTabArt::SetSelectedFont(const wxFont& font)
{
	//m_selectedFont = font;
}


/*******************************************************************
 * SAUIDOCKART CLASS FUNCTIONS
 *******************************************************************/

SAuiDockArt::SAuiDockArt()
{
	captionBackColour = Drawing::darkColour(Drawing::getPanelBGColour(), 0.0f);

	wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	float r = ((float)textColour.Red() * 0.2f) + ((float)captionBackColour.Red() * 0.8f);
	float g = ((float)textColour.Green() * 0.2f) + ((float)captionBackColour.Green() * 0.8f);
	float b = ((float)textColour.Blue() * 0.2f) + ((float)captionBackColour.Blue() * 0.8f);
	captionAccentColour = wxColor(r, g, b);

	m_activeCloseBitmap = bitmapFromBits(close_bits, 16, 16, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	m_inactiveCloseBitmap = bitmapFromBits(close_bits, 16, 16, wxColour(128, 128, 128));

	m_captionSize = 19;
	m_sashSize = 4;
}

SAuiDockArt::~SAuiDockArt()
{
}

void SAuiDockArt::DrawCaption(wxDC& dc,
	wxWindow *window,
	const wxString& text,
	const wxRect& rect,
	wxAuiPaneInfo& pane)
{
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetFont(m_captionFont);

	dc.SetBrush(wxBrush(captionBackColour));
	dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

	//dc.SetPen(m_borderPen);
	//dc.SetBrush(wxBrush(Drawing::darkColour(captionBackColour, 2.0f)));
	//dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

	wxColor sepCol;
	int l = rgba_t(captionBackColour.Red(), captionBackColour.Green(), captionBackColour.Blue()).greyscale().r;
	if (l < 100)
		sepCol = Drawing::lightColour(captionBackColour, 2.0f);
	else
		sepCol = Drawing::darkColour(captionBackColour, 2.0f);

	//dc.SetPen(wxPen(sepCol));
	//dc.DrawLine(rect.x, rect.y + rect.height - 1, rect.x + rect.width, rect.y + rect.height - 1);

	dc.SetBrush(wxBrush(sepCol));
	dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height + 1);

	int caption_offset = 0;
	if (pane.icon.IsOk())
	{
		DrawIcon(dc, rect, pane);
		caption_offset += pane.icon.GetWidth() + 3;
	}

	dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

	wxRect clip_rect = rect;
	clip_rect.width -= 3; // text offset
	clip_rect.width -= 2; // button padding
	if (pane.HasCloseButton())
		clip_rect.width -= m_buttonSize;
	if (pane.HasPinButton())
		clip_rect.width -= m_buttonSize;
	if (pane.HasMaximizeButton())
		clip_rect.width -= m_buttonSize;

	wxString draw_text = wxAuiChopText(dc, text, clip_rect.width);
	wxCoord w, h;
	dc.GetTextExtent(draw_text, &w, &h);

	dc.SetClippingRegion(clip_rect);
#ifdef __WXMSW__
	dc.DrawText(draw_text, rect.x + 5 + caption_offset, rect.y + (rect.height / 2) - (h / 2));
#else
	dc.DrawText(draw_text, rect.x + 5 + caption_offset, rect.y + (rect.height / 2) - (h / 2) + 1);
#endif

	//dc.SetPen(wxPen(captionAccentColour));
	//dc.DrawLine(rect.x + w + 8, rect.y + (rect.height / 2) - 1, rect.x + rect.width - 16, rect.y + (rect.height / 2) - 1);
	//dc.DrawLine(rect.x + w + 8, rect.y + (rect.height / 2) + 1, rect.x + rect.width - 16, rect.y + (rect.height / 2) + 1);

	dc.DestroyClippingRegion();
}

void SAuiDockArt::DrawPaneButton(wxDC& dc, wxWindow *WXUNUSED(window),
	int button,
	int button_state,
	const wxRect& _rect,
	wxAuiPaneInfo& pane)
{
	wxBitmap bmp;
	if (!(&pane))
		return;
	switch (button)
	{
	default:
	case wxAUI_BUTTON_CLOSE:
		if (pane.state & wxAuiPaneInfo::optionActive)
			bmp = m_activeCloseBitmap;
		else
			bmp = m_inactiveCloseBitmap;
		break;
	case wxAUI_BUTTON_PIN:
		if (pane.state & wxAuiPaneInfo::optionActive)
			bmp = m_activePinBitmap;
		else
			bmp = m_inactivePinBitmap;
		break;
	case wxAUI_BUTTON_MAXIMIZE_RESTORE:
		if (pane.IsMaximized())
		{
			if (pane.state & wxAuiPaneInfo::optionActive)
				bmp = m_activeRestoreBitmap;
			else
				bmp = m_inactiveRestoreBitmap;
		}
		else
		{
			if (pane.state & wxAuiPaneInfo::optionActive)
				bmp = m_activeMaximizeBitmap;
			else
				bmp = m_inactiveMaximizeBitmap;
		}
		break;
	}


	wxRect rect = _rect;

	int old_y = rect.y;
	rect.y = rect.y + (rect.height / 2) - (bmp.GetHeight() / 2) + 1;
	rect.height = old_y + rect.height - rect.y - 1;
	//rect.y--;


	if (button_state == wxAUI_BUTTON_STATE_PRESSED)
	{
		rect.x++;
		rect.y++;
	}

	if (button_state == wxAUI_BUTTON_STATE_HOVER ||
		button_state == wxAUI_BUTTON_STATE_PRESSED)
	{
		/*if (pane.state & wxAuiPaneInfo::optionActive)
		{
			dc.SetBrush(wxBrush(m_activeCaptionColour.ChangeLightness(120)));
			dc.SetPen(wxPen(m_activeCaptionColour.ChangeLightness(70)));
		}
		else
		{
			dc.SetBrush(wxBrush(m_inactiveCaptionColour.ChangeLightness(120)));
			dc.SetPen(wxPen(m_inactiveCaptionColour.ChangeLightness(70)));
		}*/

		//dc.SetBrush(wxBrush(captionAccentColour));
		//dc.SetPen(wxPen(m_inactiveCaptionColour.ChangeLightness(70)));

		// draw the background behind the button
		//dc.DrawRectangle(rect.x, rect.y, 15, 15);

		dc.SetPen(wxPen(Drawing::darkColour(Drawing::getPanelBGColour(), 2.0f)));
		dc.SetBrush(wxBrush(Drawing::lightColour(Drawing::getPanelBGColour(), 1.0f)));
		dc.DrawRectangle(rect.x, rect.y, rect.width + 1, rect.width + 1);

		bmp = m_activeCloseBitmap;
		//dc.DrawBitmap(bmp, rect.x + 1, rect.y, true);
		//dc.DrawBitmap(bmp, rect.x - 1, rect.y, true);
		//dc.DrawBitmap(bmp, rect.x, rect.y, true);
	}


	// draw the button itself
	dc.DrawBitmap(bmp, rect.x, rect.y, true);
}
