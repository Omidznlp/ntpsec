
/*
 *  $Id: putshell.c,v 2.26 2004/07/22 02:46:22 bkorb Exp $
 *
 *  This module will interpret the options set in the tOptions
 *  structure and print them to standard out in a fashion that
 *  will allow them to be interpreted by the Bourne or Korn shells.
 */

/*
 *  Automated Options copyright 1992-2004 Bruce Korb
 *
 *  Automated Options is free software.
 *  You may redistribute it and/or modify it under the terms of the
 *  GNU General Public License, as published by the Free Software
 *  Foundation; either version 2, or (at your option) any later version.
 *
 *  Automated Options is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Automated Options.  See the file "COPYING".  If not,
 *  write to:  The Free Software Foundation, Inc.,
 *             59 Temple Place - Suite 330,
 *             Boston,  MA  02111-1307, USA.
 *
 * As a special exception, Bruce Korb gives permission for additional
 * uses of the text contained in his release of AutoOpts.
 *
 * The exception is that, if you link the AutoOpts library with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the AutoOpts library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by Bruce Korb under
 * the name AutoOpts.  If you copy code from other sources under the
 * General Public License into a copy of AutoOpts, as the General Public
 * License permits, the exception does not apply to the code that you add
 * in this way.  To avoid misleading anyone as to the status of such
 * modified files, you must delete this exception notice from them.
 *
 * If you write modifications of your own for AutoOpts, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 */

/*
 *  Make sure embedded single quotes come out okay.  The initial quote has
 *  been emitted and the closing quote will be upon return.
 */
STATIC void
putQuotedStr( tCC* pzStr )
{
    /*
     *  Handle empty strings to make the rese of the logic simpler.
     */
    if ((pzStr == NULL) || (*pzStr == NUL)) {
        fputs( "''", stdout );
        return;
    }

    /*
     *  Emit any single quotes/apostrophes at the start of the string and
     *  bail if that is all we need to do.
     */
    while (*pzStr == '\'') {
        fputs( "\\'", stdout );
        pzStr++;
    }
    if (*pzStr == NUL)
        return;

    /*
     *  Start the single quote string
     */
    fputc( '\'', stdout );
    for (;;) {
        tCC* pz = strchr( pzStr, '\'' );
        if (pz == NULL)
            break;

        /*
         *  Emit the string up to the single quote (apostrophe) we just found.
         */
        fwrite( pzStr, (pz - pzStr), 1, stdout );
        fputc( '\'', stdout );
        pzStr = pz;

        /*
         *  Emit an escaped apostrophe for every one we find.
         *  If that ends the string, do not re-open the single quotes.
         */
        while (*++pzStr == '\'')   fputs( "\\'", stdout );
        if (*pzStr == NUL)
            return;

        fputc( '\'', stdout );
    }

    /*
     *  If we broke out of the loop, we must still emit the remaining text
     *  and then close the single quote string.
     */
    fputs( pzStr, stdout );
    fputc( '\'', stdout );
}


/*=export_func  putBourneShell
 * what:  write a portable shell script to parse options
 * private:
 * arg:   tOptions*, pOpts, the program options descriptor
 * doc:   This routine will emit portable shell script text for parsing
 *        the options described in the option definitions.
=*/
void
putBourneShell( tOptions* pOpts )
{
    int  optIx = 0;
    tSCC zOptCtFmt[]  = "OPTION_CT=%d\nexport OPTION_CT\n";
    tSCC zOptNumFmt[] = "%1$s_%2$s=%3$d # 0x%3$X\nexport %1$s_%2$s\n";
    tSCC zOptDisabl[] = "%1$s_%2$s=%3$s\nexport %1$s_%2$s\n";
    tSCC zOptValFmt[] = "%s_%s=";
    tSCC zOptEnd[]    = "\nexport %s_%s\n";
    tSCC zFullOptFmt[]= "%1$s_%2$s='%3$s'\nexport %1$s_%2$s\n";
    tSCC zEquivMode[] = "%1$s_%2$s_MODE='%3$s'\nexport %1$s_%2$s_MODE\n";

    printf( zOptCtFmt, pOpts->curOptIdx-1 );

    do  {
        tOptDesc* pOD = pOpts->pOptDesc + optIx;

        if (SKIP_OPT(pOD))
            continue;

        /*
         *  Equivalence classes are hard to deal with.  Where the
         *  option data wind up kind of squishes around.  For the purposes
         *  of emitting shell state, they are not recommended, but we'll
         *  do something.  I guess we'll emit the equivalenced-to option
         *  at the point in time when the base option is found.
         */
        if (pOD->optEquivIndex != NO_EQUIVALENT)
            continue; /* equivalence to a different option */

        /*
         *  Equivalenced to a different option.  Process the current option
         *  as the equivalenced-to option.  Keep the persistent state bits,
         *  but copy over the set-state bits.
         */
        if (pOD->optActualIndex != optIx) {
            tOptDesc* p = pOpts->pOptDesc + pOD->optActualIndex;
            p->pzLastArg = pOD->pzLastArg;
            p->fOptState &= OPTST_PERSISTENT;
            p->fOptState |= pOD->fOptState & ~OPTST_PERSISTENT;
            printf( zEquivMode, pOpts->pzPROGNAME, pOD->pz_NAME, p->pz_NAME );
            pOD = p;
        }

        /*
         *  If the argument type is a set membership bitmask, then we always
         *  emit the thing.  We do this because it will always have some sort
         *  of bitmask value and we need to emit the bit values.
         */
        if ((pOD->fOptState & OPTST_MEMBER_BITS) != 0) {
            char* pz;
            uintptr_t val = 1;
            printf( zOptNumFmt, pOpts->pzPROGNAME, pOD->pz_NAME,
                    (uintptr_t)(pOD->optCookie) );
            pOD->optCookie = (void*)(uintptr_t)~0UL;
            (*(pOD->pOptProc))( (tOptions*)2UL, pOD );

            /*
             *  We are building the typeset list.  The list returned starts with
             *  'none + ' for use by option saving stuff.  We must ignore that.
             */
            pz = (char*)pOD->pzLastArg + 7;
            while (*pz != NUL) {
                printf( "typeset -x -i %s_", pOD->pz_NAME );
                pz += strspn( pz, " +\t\n\f" );
                for (;;) {
                    char ch = *(pz++);
                         if (islower( ch ))  fputc( toupper( ch ), stdout );
                    else if (isalnum( ch ))  fputc( ch, stdout );
                    else if (isspace( ch )
                          || (ch == '+'))    goto name_done;
                    else if (ch == NUL)      { pz--; goto name_done; }
                    else fputc( '_', stdout );
                } name_done:;
                printf( "=%1$d # 0x%1$X\n", val );
                val <<= 1;
            }
            free( (void*)(pOD->pzLastArg) );
            continue;
        }

        /*
         *  IF the option was either specified or it wakes up enabled,
         *  then we will emit information.  Otherwise, skip it.
         *  The idea is that if someone defines an option to initialize
         *  enabled, we should tell our shell script that it is enabled.
         */
        if (UNUSED_OPT( pOD ) && DISABLED_OPT( pOD ))
            continue;

        /*
         *  Handle stacked arguments
         */
        if (  (pOD->fOptState & OPTST_STACKED)
           && (pOD->optCookie != NULL) )  {
            tSCC zOptCookieCt[] = "%1$s_%2$s_CT=%3$d\nexport %1$s_%2$s_CT\n";

            tArgList*    pAL = (tArgList*)pOD->optCookie;
            tCC**        ppz = pAL->apzArgs;
            int          ct  = pAL->useCt;

            printf( zOptCookieCt, pOpts->pzPROGNAME, pOD->pz_NAME, ct );

            while (--ct >= 0) {
                tSCC zOptNumArg[] = "%s_%s_%d=";
                tSCC zOptEnd[]    = "\nexport %s_%s_%d\n";

                printf( zOptNumArg, pOpts->pzPROGNAME, pOD->pz_NAME,
                        pAL->useCt - ct );
                putQuotedStr( *(ppz++) );
                printf( zOptEnd, pOpts->pzPROGNAME, pOD->pz_NAME,
                        pAL->useCt - ct );
            }
        }

        /*
         *  If the argument has been disabled,
         *  Then set its value to the disablement string
         */
        else if ((pOD->fOptState & OPTST_DISABLED) != 0)
            printf( zOptDisabl, pOpts->pzPROGNAME, pOD->pz_NAME,
                    (pOD->pz_DisablePfx != NULL)
                    ? pOD->pz_DisablePfx : "false" );

        /*
         *  If the argument type is numeric, the last arg pointer
         *  is really the VALUE of the string that was pointed to.
         */
        else if ((pOD->fOptState & OPTST_NUMERIC) != 0)
            printf( zOptNumFmt, pOpts->pzPROGNAME, pOD->pz_NAME,
                    (uintptr_t)(pOD->pzLastArg) );

        /*
         *  If the argument type is an enumeration, then it is much
         *  like a text value, except we call the callback function
         *  to emit the value corresponding to the "pzLastArg" number.
         */
        else if ((pOD->fOptState & OPTST_ENUMERATION) != 0) {
            printf( zOptValFmt, pOpts->pzPROGNAME, pOD->pz_NAME );
            fputc( '\'', stdout );
            (*(pOD->pOptProc))( (tOptions*)1UL, pOD );
            fputc( '\'', stdout );
            printf( zOptEnd, pOpts->pzPROGNAME, pOD->pz_NAME );
        }

        /*
         *  If the argument type is numeric, the last arg pointer
         *  is really the VALUE of the string that was pointed to.
         */
        else if ((pOD->fOptState & OPTST_BOOLEAN) != 0)
            printf( zFullOptFmt, pOpts->pzPROGNAME, pOD->pz_NAME,
                    ((uintptr_t)(pOD->pzLastArg) == 0) ? "false" : "true" );

        /*
         *  IF the option has an empty value,
         *  THEN we set the argument to the occurrence count.
         */
        else if (  (pOD->pzLastArg == NULL)
                || (pOD->pzLastArg[0] == NUL) )

            printf( zOptNumFmt, pOpts->pzPROGNAME, pOD->pz_NAME,
                    (int)pOD->optOccCt );

        /*
         *  This option has a text value
         */
        else {
            printf( zOptValFmt, pOpts->pzPROGNAME, pOD->pz_NAME );
            putQuotedStr( pOD->pzLastArg );
            printf( zOptEnd, pOpts->pzPROGNAME, pOD->pz_NAME );
        }
    } while (++optIx < pOpts->presetOptCt );

    if (  ((pOpts->fOptSet & OPTPROC_REORDER) != 0)
       && (pOpts->curOptIdx < pOpts->origArgCt)) {
        fputs( "set --", stdout );
        for (optIx = pOpts->curOptIdx; optIx < pOpts->origArgCt; optIx++) {
            char* pzArg = pOpts->origArgVect[ optIx ];
            if (strchr( pzArg, '\'' ) == NULL)
                printf( " '%s'", pzArg );
            else {
                fputs( " '", stdout );
                for (;;) {
                    char ch = *(pzArg++);
                    switch (ch) {
                    case '\'':  fputs( "'\\''", stdout ); break;
                    case NUL:   goto arg_done;
                    default:    fputc( ch, stdout ); break;
                    }
                } arg_done:;
                fputc( '\'', stdout );
            }
        }
        fputs( "\nOPTION_CT=0\n", stdout );
    }
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 * end of autoopts/putshell.c */
