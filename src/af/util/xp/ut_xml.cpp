/* AbiWord
 * Copyright (C) 2001 AbiSource, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ut_assert.h"
#include "ut_debugmsg.h"
#include "ut_string.h"
#include "ut_xml.h"

#include "xap_EncodingManager.h"


DefaultReader::DefaultReader () :
  in(0)
{
}

DefaultReader::~DefaultReader ()
{
  if (in) fclose (in);
}

bool DefaultReader::openFile (const char * szFilename)
{
  in = fopen (szFilename, "r");
  return (in != NULL);
}

UT_uint32 DefaultReader::readBytes (char * buffer, UT_uint32 length)
{
  UT_ASSERT(in);
  UT_ASSERT(buffer);

  return fread (buffer, 1, length, in);
}

void DefaultReader::closeFile (void)
{
  if (in) fclose (in);
  in = 0;
}

UT_XML::UT_XML () :
  m_namespace(0),
  m_nslength(0),
  m_bSniffing(false),
  m_bValid(false),
  m_xml_type(0),
  m_bStopped(false),
  m_pListener(0),
  m_pReader(0),
  m_decoder(0)
{
}

/* these functions needs to be declared as plain C for MrCPP (Apple MPW C)
 */

/* Declared in ut_xml.h as: void UT_XML::startElement (const XML_Char * name, const XML_Char ** atts);
 */
void UT_XML::startElement (const char * name, const char ** atts)
{
  if (m_bStopped) return;
  if (m_nslength)
    if (strncmp (name,m_namespace,m_nslength) == 0)
      {
	if (*(name + m_nslength) == ':') name += m_nslength + 1;
      }
  if (m_bSniffing)
    {
      if (strcmp (name,m_xml_type) == 0) m_bValid = true;
      stop (); // proceed no further - we don't have any listener
      return;
    }

  UT_ASSERT (m_pListener);
  m_pListener->startElement (name, atts);
}

/* Declared in ut_xml.h as: void UT_XML::endElement (const XML_Char * name);
 */
void UT_XML::endElement (const char * name)
{
  if (m_bStopped) return;
  if (m_nslength)
    if (strncmp (name,m_namespace,m_nslength) == 0)
      {
	if (*(name + m_nslength) == ':') name += m_nslength + 1;
      }

  UT_ASSERT (m_pListener);
  m_pListener->endElement (name);
}

/* Declared in ut_xml.h as: void UT_XML::charData (const XML_Char * buffer, int length);
 */
void UT_XML::charData (const char * buffer, int length)
{
  if (m_bStopped) return;

  UT_ASSERT (m_pListener);
  m_pListener->charData (buffer, length);
}

/* I'm still very confused about XML namespaces so the handling here is likely to change a lot as I learn...
 */
void UT_XML::setNameSpace (const char * xml_namespace)
{
  FREEP (m_namespace);
  if (xml_namespace) m_namespace = UT_strdup (xml_namespace);

  m_nslength = 0;
  if (m_namespace) m_nslength = strlen (m_namespace);
}

bool UT_XML::sniff (const UT_ByteBuf * pBB, const char * xml_type)
{
  UT_ASSERT (pBB);
  UT_ASSERT (xml_type);

  const char * buffer = (const char *) pBB->getPointer (0);
  UT_uint32 length = pBB->getLength ();

  return sniff (buffer, length, xml_type);
}

bool UT_XML::sniff (const char * buffer, UT_uint32 length, const char * xml_type)
{
  UT_ASSERT (buffer);
  UT_ASSERT (xml_type);

  m_bSniffing = true; // This *must* be reset to false before returning
  m_bValid = true;

  m_xml_type = xml_type;

  bool valid = false;
  if (parse (buffer, length) == UT_OK) valid = m_bValid;

  m_bSniffing = false;
  return valid;
}

UT_Error UT_XML::parse (const UT_ByteBuf * pBB)
{
  UT_ASSERT (m_pListener);
  UT_ASSERT (pBB);

  const char * buffer = (const char *) pBB->getPointer (0);
  UT_uint32 length = pBB->getLength ();

  return parse (buffer, length);
}

