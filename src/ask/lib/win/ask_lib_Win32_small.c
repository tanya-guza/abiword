/*
 * AbiSource Setup Kit
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

#include <windows.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>

#include "ask.h"

long ASK_getDiskFreeSpace(const char* pszDir)
{
	// TODO GetDiskFreeSpaceEx()

	return 0;
}

unsigned int ASK_getFileModTime(const char* pszFileName)
{
	struct _stat st;

	_stat(pszFileName, &st);

	return st.st_mtime;
}

void ASK_setFileModTime(const char* pszFileName, unsigned int iModTime)
{
	struct _stat st;
	struct _utimbuf ut;

	_stat(pszFileName, &st);

	ut.actime = st.st_atime;
	ut.modtime = iModTime;

	_utime(pszFileName, &ut);
}

unsigned int ASK_getFileAttributes(const char* pszFileName)
{
	return GetFileAttributes(pszFileName);
}

void ASK_setFileAttributes(const char* pszFileName, unsigned int iAttributes)
{
	SetFileAttributes(pszFileName, iAttributes);
}

void ASK_fixSlashes(char* pszPath)
{
	char* p = pszPath;
	while (p && *p)
	{
		if (*p == '/')
		{
			*p = '\\';
		}

		p++;
	}
}

int ASK_isDirectory(char* pszFile)
{
	DWORD att = GetFileAttributes(pszFile);

	if (att == 0xffffffff)
	{
		return 0;
	}

	return (att & FILE_ATTRIBUTE_DIRECTORY);
}

int ASK_fileExists(char* pszFile)
{
	DWORD att = GetFileAttributes(pszFile);

	if (att == 0xffffffff)
	{
		DWORD err = GetLastError();

		if (err == ERROR_FILE_NOT_FOUND)
		{
			return 0;
		}

		if (err == ERROR_PATH_NOT_FOUND)
		{
			return 0;
		}

		// TODO what to do here?
		return 0;
	}

	return 1;
}

