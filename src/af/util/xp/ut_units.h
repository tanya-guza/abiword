 
/*
** The contents of this file are subject to the AbiSource Public
** License Version 1.0 (the "License"); you may not use this file
** except in compliance with the License. You may obtain a copy
** of the License at http://www.abisource.com/LICENSE/ 
** 
** Software distributed under the License is distributed on an
** "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
** implied. See the License for the specific language governing
** rights and limitations under the License. 
** 
** The Original Code is AbiSource Utilities.
** 
** The Initial Developer of the Original Code is AbiSource, Inc.
** Portions created by AbiSource, Inc. are Copyright (C) 1998 AbiSource, Inc. 
** All Rights Reserved. 
** 
** Contributor(s):
**  
*/

#ifndef UT_UNITS_H
#define UT_UNITS_H

#include "ut_types.h"
class DG_Graphics;

NSPR_BEGIN_EXTERN_C

#define UT_PAPER_UNITS_PER_INCH				100

double UT_convertToInches(const char* s);
UT_sint32 UT_paperUnits(const char * sz);
UT_sint32 UT_docUnitsFromPaperUnits(DG_Graphics * pG, UT_sint32 iPaperUnits);

NSPR_END_EXTERN_C

#endif /* UT_UNITS_H */
