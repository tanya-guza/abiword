/* AbiWord
 * Copyright (C) 1998-2000 AbiSource, Inc.
 * Copyright (C) 2002 Tomas Frydrych <tomas@frydrych.uklinux.net>
 *
 * This program is g_free software; you can redistribute it and/or
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

#ifndef AP_QNXDIALOG_MARKREVISIONS_H
#define AP_QNXDIALOG_MARKREVISIONS_H

#include "ap_Dialog_MarkRevisions.h"
#include <Pt.h>

class XAP_QNXFrame;

/*****************************************************************/

class AP_QNXDialog_MarkRevisions: public AP_Dialog_MarkRevisions
{
public:
	AP_QNXDialog_MarkRevisions(XAP_DialogFactory * pDlgFactory, XAP_Dialog_Id id);
	virtual ~AP_QNXDialog_MarkRevisions(void);

	virtual void			runModal(XAP_Frame * pFrame);

	static XAP_Dialog *		static_constructor(XAP_DialogFactory *, XAP_Dialog_Id id);
	int done;
	void event_OK();
	void event_Cancel();
	void event_toggle(PtWidget_t *widget);	
protected:

      PtWidget_t * _constructWindow();
      PtWidget_t *m_text1;
      PtWidget_t *m_toggle1;
      PtWidget_t *m_toggle2;

};

#endif /* AP_QNXDIALOG_MARKREVISIONS_H */
