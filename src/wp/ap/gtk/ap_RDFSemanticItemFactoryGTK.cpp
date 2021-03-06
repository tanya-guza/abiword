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


#include "ap_RDFSemanticItemFactoryGTK.h"


AP_SemanticItemFactoryGTK::AP_SemanticItemFactoryGTK()
{
    UT_DEBUGMSG(("AP_SemanticItemFactoryGTK()\n"));
    PD_DocumentRDF::setSemanticItemFactory( this );
}
    
PD_RDFContact*
AP_SemanticItemFactoryGTK::createContact( PD_DocumentRDFHandle rdf, PD_ResultBindings_t::iterator it )
{
    return new AP_RDFContactGTK( rdf, it );
}
PD_RDFEvent*
AP_SemanticItemFactoryGTK::createEvent( PD_DocumentRDFHandle rdf, PD_ResultBindings_t::iterator it )
{
    return new AP_RDFEventGTK( rdf, it );
}
PD_RDFLocation*
AP_SemanticItemFactoryGTK::createLocation( PD_DocumentRDFHandle rdf, PD_ResultBindings_t::iterator it,
                                           bool isGeo84 )
{
    return new AP_RDFLocationGTK( rdf, it, isGeo84  );
}

namespace 
{
    static AP_SemanticItemFactoryGTK __obj;
};
