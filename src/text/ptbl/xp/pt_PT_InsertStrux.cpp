/* AbiWord
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


// insertStrux-related functions for class pt_PieceTable

#include "ut_types.h"
#include "ut_misc.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_growbuf.h"
#include "pt_PieceTable.h"
#include "pf_Frag.h"
#include "pf_Frag_Object.h"
#include "pf_Frag_Strux.h"
#include "pf_Frag_Strux_Block.h"
#include "pf_Frag_Strux_Section.h"
#include "pf_Frag_Text.h"
#include "pf_Fragments.h"
#include "px_ChangeRecord.h"
#include "px_ChangeRecord_Strux.h"

/****************************************************************/
/****************************************************************/

UT_Bool pt_PieceTable::_createStrux(PTStruxType pts,
									PT_AttrPropIndex indexAP,
									pf_Frag_Strux ** ppfs)
{
	// create a strux frag for this.
	// return *pfs and true if successful.

	// create an unlinked strux fragment.
	
	pf_Frag_Strux * pfs = NULL;
	switch (pts)
	{
	case PTX_Section:
		pfs = new pf_Frag_Strux_Section(this,indexAP);
		break;
		
	case PTX_Block:
		pfs = new pf_Frag_Strux_Block(this,indexAP);
		break;

	default:
		UT_ASSERT(0);
		break;
	}

	if (!pfs)
	{
		UT_DEBUGMSG(("Could not create structure fragment.\n"));
		// we forget about the AP that we created
		return UT_FALSE;
	}

	*ppfs = pfs;
	return UT_TRUE;
}

void pt_PieceTable::_insertStrux(pf_Frag * pf,
								 PT_BlockOffset fragOffset,
								 pf_Frag_Strux * pfsNew)
{
	// insert the new strux frag at (pf,fragOffset)

	switch (pf->getType())
	{
	default:
		UT_ASSERT(0);
		return;

	case pf_Frag::PFT_Object:
	case pf_Frag::PFT_EndOfDoc:
	case pf_Frag::PFT_Strux:
		{
			// insert pfsNew before pf.
			// TODO this may introduce some oddities due to empty paragraphs.
			// TODO investigate this later.
			UT_ASSERT(fragOffset == 0);
			m_fragments.insertFrag(pf->getPrev(),pfsNew);
			return;
		}
		
	case pf_Frag::PFT_Text:
		{
			// insert pfsNew somewhere inside pf.
			// we have a text fragment which we must deal with.
			// if we are in the middle of it, we split it.
			// if we are at one of the ends of it, we just insert
			// the fragment.

			// TODO if we are at one of the ends of the fragment,
			// TODO should we create a zero-length fragment in one
			// TODO of the paragraphs so that text typed will have
			// TODO the right attributes.

			pf_Frag_Text * pft = static_cast<pf_Frag_Text *> (pf);
			UT_uint32 fragLen = pft->getLength();
			if (fragOffset == fragLen)
			{
				// we are at the right end of the fragment.
				// insert the strux after the text fragment.

				m_fragments.insertFrag(pft,pfsNew);

				// TODO decide if we should create a zero-length
				// TODO fragment in the new paragraph to retain
				// TODO the attr/prop of the pft.
				// TODO         pf_Frag_Text * pftNew = new...
				// TODO         m_fragments.insertFrag(pfsNew,pftNew);
			}
			else if (fragOffset == 0)
			{
				// we are at the left end of the fragment.
				// insert the strux before the text fragment.

				m_fragments.insertFrag(pft->getPrev(),pfsNew);
			}
			else
			{
				// we are in the middle of a text fragment.  split it
				// and insert the new strux in between the pieces.

				UT_uint32 lenTail = pft->getLength() - fragOffset;
				PT_BufIndex biTail = m_varset.getBufIndex(pft->getBufIndex(),fragOffset);
				pf_Frag_Text * pftTail = new pf_Frag_Text(this,biTail,lenTail,pft->getIndexAP());
				UT_ASSERT(pftTail);
			
				pft->changeLength(fragOffset);
				m_fragments.insertFrag(pft,pfsNew);
				m_fragments.insertFrag(pfsNew,pftTail);
			}

			return;
		}
	}
}


UT_Bool pt_PieceTable::insertStrux(PT_DocPosition dpos,
								   PTStruxType pts)
{
	// insert a new structure fragment at the given document position.
	// this function can only be called while editing the document.

	UT_ASSERT(m_pts==PTS_Editing);

	// get the fragment at the doc postion containing the given
	// document position.

	pf_Frag * pf = NULL;
	PT_BlockOffset fragOffset = 0;
	UT_Bool bFoundFrag = getFragFromPosition(dpos,&pf,&fragOffset);
	UT_ASSERT(bFoundFrag);

	// get the strux containing the given position.
	
	pf_Frag_Strux * pfsContainer = NULL;
	UT_Bool bFoundContainer = _getStruxFromPosition(dpos,&pfsContainer);
	UT_ASSERT(bFoundContainer);

	// if we are inserting something similar to the previous strux,
	// we will clone the attributes/properties; we assume that the
	// new strux should have the same AP as the one which preceeds us.
	// This is generally true for inserting a paragraph -- it should
	// inherit the style of the one we just broke.

	PT_AttrPropIndex indexAP = 0;
	if (pfsContainer->getStruxType() == pts)
		indexAP = pfsContainer->getIndexAP();
	
	pf_Frag_Strux * pfsNew = NULL;
	if (!_createStrux(pts,indexAP,&pfsNew))
		return UT_FALSE;
	
	// insert this frag into the fragment list.

	_insertStrux(pf,fragOffset,pfsNew);

	PT_AttrPropIndex preferredSpanFmt = 0;
	if (pfsNew->getStruxType() == PTX_Block)
	{
		// when inserting paragraphs, we try to remember the current
		// span formatting active at the insertion point.  if the
		// user keeps typing text (or comes back later and starts
		// inserting text at the beginning of the paragraph), we'll
		// try to use it rather than defaulting to plain text.

		pf_Frag_Strux_Block * pfsbNew = static_cast<pf_Frag_Strux_Block *>(pfsNew);
		_captureActiveSpan(pfsbNew);
		preferredSpanFmt = pfsbNew->getPreferredSpanFmt();
	}

	// create a change record to describe the change, add
	// it to the history, and let our listeners know about it.
	
	PX_ChangeRecord_Strux * pcrs
		= new PX_ChangeRecord_Strux(PX_ChangeRecord::PXT_InsertStrux,
									dpos,indexAP,pts,preferredSpanFmt);
	UT_ASSERT(pcrs);

	// add record to history.  we do not attempt to coalesce these.
	m_history.addChangeRecord(pcrs);
	m_pDocument->notifyListeners(pfsContainer,pfsNew,pcrs);

	return UT_TRUE;
}

void pt_PieceTable::_captureActiveSpan(pf_Frag_Strux_Block * pfsBlock)
{
	// set the preferredSpanFmt field in pfsBlock based upon the
	// current state in the document.
	
	PT_AttrPropIndex api;
	if (_haveTempSpanFmt(NULL,&api))
	{
		pfsBlock->setPreferredSpanFmt(api);
		return;
	}

	pf_Frag * pfPrev = pfsBlock->getPrev();
	if (!pfPrev)
		return;

	switch (pfPrev->getType())
	{
	default:
		return;

	case pf_Frag::PFT_Text:
		{
			pf_Frag_Text * pfPrevText = static_cast<pf_Frag_Text *>(pfPrev);
			pfsBlock->setPreferredSpanFmt(pfPrevText->getIndexAP());
		}
		return;

	case pf_Frag::PFT_Object:
		{
			// TODO this might not be the right thing to do.  Referencing
			// TODO the span-level formatting for a Field is probably OK,
			// TODO but referencing the span-level formatting of an Image
			// TODO is probably bogus.
			pf_Frag_Object * pfPrevObject = static_cast<pf_Frag_Object *>(pfPrev);
			pfsBlock->setPreferredSpanFmt(pfPrevObject->getIndexAP());
		}
		return;

	case pf_Frag::PFT_Strux:
		{	
			pf_Frag_Strux * pfPrevStrux = static_cast<pf_Frag_Strux *>(pfPrev);
			switch (pfPrevStrux->getStruxType())
			{
			default:
				return;
				
			case PTX_Block:
				{
					pf_Frag_Strux_Block * pfPrevStruxBlock = static_cast<pf_Frag_Strux_Block *>(pfPrevStrux);
					pfsBlock->setPreferredSpanFmt(pfPrevStruxBlock->getPreferredSpanFmt());
				}
				return;
			}
		}
	}
}
