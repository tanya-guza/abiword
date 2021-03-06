/* AbiWord
 * Copyright (C) Ben Martin 2012.
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

#include "GTKCommon.h"
#include "ap_RDFSemanticItemGTKInjected.h"
#include "pd_Document.h"
#include "fv_View.h"
#include "xap_UnixDialogHelper.h"
#include "ap_Strings.h"
#include "xap_Frame.h"
#include "xap_UnixFrameImpl.h"
#include "xap_GtkComboBoxHelpers.h"

static gchar *s_id;

typedef struct
{
    const XAP_String_Id translation_id;  
    const char *stylesheet;  
} ssList_t;

static const ssList_t ssListContact[] =
{
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_CONTACT_NAME, RDF_SEMANTIC_STYLESHEET_CONTACT_NAME},
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_CONTACT_NICK, RDF_SEMANTIC_STYLESHEET_CONTACT_NICK},    
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_CONTACT_NAME_PHONE, RDF_SEMANTIC_STYLESHEET_CONTACT_NAME_PHONE},   
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_CONTACT_NICK_PHONE, RDF_SEMANTIC_STYLESHEET_CONTACT_NICK_PHONE},   
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_CONTACT_NAME_HOMEPAGE_PHONE, RDF_SEMANTIC_STYLESHEET_CONTACT_NAME_HOMEPAGE_PHONE},   
    {0, NULL}
};

static const ssList_t ssListEvent[] =
{
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_EVENT_NAME, RDF_SEMANTIC_STYLESHEET_EVENT_NAME},
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_EVENT_SUMMARY, RDF_SEMANTIC_STYLESHEET_EVENT_SUMMARY},
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_EVENT_SUMMARY_LOCATION, RDF_SEMANTIC_STYLESHEET_EVENT_SUMMARY_LOCATION},
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_EVENT_SUMMARY_LOCATION_TIMES, RDF_SEMANTIC_STYLESHEET_EVENT_SUMMARY_LOCATION_TIMES},
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_EVENT_SUMMARY_TIMES, RDF_SEMANTIC_STYLESHEET_EVENT_SUMMARY_TIMES},
    {0, NULL}
};

static const ssList_t ssListLocation[] =
{
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_LOCATION_NAME, RDF_SEMANTIC_STYLESHEET_LOCATION_NAME},
    {AP_STRING_ID_MENU_LABEL_RDF_SEMITEM_STYLESHEET_LOCATION_NAME_LATLONG, RDF_SEMANTIC_STYLESHEET_LOCATION_NAME_LATLONG},
    {0, NULL}
};

static const char *getStylesheetName( const ssList_t *ssList, const gchar *translation )
{
    const XAP_StringSet *pSS = XAP_App::getApp()->getStringSet();
    std::string text;
    int i;

    for (i = 0; ssList[i].stylesheet; i++)
    {
        pSS->getValueUTF8(ssList[i].translation_id, text);

        if (strcmp(translation, text.c_str()) == 0) break;
    }

    UT_DEBUGMSG(("getStylesheetName: in=\"%s\", out=\"%s\"\n", translation, ssList[i].stylesheet ? ssList[i].stylesheet : "NULL"));

    return ssList[i].stylesheet;
}

#if !GTK_CHECK_VERSION(3,0,0)
const gchar* gtk_combo_box_get_active_id( GtkComboBox* combo_box )
{
    g_free(s_id);
    s_id = gtk_combo_box_get_active_text(combo_box);
    return s_id;
}
#endif

void GDestroyNotify_GObjectSemItem(gpointer data)
{
    ap_GObjectSemItem* obj = (ap_GObjectSemItem*)data;
    delete obj;
}

PD_RDFSemanticItemHandle getHandle(GtkDialog* d)
{
    ap_GObjectSemItem* data = (ap_GObjectSemItem*)
        g_object_get_data( G_OBJECT(d), G_OBJECT_SEMITEM );
    return data->h;
}

void OnSemItemEdited ( GtkDialog* d, gint /*response_id*/, 
					   gpointer /*user_data*/)
{
    UT_DEBUGMSG(("OnSemItemEdited()\n"));
    PD_RDFSemanticItemHandle h = getHandle( d );
    h->updateFromEditorData();
    gtk_widget_destroy( GTK_WIDGET(d) );
}



void GDestroyNotify_GObjectSemItem_List(gpointer data)
{
    ap_GObjectSemItem_List* obj = (ap_GObjectSemItem_List*)data;
    delete obj;
}
PD_RDFSemanticItems getSemItemListHandle(GtkDialog* d)
{
    ap_GObjectSemItem_List* data = (ap_GObjectSemItem_List*)
        g_object_get_data( G_OBJECT(d), G_OBJECT_SEMITEM_LIST );
    return data->cl;
}
void OnSemItemListEdited ( GtkDialog* d, gint response_id, 
						   gpointer /*user_data*/)
{
    UT_DEBUGMSG(("OnSemItemListEdited() response_id:%d\n", response_id ));
    if( response_id != GTK_RESPONSE_DELETE_EVENT )
    {
        PD_RDFSemanticItems cl = getSemItemListHandle( d );
        for( PD_RDFSemanticItems::iterator ci = cl.begin(); ci != cl.end(); ++ci )
        {
            PD_RDFSemanticItemHandle c = *ci;
            c->updateFromEditorData();
        }
    }
    gtk_widget_destroy( GTK_WIDGET(d) );
}


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


static void
OnSemanticStylesheetsDialogResponse( GtkWidget* dialog,
                                     GtkTreeView* /*tree*/,
                                     FV_View* /*pView*/)
{
    gtk_widget_destroy(dialog);
}


static void
ApplySemanticStylesheets( const std::string& semItemClassRestriction,
                          const std::string& ssName )
{
    // set the RDF linking to the stylesheets
    std::list< AD_Document* > dl = XAP_App::getApp()->getDocuments();
    for( std::list< AD_Document* >::iterator diter = dl.begin(); diter != dl.end(); ++diter )
    {
        PD_Document* pDoc = dynamic_cast<PD_Document*>(*diter);
        pDoc->beginUserAtomicGlob();
        
        PD_DocumentRDFHandle rdf = pDoc->getDocumentRDF();
        PD_RDFSemanticItems   sl = rdf->getAllSemanticObjects( semItemClassRestriction );

        for( PD_RDFSemanticItems::iterator siter = sl.begin(); siter != sl.end(); ++siter )
        {
            PD_RDFSemanticItemHandle si = *siter;
            PD_RDFSemanticStylesheetHandle ss = si->findStylesheetByName(
                PD_RDFSemanticStylesheet::stylesheetTypeSystem(), ssName );

            std::set< std::string > xmlids = si->getXMLIDs();
            for( std::set< std::string >::iterator xiter = xmlids.begin(); xiter != xmlids.end(); ++xiter )
            {
                std::string xmlid = *xiter;
                PD_RDFSemanticItemViewSite vs( si, xmlid );
                vs.setStylesheetWithoutReflow( ss );
            }
        }
        pDoc->endUserAtomicGlob();
    }

    UT_DEBUGMSG(("ApplySemanticStylesheets(reflowing)\n" ));

    // reflow all the viewsites
    for( std::list< AD_Document* >::iterator diter = dl.begin(); diter != dl.end(); ++diter )
    {
        PD_Document* pDoc = dynamic_cast<PD_Document*>(*diter);
        pDoc->beginUserAtomicGlob();
        pDoc->notifyPieceTableChangeStart();
        pDoc->setDontImmediatelyLayout(true);
        
        PD_DocumentRDFHandle rdf = pDoc->getDocumentRDF();
        PD_RDFSemanticItems   sl = rdf->getAllSemanticObjects( semItemClassRestriction );

        std::list<AV_View*> vl = pDoc->getAllViews();
        for( std::list<AV_View*>::iterator viter = vl.begin(); viter != vl.end(); ++viter )
        {
            FV_View* pView = dynamic_cast<FV_View*>(*viter);

            for( PD_RDFSemanticItems::iterator siter = sl.begin(); siter != sl.end(); ++siter )
            {
                PD_RDFSemanticItemHandle si = *siter;
                std::set< std::string > xmlids = si->getXMLIDs();
                for( std::set< std::string >::iterator xiter = xmlids.begin(); xiter != xmlids.end(); ++xiter )
                {
                    std::string xmlid = *xiter;
                    PD_RDFSemanticItemViewSite vs( si, xmlid );
                    vs.reflowUsingCurrentStylesheet( pView );
                }
            }
            break;
        }
        
        pDoc->setDontImmediatelyLayout(false);
        pDoc->notifyPieceTableChangeEnd();
        pDoc->endUserAtomicGlob();
    }

    UT_DEBUGMSG(("ApplySemanticStylesheets(done)\n" ));
}


static gboolean
OnSemanticStylesheetsSetContacts_cb( GtkWidget* /*w*/, GdkEvent* /*event*/, 
									 GtkComboBoxText *combo_box )
{
    const gchar * t = getStylesheetName( ssListContact, gtk_combo_box_get_active_id( GTK_COMBO_BOX(combo_box) ) );
    std::string ssName = t ? t : "name";

    UT_DEBUGMSG(("OnSemanticStylesheetsSetContacts_cb() ssName:%s\n", ssName.c_str() ));
    UT_DEBUGMSG(("OnSemanticStylesheetsSetContacts_cb() combo:%p\n", combo_box ));
    UT_DEBUGMSG(("OnSemanticStylesheetsSetContacts_cb() t:%s\n", t ));

    ApplySemanticStylesheets( "Contact", ssName );
    
    return false;
}


static gboolean
OnSemanticStylesheetsSetEvents_cb( GtkWidget* /*w*/, GdkEvent* /*event*/, 
								   GtkComboBoxText *combo_box )
{
    const gchar * t = getStylesheetName( ssListEvent, gtk_combo_box_get_active_id( GTK_COMBO_BOX(combo_box) ) );
    std::string ssName = t ? t : "name";

    UT_DEBUGMSG(("OnSemanticStylesheetsSetEvents_cb() ssName:%s\n", ssName.c_str() ));
    UT_DEBUGMSG(("OnSemanticStylesheetsSetEvents_cb() combo:%p\n", combo_box ));
    UT_DEBUGMSG(("OnSemanticStylesheetsSetEvents_cb() t:%s\n", t ));

    ApplySemanticStylesheets( "Event", ssName );
    return false;
}

static gboolean
OnSemanticStylesheetsSetLocations_cb( GtkWidget* /*w*/, GdkEvent* /*event*/, 
									  GtkComboBoxText *combo_box )
{
    const gchar * t = getStylesheetName( ssListLocation, gtk_combo_box_get_active_id( GTK_COMBO_BOX(combo_box) ) );
    std::string ssName = t ? t : "name";

    UT_DEBUGMSG(("OnSemanticStylesheetsSetLocations_cb() ssName:%s\n", ssName.c_str() ));
    UT_DEBUGMSG(("OnSemanticStylesheetsSetLocations_cb() combo:%p\n", combo_box ));
    UT_DEBUGMSG(("OnSemanticStylesheetsSetLocations_cb() t:%s\n", t ));

    ApplySemanticStylesheets( "Location", ssName );
    return false;
}

/******************************/
/******************************/
/******************************/

enum {
    COLUMN_REFDLG_NAME = 0,
    NUM_REFDLG_COLUMNS
};

static void
OnInsertReferenceBase( GtkWidget* dialog,
                   GtkTreeView* tree,
                   FV_View* pView )
{
    PD_Document* pDoc = pView->getDocument();
    PD_DocumentRDFHandle rdf = pDoc->getDocumentRDF();

    std::string n = getSelectedText( GTK_TREE_VIEW (tree), COLUMN_REFDLG_NAME );
    UT_DEBUGMSG(("clicked on: %s\n", n.c_str() ));

    bool found = false;
    PD_RDFContacts clist = rdf->getContacts();
    for( PD_RDFContacts::iterator ci = clist.begin(); ci != clist.end(); ++ci )
    {
        PD_RDFContactHandle obj = *ci;
        if( obj->name() == n )
        {
            obj->insert( pView );
            found = true;
            break;
        }
    }
    if( found )
        gtk_widget_destroy(dialog);

}

static void OnInsertReference( GtkDialog* d, gint /*response_id*/, gpointer user_data)
{
    UT_DEBUGMSG(("OnInsertReference()\n"));
    FV_View* pView = (FV_View*)user_data;

    GtkTreeView* tv = GTK_TREE_VIEW( g_object_get_data( G_OBJECT(d), G_OBJECT_TREEVIEW ));
    OnInsertReferenceBase( GTK_WIDGET(d), tv, pView );
}

static void
OnInsertReferenceDblClicked( GtkTreeView       * tree,
                             GtkTreePath       * /*path*/,
                             GtkTreeViewColumn * /*col*/,
                             gpointer		    user_data )
{
    FV_View* pView = (FV_View*)user_data;

    GtkWidget* d = GTK_WIDGET(g_object_get_data( G_OBJECT(tree), G_OBJECT_WINDOW ));
    OnInsertReferenceBase( d, tree, pView );
}


/******************************/
/******************************/
/******************************/

class PD_RDFDialogsGTK : public PD_RDFDialogs
{
private:
    void _setIcon (GtkWidget *window)
    {
        XAP_Frame *lff = XAP_App::getApp()->getLastFocussedFrame();
        XAP_UnixFrameImpl *pUnixFrameImpl = static_cast<XAP_UnixFrameImpl *>(lff->getFrameImpl());
        GtkWidget *top = gtk_widget_get_toplevel(pUnixFrameImpl->getTopLevelWindow());
        
        if (gtk_widget_is_toplevel(top))
        {
            GdkPixbuf *icon = gtk_window_get_icon(GTK_WINDOW(top));	
            
            if (icon) gtk_window_set_icon(GTK_WINDOW(window), icon);
        }
    }
public:
    PD_RDFDialogsGTK()
    {
        PD_DocumentRDF::setRDFDialogs( this );
    }
    ~PD_RDFDialogsGTK()
    {
        g_free(s_id);
    }
    virtual void runSemanticStylesheetsDialog( FV_View* pView )
    {
        const XAP_StringSet *pSS = XAP_App::getApp()->getStringSet();
        std::string text;

#if GTK_CHECK_VERSION(3,0,0)
        GtkBuilder* builder   = newDialogBuilder("ap_UnixDialog_SemanticStylesheets.ui");
#else
        GtkBuilder* builder   = newDialogBuilder("ap_UnixDialog_SemanticStylesheets-2.ui");        
#endif
        GtkWidget*  window    = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
        GtkWidget*  lbExplanation = GTK_WIDGET(gtk_builder_get_object(builder, "lbExplanation"));         
        GtkWidget*  contacts  = GTK_WIDGET(gtk_builder_get_object(builder, "contacts"));
        GtkWidget*  events    = GTK_WIDGET(gtk_builder_get_object(builder, "events"));
        GtkWidget*  locations = GTK_WIDGET(gtk_builder_get_object(builder, "locations"));
        GtkWidget*  setContacts  = GTK_WIDGET(gtk_builder_get_object(builder, "setContacts"));
        GtkWidget*  setEvents    = GTK_WIDGET(gtk_builder_get_object(builder, "setEvents"));
        GtkWidget*  setLocations = GTK_WIDGET(gtk_builder_get_object(builder, "setLocations"));
        GtkWidget*  setAll       = GTK_WIDGET(gtk_builder_get_object(builder, "setAll"));

        // localization

        pSS->getValueUTF8(AP_STRING_ID_DLG_RDF_SemanticStylesheets_Explanation, text);
        text += "\xe2\x80\xa9";     // paragraph separator 
        gtk_label_set_text(GTK_LABEL(lbExplanation), text.c_str());
        localizeLabel(GTK_WIDGET(gtk_builder_get_object(builder, "lbContacts")), pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Contacts);
        localizeLabel(GTK_WIDGET(gtk_builder_get_object(builder, "lbEvents")), pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Events);
        localizeLabel(GTK_WIDGET(gtk_builder_get_object(builder, "lbLocations")), pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Locations);
        localizeButton(setContacts, pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Set);        
        localizeButton(setEvents, pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Set);        
        localizeButton(setLocations, pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Set);        
        localizeButton(setAll, pSS, AP_STRING_ID_DLG_RDF_SemanticStylesheets_Set);        
        // combo boxes
        for (int i = 0; ssListContact[i].stylesheet; i++)
        {
            pSS->getValueUTF8(ssListContact[i].translation_id, text);
            XAP_appendComboBoxText(GTK_COMBO_BOX(contacts), text.c_str());
        }
        for (int i = 0; ssListEvent[i].stylesheet; i++)
        {
            pSS->getValueUTF8(ssListEvent[i].translation_id, text);
            XAP_appendComboBoxText(GTK_COMBO_BOX(events), text.c_str());
        }
        for (int i = 0; ssListLocation[i].stylesheet; i++)
        {
            pSS->getValueUTF8(ssListLocation[i].translation_id, text);
            XAP_appendComboBoxText(GTK_COMBO_BOX(locations), text.c_str());
        }

        // set max. text width for explanation
        GtkRequisition requisition;
        gtk_widget_size_request(gtk_widget_get_parent(lbExplanation), &requisition);
        gtk_widget_set_size_request(lbExplanation, requisition.width, -1);

        // window title and icon
        pSS->getValueUTF8(AP_STRING_ID_DLG_RDF_SemanticStylesheets_Title, text);
        gtk_window_set_title(GTK_WINDOW(window), text.c_str());
        _setIcon(window);

        UT_DEBUGMSG(("runSemanticStylesheetsDialog_cb() combo:%p\n", contacts ));
        g_signal_connect (setContacts,  "button-release-event", G_CALLBACK (OnSemanticStylesheetsSetContacts_cb),  contacts );
        g_signal_connect (setEvents,    "button-release-event", G_CALLBACK (OnSemanticStylesheetsSetEvents_cb),    events );
        g_signal_connect (setLocations, "button-release-event", G_CALLBACK (OnSemanticStylesheetsSetLocations_cb), locations );

        g_signal_connect (setAll, "button-release-event", G_CALLBACK (OnSemanticStylesheetsSetContacts_cb),  contacts );
        g_signal_connect (setAll, "button-release-event", G_CALLBACK (OnSemanticStylesheetsSetEvents_cb),    events );
        g_signal_connect (setAll, "button-release-event", G_CALLBACK (OnSemanticStylesheetsSetLocations_cb), locations );
    
    
    
        g_signal_connect (G_OBJECT(window), "response",  G_CALLBACK(OnSemanticStylesheetsDialogResponse), pView );
        gtk_widget_show_all (window);
        
    }
    std::pair< PT_DocPosition, PT_DocPosition > runInsertReferenceDialog( FV_View* pView )
    {
        const XAP_StringSet *pSS = XAP_App::getApp()->getStringSet();
        std::string text;

#if GTK_CHECK_VERSION(3,0,0)
        GtkBuilder* builder = newDialogBuilder("pd_RDFInsertReference.ui");
#else
        GtkBuilder* builder = newDialogBuilder("pd_RDFInsertReference-2.ui");
#endif
        GtkWidget*  window  = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
        GtkWidget*  tv      = GTK_WIDGET(gtk_builder_get_object(builder, "tv"));

        // localization
        localizeButton(GTK_WIDGET(gtk_builder_get_object(builder, "ok")), pSS, AP_STRING_ID_DLG_RDF_SemanticItemInsert_Ok);

        // window title and icon
        pSS->getValueUTF8(AP_STRING_ID_DLG_RDF_SemanticItemInsert_Title, text);
        gtk_window_set_title(GTK_WINDOW(window), text.c_str());
        _setIcon(window);

        PD_Document* pDoc = pView->getDocument();
        PD_DocumentRDFHandle rdf = pDoc->getDocumentRDF();

        {
            GtkTreeStore *store = gtk_tree_store_new ( NUM_REFDLG_COLUMNS, G_TYPE_STRING );
            gtk_tree_view_set_model (GTK_TREE_VIEW (tv), GTK_TREE_MODEL (store));
            g_object_unref (G_OBJECT (store));
        }
    
        GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));

        GtkTreeViewColumn *column = NULL;
        GtkCellRenderer *renderer = NULL;
        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
                                                     -1, "Name", renderer,
                                                     "text", COLUMN_REFDLG_NAME,
                                                     NULL);
        column = gtk_tree_view_get_column (GTK_TREE_VIEW (tv), COLUMN_REFDLG_NAME );
        gtk_tree_view_column_set_sort_column_id (column, COLUMN_REFDLG_NAME );
    
        PD_RDFContacts l = rdf->getContacts();
        GtkTreeIter giter;
        GtkTreeIter parentiter;

        if (l.begin() != l.end())
        {
            pSS->getValueUTF8(AP_STRING_ID_DLG_RDF_SemanticItemInsert_Column_Refdlg, text);
            gtk_tree_store_append (GTK_TREE_STORE (model), &parentiter, 0);
            gtk_tree_store_set (GTK_TREE_STORE (model), &parentiter, 
                                COLUMN_REFDLG_NAME, text.c_str(),
                                -1);
        }
    
        for( PD_RDFContacts::iterator iter = l.begin(); iter != l.end(); ++iter )
        {
            PD_RDFContactHandle c = *iter;
            gtk_tree_store_append (GTK_TREE_STORE (model), &giter, &parentiter );
            gtk_tree_store_set (GTK_TREE_STORE (model), &giter, 
                                COLUMN_REFDLG_NAME, c->name().c_str(),
                                -1);

        }
        gtk_tree_view_expand_all(GTK_TREE_VIEW(tv));
        g_object_set_data( G_OBJECT(tv),     G_OBJECT_WINDOW,   window );
        g_object_set_data( G_OBJECT(window), G_OBJECT_TREEVIEW, tv );
    
        g_signal_connect (GTK_TREE_VIEW (tv), "row-activated", 
                          G_CALLBACK (OnInsertReferenceDblClicked), static_cast <gpointer>(pView));
        g_signal_connect (G_OBJECT(window), "response",  G_CALLBACK(OnInsertReference), pView );
        gtk_widget_show_all (window);

        std::pair< PT_DocPosition, PT_DocPosition > ret;
        return ret;
    }
    
};

namespace 
{
    PD_RDFDialogsGTK __obj;
};
