/* -*- mode: C++; tab-width: 4; c-basic-offset: 4; -*- */

/* AbiSource Application Framework
 * Copyright (C) 2001 Dom Lachowicz <doml@appligent.com>
 * Copyright (C) 2001 Hubert Figuiere
 * Copyright (C) 2004 Francis James Franklin
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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <glib.h>
#include <gmodule.h>

#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_path.h"
#include "ut_string.h"

#include "xap_App.h"
#include "xap_CocoaAppController.h"
#include "xap_CocoaModule.h"
#include "xap_CocoaPlugin.h"
#include "xap_ModuleManager.h"

XAP_CocoaModule::XAP_CocoaModule () :
	m_szname("??"),
	m_module_path("??"),
	m_module(0),
	m_bLoaded(false)
{
	// 
}

XAP_CocoaModule::~XAP_CocoaModule (void)
{
	// 
}

bool XAP_CocoaModule::getModuleName (char ** dest) const
{
	if (dest)
	{
		*dest = (char *) UT_strdup(m_szname.utf8_str());
		return (*dest ? true : false);
	}
	return false;
}

bool XAP_CocoaModule::load (const char * name)
{
	if (m_bLoaded)
		return false;

	if (!name)
		return false;

	m_szname = UT_basename(name);

	m_module_path = name;

	if (m_module_path.byteLength() > 7)
		if (strcmp(m_module_path.utf8_str() + m_module_path.byteLength() - 7, ".so-abi") == 0)
		{
			/* This is an ordinary plugin.
			 */
			m_bLoaded = false;

			if (m_module = g_module_open(name, (GModuleFlags) 0))
			{
				m_bLoaded = true;
				UT_DEBUGMSG(("FJF: plugin loaded: \"%s\"\n", m_module_path.utf8_str()));

				UT_UTF8String config_path(m_module_path.utf8_str(), m_module_path.byteLength() - 6);

				config_path += "config";

				if (FILE * config = fopen(config_path.utf8_str(), "r"))
				{
					fclose(config);
					UT_DEBUGMSG(("FJF: plugin config: \"%s\"\n", config_path.utf8_str()));

					int (*plugin_preconfigure_func) (const char * config_path);

					if (resolveSymbol("abi_plugin_preconfigure", reinterpret_cast<void **>(&plugin_preconfigure_func)))
					{
						plugin_preconfigure_func(config_path.utf8_str());
					}
				}
			}
#if 0
			else
			{
				fprintf(stderr, "module failed to load: error=\"%s\"\n", g_module_error());
			}
#endif
		}
	return m_bLoaded;
}

bool XAP_CocoaModule::unload (void)
{
	if (!m_bLoaded || !m_module)
		return false;

	if (g_module_close((GModule *) m_module))
	{
		m_bLoaded = false;
		m_module = 0;
		return true;
	}
	return false;
}

bool XAP_CocoaModule::resolveSymbol (const char * symbol_name, void ** symbol)
{
	if (!m_bLoaded || !m_module)
		return false;

	if (!symbol_name || !symbol)
		return false;

	return (g_module_symbol((GModule *) m_module, symbol_name, symbol) ? true : false);
}

bool XAP_CocoaModule::getErrorMsg (char ** dest) const
{
	if (!m_bLoaded || !m_module)
		return false;

	if (!dest)
		return false;

	*dest = (char *) UT_strdup(g_module_error());
	return (*dest ? true : false);
}

/**
 * return > 0 for directory entries ending in ".Abi"
 */
static int s_Abi_only (struct dirent * d)
{
	const char * name = d->d_name;

	if (name)
		{
			int length = strlen (name);

			if (length > 4)
				if (strcmp (name + length - 4, ".Abi") == 0)
					return 1;

			if (length > 7)
				if (strcmp (name + length - 7, ".so-abi") == 0)
					return 1;
		}
	return 0;
}

/**
 * return true if dirname exists and is a directory; symlinks probably not counted
 */
static bool s_dir_exists (const char * dirname)
{
	struct stat sbuf;

	if (stat (dirname, &sbuf) == 0)
		if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
			return true;

	return false;
}

/*!
  Creates a directory if the specified one does not yet exist.
  /param  A character string representing the to-be-created directory.
  /return True,  if the directory already existed, or was successfully created.
		  False, if the input path was already a file, not a directory, or if the directory was unable to be created.
*/
static bool s_createDirectoryIfNecessary(const char * szDir, bool publicdir = false)
{
    struct stat statbuf;
    
    if (stat(szDir,&statbuf) == 0)								// if it exists
    {
		if (S_ISDIR(statbuf.st_mode))							// and is a directory
			return true;

		UT_DEBUGMSG(("Pathname [%s] is not a directory.\n",szDir));
		return false;
    }

	bool success = true;
    mode_t old_mask = umask (0);
    if (mkdir (szDir, publicdir ? 0775 : 0700))
		{   
			UT_DEBUGMSG(("Could not create Directory [%s].\n",szDir));
			success = false;
		}
	umask (old_mask);
	return success;
}	

/**
 * MacOSX applications look for plugins in Contents/Plug-ins, and there's probably
 * no need to jump through scandir hoops identifying these. Third party plugins or
 * plugins not distributed with AbiWord.app can be found in the system directory
 * "/Library/Application Support" or in the user's home equivalent - I'm choosing
 * to make the plug-in directory "/Library/Application Support/AbiWord/Plug-ins".
 * 
 * The System Overview recommends that plugins be given a suffix that the
 * application claims (presumably as a document type) so I'm opting for the suffix
 * ".Abi".
 */
void XAP_CocoaModule::loadAllPlugins ()
{
	int support_dir_count = 0;

	UT_UTF8String support_dir[3];

	/* Load from:
	 *  a. "/Library/Application Support/AbiSuite/Plug-ins"
	 *  b. "/Library/Application Support/AbiSuite/Plug-ins"
	 *  c. "$HOME/Library/Application Support/AbiSuite/Plug-ins"
	 */

	NSString * app_path = [[NSBundle mainBundle] bundlePath];
	if (app_path)
		if (NSString * plugin_path = [app_path stringByAppendingString:@"/Contents/Plug-ins"])
			{
				support_dir[support_dir_count] = [plugin_path UTF8String];

				if (s_dir_exists (support_dir[support_dir_count].utf8_str()))
					{
						UT_DEBUGMSG(("FJF: path to bundle's plug-ins: %s\n",support_dir[support_dir_count].utf8_str()));
						support_dir_count++;
					}
				else
					{
						UT_DEBUGMSG(("FJF: path to bundle's plug-ins: %s (not found)\n",support_dir[support_dir_count].utf8_str()));
					}
			}

	/* create the system plugins directory - if we can...
	 */
	if (s_createDirectoryIfNecessary ("/Library", true))
		if (s_createDirectoryIfNecessary ("/Library/Application Support", true))
			if (s_createDirectoryIfNecessary ("/Library/Application Support/AbiSuite", true))
				s_createDirectoryIfNecessary ("/Library/Application Support/AbiSuite/Plug-ins", true);

	support_dir[support_dir_count] = "/Library/Application Support/AbiSuite/Plug-ins";

	if (!s_dir_exists (support_dir[support_dir_count].utf8_str()))
		{
			UT_DEBUGMSG(("FJF: %s: no such directory\n",support_dir[support_dir_count].utf8_str()));
		}
	else
		{
			UT_DEBUGMSG(("FJF: adding to plug-in search path: %s\n",support_dir[support_dir_count].utf8_str()));
			support_dir_count++;
		}

	const char * homedir = XAP_App::getApp()->getUserPrivateDirectory();
	if (homedir == 0)
		{
			UT_DEBUGMSG(("FJF: no home directory?\n"));
		}
	else if (!s_dir_exists (homedir))
		{
			UT_DEBUGMSG(("FJF: invalid home directory?\n"));
		}
	else
		{
			UT_UTF8String plugin_dir(homedir);
			plugin_dir += "/Plug-ins";
			if (!s_createDirectoryIfNecessary (plugin_dir.utf8_str()))
				{
					UT_DEBUGMSG(("FJF: %s: no such directory\n",plugin_dir.utf8_str()));
				}
			else
				{
					UT_DEBUGMSG(("FJF: adding to plug-in search path: %s\n",plugin_dir.utf8_str()));
					support_dir[support_dir_count++] = plugin_dir;
				}
		}

	for (int i = 0; i < support_dir_count; i++)
		{
			struct dirent ** namelist = 0;
			int n = scandir (support_dir[i].utf8_str(), &namelist, s_Abi_only, alphasort);
			UT_DEBUGMSG(("DOM: found %d plug-ins in %s\n", n, support_dir[i].utf8_str()));
			if (n < 0)
				{
					continue;
				}
			if (n == 0)
				{
					FREEP (namelist);
					continue;
				}

			UT_UTF8String plugin_path;
			while (n--)
				{
					plugin_path  = support_dir[i];
					plugin_path += '/';
					plugin_path += namelist[n]->d_name;

					UT_DEBUGMSG(("FJF: loading plugin %s\n", plugin_path.utf8_str()));

					if (XAP_CocoaModule::loadPlugin (plugin_path))
						{
							UT_DEBUGMSG(("FJF: loaded plug-in: %s\n", namelist[n]->d_name));
						}
					else
						{
							UT_DEBUGMSG(("FJF: didn't load plug-in: %s\n", namelist[n]->d_name));
						}
					free (namelist[n]);
				}
			free (namelist);
		}
}

bool XAP_CocoaModule::loadPlugin (const UT_UTF8String & path)
{
	bool bAbi = false;
	bool bLoaded = false;

	if (path.length() > 4) {
		if (strcmp(path.utf8_str() + path.length() - 4, ".Abi") == 0) {
			XAP_CocoaAppController * pController = (XAP_CocoaAppController *) [NSApp delegate];

			XAP_CocoaPlugin * plugin = [pController loadPlugin:[NSString stringWithUTF8String:(path.utf8_str())]];

			if (plugin) {
				[[plugin delegate] pluginActivate];
				bLoaded = true; // we have no idea if activate succeeded. just assume so.
			}
			bAbi = true;
		}
	}

	if (!bAbi && (path.length() > 7)) {
		if (strcmp(path.utf8_str() + path.length() - 7, ".so-abi") == 0) {
			bLoaded = XAP_ModuleManager::instance().loadModule (path.utf8_str());
		}
	}

	return bLoaded;
}

bool XAP_CocoaModule::hasPluginExtension (const UT_UTF8String & path)
{
	bool bHasPluginExtension = false;

	if (path.length() > 4)
		if (strcmp(path.utf8_str() + path.length() - 4, ".Abi") == 0)
			{
				bHasPluginExtension = true;
			}

	if (!bHasPluginExtension && (path.length() > 7))
		if (strcmp(path.utf8_str() + path.length() - 7, ".so-abi") == 0)
			{
				bHasPluginExtension = true;
			}

	return bHasPluginExtension;
}
