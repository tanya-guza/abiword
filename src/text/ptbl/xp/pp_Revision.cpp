/* AbiWord
 * Copyright (C) 2002 Tomas Frydrych <tomas@frydrych.uklinux.net>
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

#include "pp_Revision.h"
#include <limits.h>

/*! create class instance from an XML attribute string
 */
pp_Revision::pp_Revision(const XML_Char * r)
{
	_setVector(r);
}

/*! initialize instance with XML attribute string
 */
void pp_Revision::setRevision(const XML_Char * r)
{
	m_vRev.clear();
	_setVector(r);
}

/*! parse given XML attribute string and fill the
    instance with the data
*/
void pp_Revision::_setVector(const XML_Char *r)
{
	char * s = (char*) UT_XML_cloneString(s,r);
	char * t = strtok(s,";");

	while(t)
	{
		UT_sint32 i = atol(t);
		m_vRev.addItem((void*)i);
		t = strtok(NULL,";");
	}

	FREEP(s);
	m_bDirty = true;
}


/*! return highest ID associated with this revision attribute that
    is at most equal to id; the returned ID can be used to determine
    whether this text should be displayed or hidden in revision with
    the original id, i.e., if the attribute is "+1;-3;+5" and we are
    in revision 4, getGreatestLesserOrEqualRevision(4) will return
    -3, i.e., this text should be hidden in revision 4.

    \param UT_uint32 id : the id of this revision (must be a positive value!!!)
    \return : id with sign indicating whether this is addition or
    deletion; two special cases are 0: addition, INT_MIN deletion
*/
UT_sint32 pp_Revision::getGreatestLesserOrEqualRevision(UT_uint32 id) const
{
	UT_sint32 r = 0; // this will be the revision id we are looking for
	UT_sint32 m = 0xFFFF; // this will be the smallest revision id present

	for(UT_uint32 i = 0; i < m_vRev.getItemCount(); i++)
	{
		UT_sint32 t = (UT_sint32) m_vRev.getNthItem(i);
		UT_uint32 t_abs = (UT_uint32) abs(t);

		// the special case speedup - if we hit our id, then we can return immediately
		if(t_abs == id)
			return t;

		if(t_abs < (UT_uint32)abs(m))
			m = t;

		if((t_abs < id) && (t_abs > (UT_uint32)abs(r)))
			r = t;
	}

	// now we have the biggest ID lesser or equal id,
	// we have to deal with the special case when this is 0
	// i.e., this fragment only figures in revisions > id
	// the problem with 0 is that it is an addition if the smallest
	// revision ID is negative, and deletion in the opposite case
	// -- if this is to be visible we return 0, it is to be hidden
	// we will return big negative number

	if(r == 0)
	{
		if(m < 0)
			return 0;
		else
			return INT_MIN;
	}
	else
		return r;
}

/*! given revision level id, this function returns true if given
    segment of text is to be visible, false if it is to be hidden
*/
bool pp_Revision::isVisible(UT_uint32 id) const
{
	return (getGreatestLesserOrEqualRevision(id) >= 0);
}

/*! adds id to the revision vector, ignoring identical ids, but
    changing sign when needed
*/
void pp_Revision::addRevisionId(UT_sint32 id)
{
	for(UT_uint32 i = 0; i < m_vRev.getItemCount(); i++)
	{
		UT_sint32 r = (UT_sint32)m_vRev.getNthItem(i);

		if(id == -r)
		{
			// we are trying to add revision id already in the vector
			// but with a different sign -- this is probably legal,
			// i.e., the editor just changed his mind
			m_vRev.deleteNthItem(i);
			m_vRev.addItem((void*)id);
			m_bDirty = true;
			return;
		}

		if(id == r)
		{
			// we are trying to add an id already in the vector
			// do nothing
			return;
		}
	}

	// if we got here then the item is not in our vector so add it
	m_vRev.addItem((void*)id);
	m_bDirty = true;
}

/*! removes id from this revision
 */
void pp_Revision::removeRevisionId(UT_sint32 id)
{
	for(UT_uint32 i = 0; i < m_vRev.getItemCount(); i++)
	{
		UT_sint32 r = (UT_sint32)m_vRev.getNthItem(i);

		if(id == r)
		{
			m_vRev.deleteNthItem(i);
			m_bDirty = true;
			return;
		}
	}
}

/*! create XML string from our vector
 */
void pp_Revision::_refreshString()
{
	char buf[30];
	m_sXMLstring.clear();

	for(UT_uint32 i = 0; i < m_vRev.getItemCount(); i++)
	{
		sprintf(buf,"%d;",(UT_sint32)m_vRev.getNthItem(i));
		m_sXMLstring += buf;
	}
	m_bDirty = false;
}



/*! get and XML_Char string representation of this revision
 */
const XML_Char * pp_Revision::getXMLstring()
{
	if(m_bDirty)
		_refreshString();

	return (const XML_Char*) m_sXMLstring.c_str();
}
