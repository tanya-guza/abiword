/* AbiWord
 * Copyright (C) 2001 AbiSource, Inc.
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

#ifndef XAP_DIALOG_IMAGE_H
#define XAP_DIALOG_IMAGE_H

#include "xap_Frame.h"
#include "xap_Dialog.h"
#include "xav_View.h"

#include "ut_types.h"

class XAP_Frame;

class XAP_Dialog_Image : public XAP_Dialog_NonPersistent
{
public:
  
  typedef enum { a_OK, a_Cancel } tAnswer;

	XAP_Dialog_Image(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id id);
	virtual ~XAP_Dialog_Image(void);

	virtual void  runModal(XAP_Frame * pFrame) = 0;

	double getWidth () const { return m_width; }
	double getHeight () const { return m_height; }

	// needed to set the initial width & height
	void setWidth (double w) { m_width = w; }
	void setHeight (double h) { m_height = h; }


	// needed to set the initial width & height
	void setWidth (UT_sint32 w);
	void setHeight (UT_sint32 h);

	void setMaxHeight ( double h ) { m_maxWidth = h; }
	void setMaxWidth ( double w ) { m_maxHeight = w; }

	double getMaxWidth () const { return m_maxWidth; }
	double getMaxHeight () const { return m_maxHeight; }

	tAnswer getAnswer () const { return m_answer; }
	const char * getHeightString(void);
	const char * getWidthString(void);
	double getIncrement(const char * sz);
	void incrementHeight(bool bIncrement);
	void incrementWidth(bool bIncrement);
	void setHeight(const char * szHeight);
	void setWidth(const char * szWidth);
	UT_Dimension getPreferedUnits(void);
	void  setPreferedUnits(UT_Dimension dim);

 protected:	  
	void setAnswer ( tAnswer ans ) { m_answer = ans; }
	void _convertToPreferredUnits(const char *sz, UT_String & pRet);

 private:
	double m_width;
	double m_height;
	double m_maxWidth;
	double m_maxHeight;
	tAnswer m_answer;
	UT_String m_HeightString;
	UT_String m_WidthString;
	bool m_bHeightChanged;
	bool m_bWidthChanged;
	UT_Dimension m_PreferedUnits;
};

#endif /* XAP_DIALOG_IMAGE_H */
