

/* AbiSuite
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
 
#include "ap_App.h"

#ifndef ABI_BUILD_ID
#define ABI_BUILD_ID		"unknown"
#endif /* ABI_BUILD_ID */

#ifndef ABI_BUILD_VERSION
#define ABI_BUILD_VERSION		""
#endif /* ABI_BUILD_VERSION */

const char* AP_App::s_szBuild_ID = ABI_BUILD_ID;
const char* AP_App::s_szBuild_Version = ABI_BUILD_VERSION;
const char* AP_App::s_szBuild_CompileTime = __TIME__;
const char* AP_App::s_szBuild_CompileDate = __DATE__;

