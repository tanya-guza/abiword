/* AbiWord
 * Copyright (C) 2000 AbiSource, Inc.
 * Copyright (C) 2003 Hubert Figuiere
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

#include <stdlib.h>

#include "ut_string.h"
#include "ut_assert.h"
#include "ut_debugmsg.h"

#include "xap_CocoaDialog_Utilities.h"

#include "xap_App.h"
#include "xap_CocoaApp.h"
#include "xap_CocoaFrame.h"

#include "ap_Strings.h"
#include "ap_Dialog_Id.h"
#include "ap_Dialog_New.h"
#include "ap_CocoaDialog_New.h"

#include "xap_Dlg_FileOpenSaveAs.h"
#include "ie_imp.h"

/*************************************************************************/

XAP_Dialog * AP_CocoaDialog_New::static_constructor(XAP_DialogFactory * pFactory,
												   XAP_Dialog_Id dlgid)
{
	AP_CocoaDialog_New * p = new AP_CocoaDialog_New(pFactory, dlgid);
	return p;
}

AP_CocoaDialog_New::AP_CocoaDialog_New(XAP_DialogFactory * pDlgFactory,
										 XAP_Dialog_Id dlgid)
	: AP_Dialog_New(pDlgFactory, dlgid), m_pFrame(0)
{
}

AP_CocoaDialog_New::~AP_CocoaDialog_New(void)
{
}

void AP_CocoaDialog_New::runModal(XAP_Frame * pFrame)
{
	m_pFrame = pFrame;
	m_dlg = [[AP_CocoaDialog_NewController alloc] initFromNib];
	[m_dlg setXAPOwner:this];
	NSWindow* win = [m_dlg window];

	// Populate the window's data items
//	_populateWindowData();
	
	[NSApp runModalForWindow:win];

//	_storeWindowData();
	[m_dlg close];
	[m_dlg release];
	m_dlg = nil;
	m_pFrame = NULL;
}

/*************************************************************************/
/*************************************************************************/

void AP_CocoaDialog_New::event_Ok ()
{
	setAnswer (AP_Dialog_New::a_OK);

	if ([m_dlg existingBtnState])
	{
		setOpenType(AP_Dialog_New::open_Existing);
	}
	else if ([m_dlg emptyBtnState])
	{
		setOpenType(AP_Dialog_New::open_Template);
	}
	else
	{
		setOpenType(AP_Dialog_New::open_New);
	}

	[NSApp stopModal];
}

void AP_CocoaDialog_New::event_Cancel ()
{
	setAnswer (AP_Dialog_New::a_CANCEL);
	[NSApp stopModal];
}

void AP_CocoaDialog_New::event_ToggleOpenExisting ()
{
	XAP_Dialog_Id dlgid = XAP_DIALOG_ID_FILE_OPEN;

	XAP_DialogFactory * pDialogFactory
		= (XAP_DialogFactory *) m_pFrame->getDialogFactory();

	XAP_Dialog_FileOpenSaveAs * pDialog
		= (XAP_Dialog_FileOpenSaveAs *)(pDialogFactory->requestDialog(dlgid));
	UT_ASSERT(pDialog);

	pDialog->setCurrentPathname(0);
	pDialog->setSuggestFilename(false);

	UT_uint32 filterCount = IE_Imp::getImporterCount();
	const char ** szDescList = (const char **) UT_calloc(filterCount + 1,
													  sizeof(char *));
	const char ** szSuffixList = (const char **) UT_calloc(filterCount + 1,
														sizeof(char *));
	IEFileType * nTypeList = (IEFileType *) UT_calloc(filterCount + 1,
												   sizeof(IEFileType));
	UT_uint32 k = 0;

	while (IE_Imp::enumerateDlgLabels(k, &szDescList[k], 
									  &szSuffixList[k], &nTypeList[k]))
			k++;

	pDialog->setFileTypeList(szDescList, szSuffixList, 
							 (const UT_sint32 *) nTypeList);

	pDialog->setDefaultFileType(IE_Imp::fileTypeForSuffix(".abw"));

	pDialog->runModal(m_pFrame);

	XAP_Dialog_FileOpenSaveAs::tAnswer ans = pDialog->getAnswer();
	bool bOK = (ans == XAP_Dialog_FileOpenSaveAs::a_OK);

	if (bOK)
	{
		const char * szResultPathname = pDialog->getPathname();
		if (szResultPathname && *szResultPathname)
		{
			// update the entry box
			[m_dlg setFileName:[NSString stringWithUTF8String:szResultPathname]];
			setFileName (szResultPathname);
		}

		[m_dlg setExistingBtnState:YES];
	}

	FREEP(szDescList);
	FREEP(szSuffixList);
	FREEP(nTypeList);
	
	pDialogFactory->releaseDialog(pDialog);
}

void AP_CocoaDialog_New::event_ToggleStartNew ()
{	
	// nada
}

/*************************************************************************/
/*************************************************************************/

@implementation AP_CocoaDialog_NewController

- (void)dealloc
{
	[m_templates release];
	[_dataSource release];
	[super dealloc];
}


- (id)initFromNib
{
	self = [super initWithWindowNibName:@"ap_CocoaDialog_New"];
	m_templates = [[NSMutableArray alloc] init];
	return self;
}


- (void)setXAPOwner:(XAP_Dialog *)owner
{
	_xap = dynamic_cast<AP_CocoaDialog_New*>(owner);
	UT_ASSERT (_xap);
}

- (void)discardXAP
{
	_xap = nil;
}

- (void)windowDidLoad
{
	const XAP_StringSet * pSS = XAP_App::getApp()->getStringSet();

	LocalizeControl([self window], pSS, AP_STRING_ID_DLG_NEW_Title);
	LocalizeControl(_okBtn, pSS, XAP_STRING_ID_DLG_OK);
	LocalizeControl(_cancelBtn, pSS, XAP_STRING_ID_DLG_Cancel);
	LocalizeControl(_chooseFileBtn, pSS, AP_STRING_ID_DLG_NEW_Choose);
	LocalizeControl(_createNewBtn, pSS, AP_STRING_ID_DLG_NEW_Create);
	LocalizeControl(_startEmptyBtn, pSS, AP_STRING_ID_DLG_NEW_StartEmpty);
	LocalizeControl(_openBtn, pSS, AP_STRING_ID_DLG_NEW_Open);
	[self synchronizeGUI:_startEmptyBtn];	// TODO check what is the default
	
	_dataSource = [[XAP_StringListDataSource alloc] init];
	NSMutableArray *templateDirs = [[NSMutableArray alloc] init];
	
	[templateDirs addObject:[NSString stringWithFormat:@"%s/templates/", XAP_App::getApp()->getUserPrivateDirectory()]];
	[templateDirs addObject:[NSString stringWithFormat:@"%s/templates/", XAP_App::getApp()->getAbiSuiteLibDir()]];

	NSEnumerator* iter = [templateDirs objectEnumerator];
	NSString * obj;
	while (obj = [iter nextObject]) {
		NSArray* files = [[NSFileManager defaultManager] directoryContentsAtPath:obj];
		if (files) {
			NSEnumerator *iter2 = [files objectEnumerator];
			NSString* obj2;
			while (obj2 = [iter2 nextObject]) {
				[_dataSource addString:obj2];
				[m_templates addObject:[NSString stringWithFormat:@"%@%@", obj, obj2]];
			}
		}
	}
	
	[templateDirs release];
	
	[_templateList setDataSource:_dataSource];
}


- (IBAction)cancelAction:(id)sender
{
	_xap->event_Cancel();
}

- (IBAction)radioButtonAction:(id)sender
{
	[self synchronizeGUI:sender];
}

- (IBAction)chooseAction:(id)sender
{
	_xap->event_ToggleOpenExisting();
}

- (IBAction)okAction:(id)sender
{
	_xap->event_Ok();
}

- (void)synchronizeGUI:(NSControl*)control
{
	enum { NONE, NEW, OPEN, EMPTY } selected;
	
	if (control == _createNewBtn) {
		selected = NEW;
	}
	else if (control == _openBtn) {
		selected = OPEN;
	}
	else if (control == _startEmptyBtn) {
		selected = EMPTY;
	}
	else {
		selected = NONE;
	}
	switch (selected) {
	case NEW:
		[_startEmptyBtn setState:NSOffState];
		[_createNewBtn setState:NSOnState];
		[_templateList setEnabled:YES];
		[_openBtn setState:NSOffState];
		[_documentNameData setEnabled:NO];
		[_chooseFileBtn setEnabled:NO];
		break;
	case OPEN:
		[_startEmptyBtn setState:NSOffState];
		[_createNewBtn setState:NSOffState];
		[_templateList setEnabled:NO];
		[_openBtn setState:NSOnState];
		[_documentNameData setEnabled:YES];
		[_chooseFileBtn setEnabled:YES];
		break;
	case EMPTY:
		[_startEmptyBtn setState:NSOnState];
		[_createNewBtn setState:NSOffState];
		[_templateList setEnabled:NO];
		[_openBtn setState:NSOffState];
		[_documentNameData setEnabled:NO];
		[_chooseFileBtn setEnabled:NO];
		break;
	default:
		break;
	}
}


- (BOOL)existingBtnState
{
	return ([_openBtn state] == NSOnState);
}

- (void)setExistingBtnState:(BOOL)state
{
	if (state) {
		[_openBtn setState:NSOnState];
	}
	else {
		[_openBtn setState:NSOffState];
	}
}

- (BOOL)newBtnState
{
	return ([_createNewBtn state] == NSOnState);
}

- (BOOL)emptyBtnState
{
	return ([_startEmptyBtn state] == NSOnState);
}

- (void)setFileName:(NSString*)name
{
	[_documentNameData setStringValue:name];
}

@end
