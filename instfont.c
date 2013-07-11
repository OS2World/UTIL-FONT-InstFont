/*****************************************************************************
 * instfont.c                                                                *
 *                                                                           *
 * To build with ICC: icc.exe /Ss instfont.c                                 *
 * To build with GCC: gcc.exe instfont.c                                     *
 *                                                                           *
 * INSTFONT is (C) 2010 Alex Taylor.                                         *
 *                                                                           *
 * Redistribution and use in source and binary forms, with or without        *
 * modification, are permitted provided that the following conditions are    *
 * met:                                                                      *
 *                                                                           *
 * 1. Redistributions of source code must retain the above copyright notice, *
 *    this list of conditions and the following disclaimer.                  *
 * 2. Redistributions in binary form must reproduce the above copyright      *
 *    notice, this list of conditions and the following disclaimer in the    *
 *    documentation and/or other materials provided with the distribution.   *
 * 3. The name of the author may not be used to endorse or promote products  *
 *    derived from this software without specific prior written permission.  *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR      *
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.   *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,          *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT  *
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF  *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.         *
 *                                                                           *
 *****************************************************************************/

#define INCL_GPI
#define INCL_WIN
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void MorphOutput( PSZ psz, ... );

HAB      hab;                       // anchor-block handle
PPIB     ppib;                      // process info block pointer
ULONG    ulPType;                   // original process type


/* ------------------------------------------------------------------------- */
int main ( int argc, char *argv[] )
{
    HMQ    hmq;                     // PM message-queue handle
    PTIB   ptib;                    // thread info block pointer
    ULONG  ulLen;                   // font key length
    BOOL   fOK;
    PSZ    psz;
    CHAR   szFontFile[ CCHMAXPATH ],
           szFontKey[ CCHMAXPATH ];
    APIRET rc;                      // return code


    // Get input options
    if ( argc < 2 ) {
        printf("%s - Install a font in Presentation Manager.\n", strupr(argv[0]) );
        printf("Syntax: %s <font filename>\n", argv[0] );
        return 0;
    }
    strupr( argv[1] );

    // Determine the fully-qualified name
    rc = DosQueryPathInfo( argv[1], FIL_QUERYFULLNAME, (PVOID) szFontFile, CCHMAXPATH );
    if ( rc ) {
        printf("Failed to resolve filename %s (RC=%u)\n", argv[1], rc );
        return rc;
    }

    // Get the filename-only portion
    psz = strrchr( szFontFile, '\\');
    if ( !psz ) {
        psz = strchr( szFontFile, ':');
        if ( !psz ) psz = szFontFile;
        else psz++;
    }
    else psz++;
    strncpy( szFontKey, psz, sizeof(szFontKey) );

    // If the font filename ends in .FON, strip it off
    ulLen = strlen( szFontKey );
    psz = szFontKey + (ulLen - 4);
    if ( stricmp( psz, ".FON") == 0 ) *psz = 0;

    // Initialize PM
    DosGetInfoBlocks( &ptib, &ppib );
    ulPType = ppib->pib_ultype;
    ppib->pib_ultype = 3;

    hab = WinInitialize( 0 );
    WinCreateMsgQueue( hab, 0 );

    fOK = GpiLoadPublicFonts( hab, szFontFile );
    if ( !fOK )
        MorphOutput("Failed to register font: GpiLoadPublicFonts() failed.\n");
    else {
        fOK = PrfWriteProfileString( HINI_USERPROFILE, "PM_Fonts", szFontKey, szFontFile );
        if ( !fOK )
            MorphOutput("Failed to register font: PrfWriteProfileString() failed.\n");
    }

    // Clean up and exit
    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );
    ppib->pib_ultype = ulPType;

    return fOK;
}



/* ------------------------------------------------------------------------- *
 * MorphOutput                                                               *
 *                                                                           *
 * This is basically a wrapper around printf() which "morphs" the program    *
 * type to VIO (text mode) first, then back again afterwards, in order to    *
 * allow console output when the program type is PM (GUI mode).              *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *     PSZ  psz: The string to be printed                                    *
 *     ...     : printf() format string                                      *
 *                                                                           *
 * RETURNS: N/A                                                              *
 * ------------------------------------------------------------------------- */
void MorphOutput( PSZ psz, ... )
{
    va_list ap;

    ppib->pib_ultype = ulPType;

    va_start( ap, psz );
    vprintf( psz, ap );
    va_end( ap );

    ppib->pib_ultype = 3;
}

