#define  DONT_USE_GLOBALS

#include "ispell.h"
#include "ut_iconv.h"

#include "sp_spell.h"
#include "ispell_checker.h"
#include "ut_vector.h"

#include "xap_App.h"
#include "ut_string_class.h"

#include "ut_string.h"
#include "ut_debugmsg.h"

// for a silly messagebox
#include <stdio.h>
#include "xap_Frame.h"
#include "xap_Strings.h"

/***************************************************************************/
/* Reduced Gobals needed by ispell code.                                   */
/***************************************************************************/
#ifndef DONT_USE_GLOBALS
       int              numhits;
struct success          hits[MAX_HITS];

struct flagptr          pflagindex[SET_SIZE + MAXSTRINGCHARS];/*prefix index*/ 
struct flagent          *pflaglist; /* prefix flag control list */
       int              numpflags;  /* Number of prefix flags in table*/
struct flagptr          sflagindex[SET_SIZE + MAXSTRINGCHARS];/*suffix index*/
struct flagent          *sflaglist; /* suffix flag control list */
       int              numsflags;  /* Number of prefix flags in table*/

struct hashheader       hashheader; /* Header of hash table */
       int              hashsize;   /* Size of main hash table */
       char             *hashstrings = NULL; /* Strings in hash table */
struct dent             *hashtbl;    /* Main hash table, for dictionary */

struct strchartype      *chartypes;  /* String character type collection */
       int              defdupchar;   /* Default duplicate string type */
       unsigned int     laststringch; /* Number of last string character */


char     possibilities[MAXPOSSIBLE][INPUTWORDLEN + MAXAFFIXLEN];
                                /* Table of possible corrections */
int      pcount;         /* Count of possibilities generated */
int      maxposslen;     /* Length of longest possibility */
int      easypossibilities; /* Number of "easy" corrections found */
                                /* ..(defined as those using legal affixes) */

int deftflag = -1;              /* NZ for TeX mode by default */
int prefstringchar = -1;        /* Preferred string character type */
UT_iconv_t  translate_in = (UT_iconv_t)-1; /* Selected translation from/to Unicode */
UT_iconv_t  translate_out = (UT_iconv_t)-1;

/*
 * The following array contains a list of characters that should be tried
 * in "missingletter."  Note that lowercase characters are omitted.
 */
int      Trynum;         /* Size of "Try" array */
ichar_t  Try[SET_SIZE + MAXSTRINGCHARS];

#endif

static void try_autodetect_charset(FIRST_ARG(istate) char* hashname)
{
	int len;
	char buf[3000];
	FILE* f;
	if (strlen(hashname)>(3000-15))
		return;
	sprintf(buf,"%s-%s",hashname,"encoding");
	f = fopen(buf,"r");
	if (!f)
		return;
	len = fread(buf,1,sizeof(buf),f);
	if (len<=0)
		return;
	buf[len]=0;
	fclose(f);
	{
		char* start, *p = buf;
		while (*p==' ' || *p=='\t' || *p=='\n')
			++p;
		start = p;
		while (!(*p==' ' || *p=='\t' || *p=='\n' || *p=='\0'))
			++p;
		*p = '\0';
		if (!*start) /* empty enc */
			return;
        	DEREF(istate, translate_in) = UT_iconv_open(start, UCS_2_INTERNAL);
        	DEREF(istate, translate_out) = UT_iconv_open(UCS_2_INTERNAL, start);
	}
	
}

/***************************************************************************/

ISpellChecker::ISpellChecker()
  : deftflag(-1), prefstringchar(-1), g_bSuccessfulInit(false)
{
#if defined(DONT_USE_GLOBALS)
	m_pISpellState = NULL;
#endif
}

ISpellChecker::~ISpellChecker()
{
#if defined(DONT_USE_GLOBALS)
  if (!m_pISpellState)
    return;

    lcleanup(m_pISpellState);
#else
    lcleanup();
#endif
    if(UT_iconv_isValid (DEREF(m_pISpellState, translate_in) ))
        UT_iconv_close(DEREF(m_pISpellState, translate_in));
    DEREF(m_pISpellState, translate_in) = (UT_iconv_t)-1;
    if(UT_iconv_isValid(DEREF(m_pISpellState, translate_out)))
        UT_iconv_close(DEREF(m_pISpellState, translate_out));
    DEREF(m_pISpellState, translate_out) = (UT_iconv_t)-1;

#if defined(DONT_USE_GLOBALS)
	//TODO: Free the structure?
#endif
}

SpellChecker::SpellCheckResult
ISpellChecker::checkWord(const UT_UCSChar *word16, size_t length)
{
    SpellChecker::SpellCheckResult retVal;
    ichar_t  iWord[INPUTWORDLEN + MAXAFFIXLEN];
    char  word8[INPUTWORDLEN + MAXAFFIXLEN];

    if (!g_bSuccessfulInit)
	{
// was
//        return SpellChecker::LOOKUP_SUCCEEDED;
        return SpellChecker::LOOKUP_FAILED;
	}

    if (!word16 || length >= (INPUTWORDLEN + MAXAFFIXLEN) || length == 0)
        return SpellChecker::LOOKUP_FAILED;

    if(!UT_iconv_isValid(DEREF(m_pISpellState, translate_in)))
    {
        /* copy to 8bit string and null terminate */
        register char *p;
        register size_t x;

        for (x = 0, p = word8; x < length; x++)
            *p++ = (unsigned char)*word16++;
        *p = '\0';
    }
	else
	{
        /* convert to 8bit string and null terminate */
		/* TF CHANGE: Use the right types 
        unsigned int len_in, len_out; 
		*/
		size_t len_in, len_out;
        const char *In = (const char *)word16;
        char *Out = word8;

        len_in = length * 2;
        len_out = sizeof( word8 ) - 1;
        UT_iconv(DEREF(m_pISpellState, translate_in), &In, &len_in, &Out, &len_out);
        *Out = '\0';
    }
    
/*UT_ASSERT(0);*/
    if( !strtoichar(DEREF_FIRST_ARG(m_pISpellState) iWord, word8, sizeof(iWord), 0) )
        retVal = (good(DEREF_FIRST_ARG(m_pISpellState) iWord, 0, 0, 1, 0) == 1 ? SpellChecker::LOOKUP_SUCCEEDED : SpellChecker::LOOKUP_FAILED);
    else
        retVal = SpellChecker::LOOKUP_ERROR;

    return retVal; /* 0 - not found, 1 on found, -1 on error */
}

UT_Vector *
ISpellChecker::suggestWord(const UT_UCSChar *word16, size_t length)
{
  UT_Vector *sgvec = new UT_Vector();
    ichar_t  iWord[INPUTWORDLEN + MAXAFFIXLEN];
    char  word8[INPUTWORDLEN + MAXAFFIXLEN];
    int  c;

    if (!g_bSuccessfulInit) 
        return 0;
    if (!word16 || length >= (INPUTWORDLEN + MAXAFFIXLEN) || length == 0)
        return 0;
    if (!sgvec)
      return 0;

    if(!UT_iconv_isValid(DEREF(m_pISpellState, translate_in)))
    {
        /* copy to 8bit string and null terminate */
        register char *p;
        register size_t x;

        for (x = 0, p = word8; x < length; ++x)
		{
            *p++ = (unsigned char)*word16++;
		}
        *p = '\0';
    }
    else
    {
        /* convert to 8bit string and null terminate */
		/* TF CHANGE: Use the right types
        unsigned int len_in, len_out; 
		*/
        size_t len_in, len_out; 
        const char *In = (const char *)word16;
        char *Out = word8;
        len_in = length * 2;
        len_out = sizeof( word8 ) - 1;
        UT_iconv(DEREF(m_pISpellState, translate_in), &In, &len_in, &Out, &len_out);
        *Out = '\0';
    }
   
    if( !strtoichar(DEREF_FIRST_ARG(m_pISpellState) iWord, word8, sizeof(iWord), 0) )
        makepossibilities(DEREF_FIRST_ARG(m_pISpellState) iWord);

    for (c = 0; c < DEREF(m_pISpellState, pcount); c++) 
    {
        int l;

        l = strlen(DEREF(m_pISpellState, possibilities[c]));

        UT_UCSChar *theWord = (unsigned short*)malloc(sizeof(unsigned short) * l + 2);
        if (theWord == NULL) 
        {
	  // OOM, but return what we have so far
	  return sgvec;
        }

        if (DEREF(m_pISpellState, translate_out) == (iconv_t)-1)
        {
            /* copy to 16bit string and null terminate */
            register int x;

            for (x = 0; x < l; x++)
                theWord[x] = (unsigned char)DEREF(m_pISpellState, possibilities[c][x]);
            theWord[l] = 0;
        }
        else
        {
            /* convert to 16bit string and null terminate */
			/* TF CHANGE: Use the right types
			unsigned int len_in, len_out; 
			*/
			size_t len_in, len_out; 
            const char *In = DEREF(m_pISpellState, possibilities[c]);
            char *Out = (char *)theWord;

            len_in = l;
            len_out = sizeof(unsigned short) * l;
            UT_iconv(DEREF(m_pISpellState, translate_out), &In, &len_in, &Out, &len_out);	    
            *((unsigned short *)Out) = 0;
        }

		sgvec->addItem((void *)theWord);
    }
	return sgvec;
}

typedef struct {
  char * dict;
  char * lang;
} Ispell2Lang_t;

// please try to keep this ordered alphabetically by country-code
static const Ispell2Lang_t m_mapping[] = {
  { "catala.hash",     "ca-ES" },
  { "czech.hash",      "cs-CZ" },
  { "dansk.hash",      "da-DK" },
  { "deutsch.hash",    "de-CH" },
  { "deutsch.hash",    "de-DE" },
  { "deutsch.hash",    "de-AT" },
  { "ellhnika.hash",   "el-GR" },
  { "british.hash",    "en-AU" },
  { "british.hash",    "en-CA" },
  { "british.hash",    "en-GB" },
  { "british.hash",    "en-IE" },
  { "british.hash",    "en-NZ" },
  { "american.hash",   "en-US" },
  { "british.hash",    "en-ZA" },
  { "espanol.hash",    "es-ES" },
  { "francais.hash",   "fr-BE" },
  { "francais.hash",   "fr-CA" },
  { "francais.hash",   "fr-CH" },
  { "francais.hash",   "fr-FR" },
  { "irish.hash",      "ga-IE" },
  { "galician.hash",   "gl-ES" },
  { "italian.hash",    "it-IT" },
  { "mlatin.hash",     "la-IT" },
  { "lietuviu.hash",   "lt-LT" },
  { "nederlands.hash", "nl-NL" },
  { "norsk.hash",      "nb-NO" },
  { "nynorsk.hash",    "nn-NO" },
  { "polish.hash",     "pl-PL" },
  { "portugues.hash",  "pt-PT" },
  { "portugues.hash",  "pt-BR" },
  { "russian.hash",    "ru-RU" },
  { "svenska.hash",    "sv-SE" }
};

static void couldNotLoadDictionary ( const char * szLang )
{
  XAP_App             * pApp   = XAP_App::getApp ();
  XAP_Frame           * pFrame = pApp->getLastFocussedFrame ();

  if ( !pFrame )
    return;

  const XAP_StringSet * pSS    = pApp->getStringSet ();

  char buf[1024]; // evil hardcoded buffer size
  const char * text = pSS->getValue (XAP_STRING_ID_DICTIONARY_CANTLOAD);
  snprintf(buf, 1024, text, szLang);

  pFrame->showMessageBox (buf,
			  XAP_Dialog_MessageBox::b_O,
			  XAP_Dialog_MessageBox::a_OK);
}

bool
ISpellChecker::requestDictionary(const char *szLang)
{
        char *hashname = NULL;

	for (UT_uint32 i = 0; i < (sizeof (m_mapping) / sizeof (m_mapping[0])); i++)
	  {
	    if (!strcmp (szLang, m_mapping[i].lang)) {
	      UT_String hName = XAP_App::getApp()->getAbiSuiteLibDir();
	      hName += "/dictionary/";
	      hName += m_mapping[i].dict;
	      hashname = UT_strdup (hName.c_str());
	      break;
	    }
	  }

	if (hashname == NULL) {
	  couldNotLoadDictionary ( szLang );
	  return false;
	}

#if defined(DONT_USE_GLOBALS)
	m_pISpellState = alloc_ispell_struct();
#endif
    if (linit(DEREF_FIRST_ARG(m_pISpellState) const_cast<char*>(hashname)) < 0)
    {
      couldNotLoadDictionary ( szLang );
      FREEP(hashname);
      return false;
    }

    UT_DEBUGMSG(("DOM: loaded dictionary (%s %s)\n", hashname, szLang));

    g_bSuccessfulInit = true;

    /* Test for utf8 first */
    prefstringchar = findfiletype(DEREF_FIRST_ARG(m_pISpellState) "utf8", 1, deftflag < 0 ? &deftflag : (int *) NULL);
    if (prefstringchar >= 0)
    {
        DEREF(m_pISpellState, translate_in) = UT_iconv_open("utf-8", UCS_2_INTERNAL);
        DEREF(m_pISpellState, translate_out) = UT_iconv_open(UCS_2_INTERNAL, "utf-8");
    }

    /* Test for "latinN" */
    if(!UT_iconv_isValid(DEREF(m_pISpellState, translate_in)))
    {
        char teststring[64];
        int n1;

        /* Look for "altstringtype" names from latin1 to latin15 */
        for(n1 = 1; n1 <= 15; n1++)
        {
            sprintf(teststring, "latin%u", n1);
            prefstringchar = findfiletype(DEREF_FIRST_ARG(m_pISpellState) teststring, 1, deftflag < 0 ? &deftflag : (int *) NULL);
            if (prefstringchar >= 0)
            {
                DEREF(m_pISpellState, translate_in) = UT_iconv_open(teststring, UCS_2_INTERNAL);
                DEREF(m_pISpellState, translate_out) = UT_iconv_open(UCS_2_INTERNAL, teststring);
                break;
            }
        }
    }
    try_autodetect_charset(DEREF_FIRST_ARG(m_pISpellState) const_cast<char*>(hashname));

    /* Test for known "hashname"s */
    if(!UT_iconv_isValid(DEREF(m_pISpellState, translate_in)))
    {
        if( strstr( hashname, "russian.hash" ))
        {
            /* ISO-8859-5, CP1251 or KOI8-R */
            DEREF(m_pISpellState, translate_in) = UT_iconv_open("KOI8-R", UCS_2_INTERNAL);
            DEREF(m_pISpellState, translate_out) = UT_iconv_open(UCS_2_INTERNAL, "KOI8-R");
        }
    }

    /* If nothing found, use latin1 */
    if(!UT_iconv_isValid(DEREF(m_pISpellState, translate_in)))
    {
        DEREF(m_pISpellState, translate_in) = UT_iconv_open("latin1", UCS_2_INTERNAL);
        DEREF(m_pISpellState, translate_out) = UT_iconv_open(UCS_2_INTERNAL, "latin1");
    }

    if (prefstringchar < 0)
        DEREF(m_pISpellState, defdupchar) = 0;
    else
        DEREF(m_pISpellState, defdupchar) = prefstringchar;

    FREEP(hashname);
	return true;
}
