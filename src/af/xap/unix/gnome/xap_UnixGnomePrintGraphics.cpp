/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* AbiSource Application Framework
 * Copyright (C) 1998 AbiSource, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

#include "ut_types.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_string.h"
#include "ut_misc.h"
#include "xap_Strings.h"
#include "xap_UnixGnomePrintGraphics.h"
#include "xap_EncodingManager.h"
#include "gr_UnixGnomeImage.h"
#include "ut_string_class.h"
#include <libgnomeprint/gnome-print-master-preview.h>

/***********************************************************************/
/*      This file provides an interface into Gnome Print               */
/***********************************************************************/

#define OUR_LINE_LIMIT          200 /* FIXME:
				       UGLY UGLY UGLY, but we need to fix the PrintPrivew
				       show bug. Test it with something like 7 when to see
				       what is the problem with PP. The show operator in
				       print preview does not move th currentpoint. Chema */

// the resolution that we report to the application (pixels per inch).
#define GPG_RESOLUTION		7200
#define GPG_DEFAULT_FONT        "Nimbus Roman No9 L"

/***********************************************************************/
/*      map abi's fonts to gnome fonts, or at least try to             */
/***********************************************************************/

struct _fontMapping 
{
        char *abi;   // what abiword calls a font's name
        char *gnome; // what gnome refers to it as (or a close substitute)
};

// mapping of the fonts that abi ships with
// to what gnome-font ships with
/* The ones with ?? have not been verified. (Chema) */
static struct _fontMapping fontMappingTable[] = 
{
		{"Arial",                  "Helvetica"}, // Arial is a MS name for Helvetica, so I've been told
		{"Bitstream",              "Palatino"}, // ??
		{"Bookman",                "URW Bookman L"},
		{"Century Schoolbook",     "Century Schoolbook L"},
		{"Courier",                "Courier"},
		{"Courier New",            "Courier"}, /* ??? not really. (I think) */
		{"Dingbats",               "Dingbats"},
		{"Goth",                   "URW Gothic L"},
		{"Helvetic",               "Helvetica"},
		{"Helvetica",              "Helvetica"},
		{"Nimbus Mono",            "Times"},
		{"Nimbus Roman",           "Times"},
		{"Nimbus Sans",            "Nimbus Sans L"},
		{"Nimbus Sans Condensed",  "Nimbus Sans L"}, // ??
		{"Palladio",               "URW Palladio L"},
		{"Standard Symbols",       "Symbol"},
		{"Symbol",                 "Symbol"}, // ?? (Symbol?)
		{"Times",                  "Times"},
		{"Times New Roman",        "Times"},
		{"*",                      GPG_DEFAULT_FONT}
};

#define TableSize	((sizeof(fontMappingTable)/sizeof(fontMappingTable[0])))

static char * mapFontName(const char *name)
{
		unsigned int idx = 0;
		// if we're passed crap, default to some normal font
		if(!name || !*name)
				{
						return GPG_DEFAULT_FONT;
				}
		UT_uint32 k;
		for (k=0; k<TableSize; k++)
		{
				if (fontMappingTable[k].abi[0] == '*')
						idx = k;
				else if (!UT_strnicmp(fontMappingTable[k].abi,name, 
									  strlen(fontMappingTable[k].abi)))
				{
						idx = k;
						break;
				}
		}

		// return the gnome mapping
		return fontMappingTable[idx].gnome;
}

#undef TableSize

static bool isItalic(XAP_UnixFont::style s)
{
        return ((s == XAP_UnixFont::STYLE_ITALIC) || 
				(s == XAP_UnixFont::STYLE_BOLD_ITALIC));
}

static GnomeFontWeight getGnomeFontWeight(XAP_UnixFont::style s)
{
        GnomeFontWeight w = GNOME_FONT_BOOK;
		switch((int)s)
				{
				case XAP_UnixFont::STYLE_BOLD_ITALIC:
				case XAP_UnixFont::STYLE_BOLD:
						w = GNOME_FONT_BOLD;
				default:
						break;
				}
		
		return w;
}

#define DEFAULT_GNOME_FONT "Helvetica"

static
gboolean fonts_match(GnomeFont *tmp, const gchar * intended)
{
		/* this is a hack - gnome_font_new_closest() returns Helvetica
		   when it gets confused */

		if(!g_strcasecmp(intended, DEFAULT_GNOME_FONT))
				return TRUE; // asked for and got helvetica
		
		const gchar * what = gnome_font_get_name(tmp);
		xxx_UT_DEBUGMSG(("Looking for %s returned %s \n",intended,what));
		if(!g_strcasecmp(what, DEFAULT_GNOME_FONT))
				return FALSE; // asked for something and got helvetica instead
		return TRUE;
}
#undef DEFAULT_GNOME_FONT

GnomeFont * XAP_UnixGnomePrintGraphics::_allocGnomeFont(PSFont* pFont)
{
        XAP_UnixFont *uf          = pFont->getUnixFont();
		XAP_UnixFont::style style = uf->getStyle();
		char *abi_name            = (char*)uf->getName();
		
		GnomeFontWeight gfw = getGnomeFontWeight(style);
		bool italic      = isItalic(style);
	
		// ok, this is the ugliest hack of the year, so I'll take it one step
		// at a time

		// add 0.0001 so 11.9999 gets rounded up to 12
		double size         = (double)pFont->getSize() * _scale_factor_get () + 0.0001;
	
		// test for oddness, if odd, subtract 1
		// why? abi allows odd point fonts for at least 
		// 9 and 11 points. gnome print does not, so we
		// scale it down. *ugly*
		if((int)size % 2 != 0)
				size -= 1.0;
		
		// first try to directly allocate abi's name
		// this is good for fonts not in my table, and
		// fonts installed by the user in both Abi and
		// using gnome-font-install
		
		GnomeFont *tmp = NULL;
		
		/* gnome_font_new_closest always returns a font. 
		   So you will get helvetica if not found */
		
		tmp      = gnome_font_new_closest(abi_name, gfw, italic, size);
		
		// assert that the fonts match
		if(tmp && fonts_match(tmp, abi_name))
				return tmp;
		
		xxx_UT_DEBUGMSG(("Dom: unreffing gnome font: ('%s','%s')\n", 
						 gnome_font_get_ps_name(tmp), abi_name));
		
		// else we got something we didn't ask for
		// unref the gnome-font and try again
		gnome_font_unref(tmp);
		
		char *fontname      = mapFontName(abi_name);
		tmp = gnome_font_new_closest(fontname, gfw, italic, size);
		
		return tmp;
}

XAP_UnixGnomePrintGraphics::~XAP_UnixGnomePrintGraphics()
{
		// unref the resource
		if(m_pCurrentFont != NULL && GNOME_IS_FONT(m_pCurrentFont))
				gnome_font_unref(m_pCurrentFont);
		delete m_eAdobeFull;
		delete m_localFont;
		if(m_eSymbol)
				delete m_eSymbol;
		if(m_eDingbats)
				delete m_eDingbats;
		m_encSymbol = NULL;
		m_encDingbats = NULL;
		m_encAdobeFull = NULL;
		m_eSymbol = NULL;
		m_eDingbats = NULL;
		m_eAdobeFull = NULL;
		m_localFont = NULL;
}

struct pageSizeMapping {
		char * abi;
		char * gnome;
};

static struct pageSizeMapping paperMap[] = {
		{"Letter", "US-Letter"},
		{"Legal",  "US-Legal"},
		{"Folio",  "Executive"}, // i think that this is correct
		{NULL,     NULL}
};

#define PaperTableSize	((sizeof(paperMap)/sizeof(paperMap[0])))

// This method maps Abi's paper sizes to their gnome-print
// equivalents if there is a mapping. If @sz is null,
// return the default name. If @sz is not in our map, return @sz
static const
char * mapPageSize (const char * sz)
{
		if (!sz && !*sz)
				return gnome_paper_name_default ();

		for (int i = 0; i < (int)PaperTableSize; i++)
				{
						if (paperMap[i].abi && !g_strcasecmp (sz, paperMap [i].abi))
								return paperMap[i].gnome;
				}

		return sz;
}

#undef PaperTableSize

XAP_UnixGnomePrintGraphics::XAP_UnixGnomePrintGraphics(GnomePrintMaster *gpm,
													   const char * pageSize,
						       XAP_UnixFontManager * fontManager,
						       XAP_App *pApp,
						       bool isPreview)
{
        m_pApp         = pApp;
	m_gpm          = gpm;
	m_gpc          = gnome_print_master_get_context(gpm);

	// TODO: be more robust about this
	const GnomePaper * paper = gnome_paper_with_name(mapPageSize (pageSize));

	xxx_UT_DEBUGMSG(("DOM: mapping '%s' returned '%s'\n", pageSize, mapPageSize (pageSize)));
	if (!paper)
			paper = gnome_paper_with_name (gnome_paper_name_default ());

	UT_ASSERT(paper);

	m_paper = paper;
	gnome_print_master_set_paper(m_gpm, paper);

	m_bIsPreview   = isPreview;
	m_fm           = fontManager;
	m_bStartPrint  = false;
	m_bStartPage   = false;
	m_pCurrentFont = NULL;
	m_pCurrentPSFont = NULL;
	m_bisSymbol = false;
	m_bisDingbats = false;
	m_encSymbol = NULL;
	m_encDingbats = NULL;
	m_encAdobeFull = NULL;
	m_eSymbol = NULL;
    m_eDingbats = NULL;
	m_eAdobeFull = NULL;
    m_localFont = NULL;
	m_cs = GR_Graphics::GR_COLORSPACE_COLOR;

	m_currentColor.m_red = 0;
	m_currentColor.m_grn = 0;
	m_currentColor.m_blu = 0;
//
// Now I need a local copy of the adobe full unicode to glyph tables
//
	m_localFont = new XAP_UnixFont();
	UT_String AdobeName(XAP_App::getApp()->getAbiSuiteLibDir());
	AdobeName += "/fonts/adobe-full.u2g";
	char * szName = const_cast<char *>(AdobeName.c_str());
	m_encAdobeFull = m_localFont->loadEncodingFile(szName);
	m_eAdobeFull = new UT_AdobeEncoding(m_encAdobeFull, m_localFont->getEncodingTableSize());
}

UT_uint32 XAP_UnixGnomePrintGraphics::measureUnRemappedChar(const UT_UCSChar c)
{
	int size = 0;
	
        UT_ASSERT(m_pCurrentFont);
	if (c >= 256)
		return 0;
	UT_UCSChar cc = c; 
//#if 1
//	unsigned char uc = c;
//	size = (int) ( _scale_factor_get_inverse () *
//		       gnome_font_get_width_string_n (m_pCurrentFont, (const char *)&uc, 1) );
//#else
//	size = (int) (_scale_factor_get_inverse () * gnome_font_get_glyph_width(m_pCurrentFont, (int)c));
//#endif
	
	size = m_pCurrentPSFont->getCharWidth(cc) * m_pCurrentPSFont->getSize() / 1000;
	return size;
}

void XAP_UnixGnomePrintGraphics::drawChars(const UT_UCSChar* pChars, 
										   int iCharOffset, int iLength,
										   UT_sint32 xoff, UT_sint32 yoff)
{
	UT_ASSERT(m_pCurrentFont);

	// The GR classes are expected to take yoff as the upper-left of
	// each glyph.  PostScript interprets the yoff as the baseline,
	// which doesn't match this expectation.  Adding the ascent of the
	// font will bring it back to the correct position.
	yoff += getFontAscent();

	// unsigned buffer holds Latin-1 data to character code 255
	guchar buf[OUR_LINE_LIMIT*6];
	guchar * pD;
	const UT_UCSChar * pS;
	const UT_UCSChar * pEnd;

	xxx_UT_DEBUGMSG(("DOM: drawChars (x: %d) (y: %d)\n", xoff, yoff));

	gnome_print_moveto(m_gpc, _scale_x_dir(xoff), _scale_y_dir(yoff));

	pEnd = pChars + iCharOffset + iLength;
	
	for (pS = pChars + iCharOffset; pS < pEnd; pS += OUR_LINE_LIMIT) {
			const UT_UCSChar * pB;
			UT_UCSChar currentChar;
   
			pD = buf;
			for (pB = pS; (pB < pS + OUR_LINE_LIMIT) && (pB < pEnd); pB++) {
					currentChar = remapGlyph(*pB, *pB >= 256 ? true : false);
					if(!m_bisSymbol && !m_bisDingbats)
					{
							currentChar = currentChar <= 0xff ? currentChar : XAP_EncodingManager::get_instance()->UToNative(currentChar);
					}
					else if(m_bisSymbol)
					{
//
// Convert to Symbol glyph then to unicode value..
//
							const char * glyph = m_eSymbol->ucsToAdobe(currentChar);
							UT_DEBUGMSG(("SEVIOR: Adobe glyph for current char is %s current char %x \n",glyph,currentChar));
							currentChar = m_eAdobeFull->adobeToUcs(glyph);
							UT_DEBUGMSG(("SEVIOR: After glyph mapping current char is %x \n",currentChar));
					}
					else if(m_bisDingbats)
					{
//
// Convert to Symbol glyph then to unicode value..
//
							const char * glyph = m_eDingbats->ucsToAdobe(currentChar);
							currentChar = m_eAdobeFull->adobeToUcs(glyph);
					}
					UT_DEBUGMSG(("SEVIOR: currentChar = %x \n",currentChar));
					pD += unichar_to_utf8 (currentChar, pD);
			}
			UT_DEBUGMSG(("SEVIOR: Calling show_sized \n"));
			gnome_print_show_sized (m_gpc, (gchar *) buf, pD - buf);
	}
}

void XAP_UnixGnomePrintGraphics::drawLine(UT_sint32 x1, UT_sint32 y1,
				  UT_sint32 x2, UT_sint32 y2)
{
        gnome_print_setlinewidth(m_gpc, m_dLineWidth); 
		gnome_print_moveto(m_gpc, _scale_x_dir(x1), _scale_y_dir(y1));
		gnome_print_lineto(m_gpc, _scale_x_dir(x2), _scale_y_dir(y2));
		gnome_print_stroke(m_gpc);

		/* Dom: you don't wanna close the path, you only need to
		   stroke it [ hhhuu hhuiuuu hhuu. Hey beavis, yes said "stroke it" ]*/
}

void XAP_UnixGnomePrintGraphics::setFont(GR_Font* pFont)
{
	UT_ASSERT(pFont);
	//
	// Need this psFont to get the scales correct
	//
	PSFont * psFont = (static_cast<PSFont*> (pFont));
	m_pCurrentPSFont = psFont;
	//
	// We have to see if we must do fancy encoding to unicode for gnomeprint
	//
	UT_String pszFname(psFont->getUnixFont()->getName());
	UT_sint32 i;
	for(i=0; pszFname[i] != 0; i++)
	{
			pszFname[i] = tolower(pszFname[i]);
	}
	UT_DEBUGMSG(("SEVIOR: Found  font %s \n",pszFname.c_str()));
	if(strstr(pszFname.c_str(),"symbol") != 0)
	{
			UT_DEBUGMSG(("SEVIOR: Found symbol font building unicode conversion stuff \n"));
			m_bisSymbol = true;
			if(!m_eSymbol)
			{
				m_encSymbol = psFont->getUnixFont()->loadEncodingFile();
				m_eSymbol = new UT_AdobeEncoding(m_encSymbol, psFont->getUnixFont()->getEncodingTableSize());
			}
	}
	else if (strstr(pszFname.c_str(),"dingbats") !=0 )
	{
			m_bisDingbats = true;
			if(!m_eDingbats)
			{
				m_encDingbats = psFont->getUnixFont()->loadEncodingFile();
				m_eDingbats = new UT_AdobeEncoding(m_encDingbats, psFont->getUnixFont()->getEncodingTableSize());
			}
	}
	else
	{
			m_bisSymbol = false;
			m_bisDingbats = false;
	}

	// TODO: We *must* be smarter about this, maybe a hash
	// TODO: of PSFonts -> GnomeFonts

	if(m_pCurrentFont && GNOME_IS_FONT(m_pCurrentFont))
			gnome_font_unref(m_pCurrentFont);

	m_pCurrentFont = _allocGnomeFont(psFont);

#if 0
	XAP_UnixFont *uf          = static_cast<PSFont*>(pFont)->getUnixFont();
	UT_DEBUGMSG(("Dom: setting font:\n"
				 "\tsize returned: %f (requested %f)\n"
				 "\tname returned: %s (requested %s)\n", 
				 gnome_font_get_size(m_pCurrentFont),
				 (double)static_cast<PSFont*>(pFont)->getSize() * _scale_factor_get(), 
				 gnome_font_get_name(m_pCurrentFont), uf->getName()));
#endif

	gnome_print_setfont (m_gpc, m_pCurrentFont);
}

bool XAP_UnixGnomePrintGraphics::queryProperties(GR_Graphics::Properties gp) const
{
	switch (gp)
	{
	case DGP_SCREEN:
		return false;
	case DGP_PAPER:
		return true;
	default:
		UT_ASSERT(0);
		return false;
	}
}

void XAP_UnixGnomePrintGraphics::setColor(UT_RGBColor& clr)
{
	if (clr.m_red == m_currentColor.m_red &&
	    clr.m_grn == m_currentColor.m_grn &&
	    clr.m_blu == m_currentColor.m_blu)
		return;
	
	// NOTE : we always set our color to something RGB, even if the color
	// NOTE : space is b&w or grayscale
	m_currentColor.m_red = clr.m_red;
	m_currentColor.m_grn = clr.m_grn;
	m_currentColor.m_blu = clr.m_blu;

	xxx_UT_DEBUGMSG(("Dom: setColor\n"));

	gnome_print_setrgbcolor(m_gpc,
							(m_currentColor.m_red / 255.0),
							(m_currentColor.m_grn / 255.0),
							(m_currentColor.m_blu / 255.0));
	
}

void XAP_UnixGnomePrintGraphics::setLineWidth(UT_sint32 iLineWidth)
{
 	m_dLineWidth = (double)((double)iLineWidth * _scale_factor_get());
}

bool XAP_UnixGnomePrintGraphics::startPrint(void)
{
        UT_ASSERT(!m_bStartPrint);
	m_bStartPrint = true;
	return _startDocument();
}

bool XAP_UnixGnomePrintGraphics::startPage(const char * szPageLabel)
{
	if (m_bStartPage)
	  _endPage();
	m_bStartPage = true;
	m_bNeedStroked = false;
	return _startPage(szPageLabel);
}

bool XAP_UnixGnomePrintGraphics::startPage (const char *szPageLabel, 
											UT_uint32, bool,
											UT_uint32, UT_uint32)
{
        return startPage(szPageLabel);
}

bool XAP_UnixGnomePrintGraphics::endPrint()
{
	if (m_bStartPage)
	  _endPage();
	return _endDocument();
}

void XAP_UnixGnomePrintGraphics::setColorSpace(GR_Graphics::ColorSpace c)
{
	m_cs = c;
}

GR_Graphics::ColorSpace XAP_UnixGnomePrintGraphics::getColorSpace(void) const
{
	return m_cs;
}


void XAP_UnixGnomePrintGraphics::drawAnyImage (GR_Image* pImg, 
											   UT_sint32 xDest, 
											   UT_sint32 yDest, bool rgb)
{
	UT_sint32 iDestWidth  = pImg->getDisplayWidth();
	UT_sint32 iDestHeight = pImg->getDisplayHeight();

	GR_UnixGnomeImage * pImage = static_cast<GR_UnixGnomeImage *>(pImg);
	GdkPixbuf * image = pImage->getData();
	UT_ASSERT(image);

	gnome_print_gsave(m_gpc);
	gnome_print_translate(m_gpc,
						  _scale_x_dir(xDest),
						  _scale_y_dir(yDest + iDestHeight));
	gnome_print_scale(m_gpc,
			  ((double) iDestWidth)  * _scale_factor_get (),
			  ((double) iDestHeight) * _scale_factor_get ());

	gnome_print_pixbuf (m_gpc, image);
	
	gnome_print_grestore(m_gpc);
}

void XAP_UnixGnomePrintGraphics::drawImage(GR_Image* pImg, UT_sint32 xDest, 
										   UT_sint32 yDest)
{
   	if (pImg->getType() != GR_Image::GRT_Raster) {
	   pImg->render(this, xDest, yDest);
	   return;
	}

   	switch(m_cs)
     	{
       	case GR_Graphics::GR_COLORSPACE_COLOR:
		drawAnyImage(pImg, xDest, yDest, true);
		break;
      	case GR_Graphics::GR_COLORSPACE_GRAYSCALE:
		drawAnyImage(pImg, xDest, yDest, false);
		break;
      	case GR_Graphics::GR_COLORSPACE_BW:
		drawAnyImage(pImg, xDest, yDest, false);
		break;
      	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
     }
}

GR_Image* XAP_UnixGnomePrintGraphics::createNewImage(const char* pszName, 
					     const UT_ByteBuf* pBB, 
					     UT_sint32 iDisplayWidth,
					     UT_sint32 iDisplayHeight, 
					     GR_Image::GRType iType)
{
	GR_Image* pImg = NULL;

   	if (iType == GR_Image::GRT_Raster)
     		pImg = new GR_UnixGnomeImage(pszName);
   	else if (iType == GR_Image::GRT_Vector)
     		pImg = new GR_VectorImage(pszName);
   
	pImg->convertFromBuffer(pBB, iDisplayWidth, iDisplayHeight);

	return pImg;
}

/***********************************************************************/
/*                          Protected things                           */
/***********************************************************************/
bool	XAP_UnixGnomePrintGraphics::_startDocument(void)
{
        return true;
}

bool XAP_UnixGnomePrintGraphics::_startPage(const char * szPageLabel)
{
        gnome_print_beginpage(m_gpc, szPageLabel);
		_setup_rotation ();
		return true;
}

bool XAP_UnixGnomePrintGraphics::_endPage(void)
{
	if(m_bNeedStroked)
	  gnome_print_stroke(m_gpc);

	gnome_print_showpage(m_gpc);
	return true;
}

bool XAP_UnixGnomePrintGraphics::_endDocument(void)
{
		// bonobo version, we'd don't own the context
		// or the master, just return
	if(!m_gpm)
	  return true;

        gnome_print_master_close(m_gpm);

	if(!m_bIsPreview)
	  {
	    gnome_print_master_print(m_gpm);
	  }
	else
	  {
	    GnomePrintMasterPreview *preview;
	    const XAP_StringSet * pSS = m_pApp->getStringSet();

	    preview = gnome_print_master_preview_new_with_orientation (m_gpm, 
																   pSS->getValue(XAP_STRING_ID_DLG_UP_PrintPreviewTitle), 
																   !isPortrait());
	    gtk_widget_show(GTK_WIDGET(preview));
	  }
	
	gtk_object_unref(GTK_OBJECT(m_gpm));
	return true;
}

UT_uint32 XAP_UnixGnomePrintGraphics::_getResolution(void) const
{
        return GPG_RESOLUTION;
}

void XAP_UnixGnomePrintGraphics::fillRect(UT_RGBColor& c, UT_sint32 x, 
										  UT_sint32 y, UT_sint32 w, 
										  UT_sint32 h)
{
		// draw background color
		gnome_print_setrgbcolor(m_gpc,
								(int)(c.m_red / 255),
								(int)(c.m_grn / 255),
								(int)(c.m_blu / 255));

#if 0
		// adjust for the text's height
		y += getFontDescent () + getFontHeight();
		
		/* Mirror gdk which excludes the far point */

		w -= (int)_scale_x_dir (1);
		h -= (int)_scale_y_dir (1);
#endif

		gnome_print_newpath (m_gpc);
		gnome_print_moveto (m_gpc, _scale_x_dir(x),   _scale_y_dir(y));		
		gnome_print_lineto (m_gpc, _scale_x_dir(x+w), _scale_y_dir(y));
		gnome_print_lineto (m_gpc, _scale_x_dir(x+w), _scale_y_dir(y+h));
		gnome_print_lineto (m_gpc, _scale_x_dir(x),   _scale_y_dir(y+h));
		gnome_print_lineto (m_gpc, _scale_x_dir(x),   _scale_y_dir(y));
		gnome_print_closepath (m_gpc);
		gnome_print_fill (m_gpc);

		// reset color to its original state
		gnome_print_setrgbcolor(m_gpc,
								(int)(m_currentColor.m_red / 255),
								(int)(m_currentColor.m_grn / 255),
								(int)(m_currentColor.m_blu / 255));
}

void XAP_UnixGnomePrintGraphics::fillRect(UT_RGBColor& c, UT_Rect & r)
{
		fillRect(c, r.left, r.top, r.width, r.height);
}

void XAP_UnixGnomePrintGraphics::setClipRect(const UT_Rect* pRect)
{
		// TODO: gnome_print_clip()
		// useful for clipping images and other objects
}

/***********************************************************************/
/*                    Things that souldn't happen                      */
/***********************************************************************/
void XAP_UnixGnomePrintGraphics::setCursor(GR_Graphics::Cursor)
{
        // nada
}

GR_Graphics::Cursor XAP_UnixGnomePrintGraphics::getCursor(void) const
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return GR_CURSOR_INVALID;
}

void XAP_UnixGnomePrintGraphics::xorLine(UT_sint32, UT_sint32, UT_sint32, 
										 UT_sint32)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void XAP_UnixGnomePrintGraphics::polyLine(UT_Point * /* pts */, 
										  UT_uint32 /* nPoints */)
{
        // only used by us for printing red squiggly lines
        // in the spell-checker
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void XAP_UnixGnomePrintGraphics::invertRect(const UT_Rect* /*pRect*/)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void XAP_UnixGnomePrintGraphics::clearArea(UT_sint32 /*x*/, UT_sint32 /*y*/,
										   UT_sint32 /*width*/, UT_sint32 /*height*/)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void XAP_UnixGnomePrintGraphics::scroll(UT_sint32, UT_sint32)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void XAP_UnixGnomePrintGraphics::scroll(UT_sint32 /* x_dest */,
										UT_sint32 /* y_dest */,
										UT_sint32 /* x_src */,
										UT_sint32 /* y_src */,
										UT_sint32 /* width */,
										UT_sint32 /* height */)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

UT_RGBColor * XAP_UnixGnomePrintGraphics::getColor3D(GR_Color3D /*c*/)
{
        UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return NULL;
}

void XAP_UnixGnomePrintGraphics::setColor3D(GR_Color3D /*c*/)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

GR_Font* XAP_UnixGnomePrintGraphics::getGUIFont()
{
        UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		return NULL;
}

void XAP_UnixGnomePrintGraphics::fillRect(GR_Color3D c, UT_sint32 x, UT_sint32 y, UT_sint32 w, UT_sint32 h)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

void XAP_UnixGnomePrintGraphics::fillRect(GR_Color3D c, UT_Rect &r)
{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}


/***********************************************************************/
/*                                 Done                                */
/***********************************************************************/

//
// This code will return the identical size to that used for the layout
//
UT_uint32 XAP_UnixGnomePrintGraphics::getFontAscent(GR_Font *fnt)
{
	UT_uint32 asc;

#if 0
	GnomeFont * gfnt = _allocGnomeFont(static_cast<PSFont*>(fnt));
	const GnomeFontFace *face;
	const ArtDRect *bbox;

	face = gnome_font_get_face (gfnt);
	bbox = 	gnome_font_face_get_stdbbox (face);
	asc = (gint) (bbox->y1 * gnome_font_get_size (gfnt) / 10);
	gnome_font_unref (gfnt);
#endif

  PSFont * psfnt = static_cast<PSFont *>(fnt);

  XAP_UnixFont * pUFont = psfnt->getUnixFont();
  UT_sint32 iSize = psfnt->getSize();
  XAP_UnixFontHandle * pHndl = new XAP_UnixFontHandle(pUFont, iSize);
  GdkFont* pFont = pHndl->getGdkFont();
  GdkFont* pMatchFont= pHndl->getMatchGdkFont();
  asc = MAX(pFont->ascent, pMatchFont->ascent);
  delete pHndl;
	return asc;
}

/* This function does not expect in return the font ascent,
   it expects the font bbox.ury. Chema */
UT_uint32 XAP_UnixGnomePrintGraphics::getFontAscent()
{
	UT_uint32 asc;
#if 0
	const GnomeFontFace *face;
	const ArtDRect *bbox;
	GnomeFont *font;

	font = m_pCurrentFont;
	face = gnome_font_get_face (font);
	bbox = 	gnome_font_face_get_stdbbox (face);
	asc = (gint) (bbox->y1 * gnome_font_get_size (font) / 10);
#endif

	asc = getFontAscent(static_cast<GR_Font *>(m_pCurrentPSFont));
	return asc;
}

UT_uint32 XAP_UnixGnomePrintGraphics::getFontDescent(GR_Font *fnt)
{
	UT_uint32 des;

#if 0
	GnomeFont * gfnt = _allocGnomeFont(static_cast<PSFont*>(fnt));
	const GnomeFontFace *face;
    const ArtDRect *bbox;

	face = gnome_font_get_face (gfnt);
	bbox = 	gnome_font_face_get_stdbbox (face);

	des = (gint) (bbox->y0 * gnome_font_get_size (gfnt) / 10);
	des *= -1;
	gnome_font_unref (gfnt);
#endif

  PSFont * psfnt = static_cast<PSFont *>(fnt);

  XAP_UnixFont * pUFont = psfnt->getUnixFont();
  UT_sint32 iSize = psfnt->getSize();
  XAP_UnixFontHandle * pHndl = new XAP_UnixFontHandle(pUFont, iSize);
  GdkFont* pFont = pHndl->getGdkFont();
  GdkFont* pMatchFont= pHndl->getMatchGdkFont();
  des = MAX(pFont->descent, pMatchFont->descent);
  delete pHndl;

	return des;
}

UT_uint32 XAP_UnixGnomePrintGraphics::getFontDescent()
{
	UT_uint32 des;
#if 0
	const GnomeFontFace *face;
	const ArtDRect *bbox;
	GnomeFont *font;

	font = m_pCurrentFont;
		
        UT_ASSERT(GNOME_IS_FONT (font));
	face = gnome_font_get_face (font);
        UT_ASSERT(GNOME_IS_FONT_FACE (face));
	bbox = 	gnome_font_face_get_stdbbox (face);

	des = (gint) (bbox->y0 * gnome_font_get_size (font) / 10);
	des *= -1;
#endif

	des = getFontDescent(static_cast<GR_Font *>(m_pCurrentPSFont));
	return des;
}

UT_uint32 XAP_UnixGnomePrintGraphics::getFontHeight()
{
	UT_ASSERT(m_pCurrentFont);
	UT_sint32 height = getFontAscent() + getFontDescent();
	return height;
}

UT_uint32 XAP_UnixGnomePrintGraphics::getFontHeight(GR_Font *fnt)
{
	UT_sint32 height = getFontAscent(fnt) + getFontDescent(fnt);
#if 0
	UT_DEBUGMSG(("Font height in gnome-print = %d \n",height));
	PSFont * pFont = static_cast<PSFont *>(fnt);
	XAP_UnixFont *uf          = pFont->getUnixFont();
	char *abi_name            = (char*)uf->getName();
	UT_DEBUGMSG(("Font size in Gnome-print = %d Font Name %s \n",pFont->getSize(),abi_name));
#endif
	return height;
}

GR_Font* XAP_UnixGnomePrintGraphics::findFont(const char* pszFontFamily, 
											  const char* pszFontStyle, 
											  const char* /* pszFontVariant */,
											  const char* pszFontWeight, 
											  const char* /* pszFontStretch */,
											  const char* pszFontSize)
{
	UT_ASSERT(pszFontFamily);
	UT_ASSERT(pszFontStyle);
	UT_ASSERT(pszFontWeight);
	UT_ASSERT(pszFontSize);
	
	// convert styles to XAP_UnixFont:: formats
	XAP_UnixFont::style s = XAP_UnixFont::STYLE_NORMAL;

	// this is kind of sloppy
	if (!UT_strcmp(pszFontStyle, "normal") &&
		!UT_strcmp(pszFontWeight, "normal"))
	{
		s = XAP_UnixFont::STYLE_NORMAL;
	}
	else if (!UT_strcmp(pszFontStyle, "normal") &&
			 !UT_strcmp(pszFontWeight, "bold"))
	{
		s = XAP_UnixFont::STYLE_BOLD;
	}
	else if (!UT_strcmp(pszFontStyle, "italic") &&
			 !UT_strcmp(pszFontWeight, "normal"))
	{
		s = XAP_UnixFont::STYLE_ITALIC;
	}
	else if (!UT_strcmp(pszFontStyle, "italic") &&
			 !UT_strcmp(pszFontWeight, "bold"))
	{
		s = XAP_UnixFont::STYLE_BOLD_ITALIC;
	}
	else
	{
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
	}

	// Request the appropriate XAP_UnixFont::, and bury it in an
	// instance of a UnixFont:: with the correct size.
	XAP_UnixFont * unixfont = m_fm->getFont(pszFontFamily, s);
	XAP_UnixFont * item = NULL;
	if (unixfont)
	{
		// make a copy
		item = new XAP_UnixFont(*unixfont);
	}
	else
	{
		// Oops!  We don't have that font here.  substitute something
		// we know we have (get smarter about this later)
		g_print ("Get times new roman !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
				 "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		item = new XAP_UnixFont(*m_fm->getFont("Times New Roman", s));
	}
	
//
// This piece of code scales the FONT chosen at low resolution to that at high
// resolution. This fixes bug 1632 and other non-WYSIWYG behaviour.
//
	double dScreenSize = UT_convertToInches(pszFontSize) * (double) getScreenResolution();
	UT_sint32 iScreenSize = (UT_sint32) (dScreenSize + 0.5);
	dScreenSize = (double) iScreenSize;

	double ratToLayout = (double) UT_LAYOUT_UNITS / (double) getScreenResolution();
	UT_sint32 iSizeLayout = (UT_sint32) (dScreenSize * ratToLayout + 0.5);

	double dPaperSize = dScreenSize * (double) getResolution() / (double) getScreenResolution();
	UT_sint32 iSize = (UT_sint32) (dPaperSize + 0.5);
    
	
	if( m_bLayoutResolutionModeEnabled)
	{
		iSize = iSizeLayout;
	} 

	xxx_UT_DEBUGMSG(("Using Gnome-Print PS Font Size %d \n",iSize));
	PSFont * pFont = new PSFont(item, iSize);

	UT_ASSERT(pFont);

	return pFont;
}

/***********************************************************************/
/*                Private Scaling Conversion Routines                  */
/***********************************************************************/

void XAP_UnixGnomePrintGraphics::_setup_rotation (void)
{
	double affine [6];

	// do nothing for this default case
	if (isPortrait ())
		return;

	// we have to apply an affine to the print context for
	// each page in order to print in landscape mode
	art_affine_rotate (affine, 90.0);
	gnome_print_concat (m_gpc, affine);

	art_affine_translate (affine, 0, - _get_height ());
	gnome_print_concat (m_gpc, affine);
}

double XAP_UnixGnomePrintGraphics::_scale_factor_get (void)
{
	return ((double) 72.0 / (double) GPG_RESOLUTION);
}

double XAP_UnixGnomePrintGraphics::_scale_factor_get_inverse (void)
{
	return ((double) GPG_RESOLUTION / (double) 72.0);
}

double XAP_UnixGnomePrintGraphics::_scale_x_dir (int x)
{
        double d = 0.0;
	
	d  =  (double) x;
	d *= _scale_factor_get ();
	
	return d;
}

double XAP_UnixGnomePrintGraphics::_get_height (void)
{
	if (m_paper)
			{
					if (isPortrait ())
							return gnome_paper_psheight (m_paper);
					else
							return gnome_paper_pswidth (m_paper);
			}
	else
			{
					/* FIXME: Hardcode US-Letter values as a standby/fallback for now */
					if (isPortrait ())
							return 11.0;
					else
							return 8.5;
			}
}

double XAP_UnixGnomePrintGraphics::_scale_y_dir (int y)
{
	double d = 0.0;
	double page_length = 0.0;
	
	page_length = _get_height ();

	/* This one is obscure:
	 * Our drawChars and drawLine functions recieve yDests relative to the
	 * page that they're on. drawImg doesn't. We need to properly
	 * correct for that
	 */

#if 0	
	d  =  page_length - (double) y;
	d *= _scale_factor_get();
#else
	d = page_length - (double)((int)(y * _scale_factor_get()) % (int)page_length);
#endif

	return d;
}
