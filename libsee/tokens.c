/*
 * Copyright (c) 2003
 *      David Leonard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of David Leonard nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* $Id: tokens.c 1282 2007-09-13 22:26:35Z d $ */

/*
 * Token tables for the lexical analyser.
 * 
 * The textual form of keywords and punctuators are
 * kept here.  See lex.c for how the tables are used.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include <see/type.h>
#include <see/value.h>
#include <see/string.h>

#include "stringdefs.h"
#include "lex.h"
#include "tokens.h"

#define lengthof(a) (sizeof a / sizeof a[0])

struct strtoken SEE_tok_keywords[] = {
	{ STRi(synchronized), tRESERVED },
	{ STRi(eleven_filler), tRESERVED },	/* hack */
	{ STRi(implements), tRESERVED },
	{ STRi(instanceof), tINSTANCEOF },
	{ STRi(transient), tRESERVED },
	{ STRi(protected), tRESERVED },
	{ STRi(interface), tRESERVED },
	{ STRi(volatile), tRESERVED },
	{ STRi(debugger), tRESERVED },
	{ STRi(function), tFUNCTION },
	{ STRi(continue), tCONTINUE },
	{ STRi(abstract), tRESERVED },
	{ STRi(private), tRESERVED },
	{ STRi(package), tRESERVED },
	{ STRi(extends), tRESERVED },
	{ STRi(boolean), tRESERVED },
	{ STRi(finally), tFINALLY },
	{ STRi(default), tDEFAULT },
	{ STRi(native), tRESERVED },
	{ STRi(export), tRESERVED },
	{ STRi(typeof), tTYPEOF },
	{ STRi(switch), tSWITCH },
	{ STRi(return), tRETURN },
	{ STRi(throws), tRESERVED },
	{ STRi(import), tRESERVED },
	{ STRi(static), tRESERVED },
	{ STRi(delete), tDELETE },
	{ STRi(public), tRESERVED },
	{ STRi(double), tRESERVED },
	{ STRi(float), tRESERVED },
	{ STRi(super), tRESERVED },
	{ STRi(short), tRESERVED },
	{ STRi(const), tRESERVED },
	{ STRi(class), tRESERVED },
	{ STRi(while), tWHILE },
	{ STRi(final), tRESERVED },
	{ STRi(throw), tTHROW },
	{ STRi(catch), tCATCH },
	{ STRi(break), tBREAK },
	{ STRi(false), tFALSE },
	{ STRi(with), tWITH },
	{ STRi(long), tRESERVED },
	{ STRi(null), tNULL },
	{ STRi(true), tTRUE },
	{ STRi(void), tVOID },
	{ STRi(else), tELSE },
	{ STRi(goto), tRESERVED },
	{ STRi(enum), tRESERVED },
	{ STRi(this), tTHIS },
	{ STRi(byte), tRESERVED },
	{ STRi(case), tCASE },
	{ STRi(char), tRESERVED },
	{ STRi(new), tNEW },
	{ STRi(try), tTRY },
	{ STRi(int), tRESERVED },
	{ STRi(for), tFOR },
	{ STRi(var), tVAR },
	{ STRi(in), tIN },
	{ STRi(do), tDO },
	{ STRi(if), tIF },
};
int SEE_tok_nkeywords = lengthof(SEE_tok_keywords);

static struct token operators1[] = {
	{ {'?'}, '?' },
	{ {'{'}, '{' },
	{ {'}'}, '}' },
	{ {'('}, '(' },
	{ {')'}, ')' },
	{ {'['}, '[' },
	{ {']'}, ']' },
	{ {'.'}, '.' },
	{ {';'}, ';' },
	{ {','}, ',' },
	{ {'<'}, '<' },
	{ {'>'}, '>' },
	{ {':'}, ':' },
	{ {'='}, '=' },
	{ {'|'}, '|' },
	{ {'^'}, '^' },
	{ {'!'}, '!' },
	{ {'~'}, '~' },
	{ {'*'}, '*' },
	{ {'-'}, '-' },
	{ {'+'}, '+' },
	{ {'%'}, '%' },
	{ {'&'}, '&' },
	{ {0}, 0 }
}, operators2[] = {
	{ {'-','-'}, tMINUSMINUS },
	{ {'<','<'}, tLSHIFT },
	{ {'>','>'}, tRSHIFT },
	{ {'&','&'}, tANDAND },
	{ {'|','|'}, tOROR },
	{ {'+','='}, tPLUSEQ },
	{ {'-','='}, tMINUSEQ },
	{ {'*','='}, tSTAREQ },
	{ {'%','='}, tMODEQ },
	{ {'&','='}, tANDEQ },
	{ {'|','='}, tOREQ },
	{ {'^','='}, tXOREQ },
	{ {'<','='}, tLE },
	{ {'>','='}, tGE },
	{ {'=','='}, tEQ },
	{ {'!','='}, tNE },
	{ {'+','+'}, tPLUSPLUS },
	{ {0}, 0 }
}, operators3[] = {
	{ {'>','>','='}, tRSHIFTEQ },
	{ {'<','<','='}, tLSHIFTEQ },
	{ {'>','>','>'}, tURSHIFT },
	{ {'=','=','='}, tSEQ },
	{ {'!','=','='}, tSNE },
	{ {'-','-','>'}, tSGMLCOMMENTEND },
	{ {0}, 0 }
}, operators4[] = {
	{ {'<','!','-','-'}, tSGMLCOMMENT },
	{ {'>','>','>','='}, tURSHIFTEQ },
	{ {0}, 0 }
};

struct token *SEE_tok_operators[] = {
	NULL, operators1, operators2, operators3, operators4
};
int SEE_tok_noperators = lengthof(SEE_tok_operators);


static struct {
	const char *name;
	int token;
} tok_names[] = {
	{ "end of file",	tEND },
	{ "a comment",		tCOMMENT },
	{ "a line break",	tLINETERMINATOR },
	{ "'/='",		tDIVEQ },
	{ "'/'",		tDIV },
	{ "a number",		tNUMBER },
	{ "a string",		tSTRING },
	{ "an identifier",	tIDENT },
	{ "a regular expression",tREGEX },
	{ "a reserved word",	tRESERVED },
	{ "'instanceof'",	tINSTANCEOF },
	{ "'function'",		tFUNCTION },
	{ "'continue'",		tCONTINUE },
	{ "'finally'",		tFINALLY },
	{ "'default'",		tDEFAULT },
	{ "'typeof'",		tTYPEOF },
	{ "'switch'",		tSWITCH },
	{ "'return'",		tRETURN },
	{ "'delete'",		tDELETE },
	{ "'while'",		tWHILE },
	{ "'throw'",		tTHROW },
	{ "'catch'",		tCATCH },
	{ "'break'",		tBREAK },
	{ "'with'",		tWITH },
	{ "'void'",		tVOID },
	{ "'else'",		tELSE },
	{ "'this'",		tTHIS },
	{ "'case'",		tCASE },
	{ "'new'",		tNEW },
	{ "'try'",		tTRY },
	{ "'for'",		tFOR },
	{ "'var'",		tVAR },
	{ "'in'",		tIN },
	{ "'do'",		tDO },
	{ "'if'",		tIF },
	{ "'>>>='",		tURSHIFTEQ },
	{ "'<!--'",		tSGMLCOMMENT },
	{ "'-->'",		tSGMLCOMMENTEND },
	{ "'>>='",		tRSHIFTEQ },
	{ "'<<='",		tLSHIFTEQ },
	{ "'>>>'",		tURSHIFT },
	{ "'==='",		tSEQ },
	{ "'!=='",		tSNE },
	{ "'--'",		tMINUSMINUS },
	{ "'<<'",		tLSHIFT },
	{ "'>>'",		tRSHIFT },
	{ "'&&'",		tANDAND },
	{ "'||'",		tOROR },
	{ "'+='",		tPLUSEQ },
	{ "'-='",		tMINUSEQ },
	{ "'*='",		tSTAREQ },
	{ "'%='",		tMODEQ },
	{ "'&='",		tANDEQ },
	{ "'|='",		tOREQ },
	{ "'^='",		tXOREQ },
	{ "'<='",		tLE },
	{ "'>='",		tGE },
	{ "'=='",		tEQ },
	{ "'!='",		tNE },
	{ "'++'",		tPLUSPLUS },
	{ "'true'",		tTRUE },
	{ "'false'",		tFALSE },
	{ "'null'",		tNULL }
};

const char *
SEE_tokenname(token)
	int token;
{
	static char buf[30];

	SEE_tokenname_buf(token, buf, sizeof buf);
	return buf;
}

void
SEE_tokenname_buf(token, buf, buflen)
	int token;
	char *buf;
	int buflen;
{
	int i;
	char tokch[4];
	const char *name;
        int namelen;

	for (i = 0; i < lengthof(tok_names); i++)
		if (tok_names[i].token == token) {
		    name = tok_names[i].name;
		    break;
		}
	else if (token >= ' ' && token <= '~') {
		tokch[0] = '\'';
		tokch[1] = (unsigned char)token;
		tokch[2] = '\'';
		tokch[3] = 0;
		name = tokch;
	} else 
		name = "<bad token>";
	namelen = strlen(name);
	if (namelen > buflen - 1)
		namelen = buflen - 1;
        memcpy(buf, name, namelen);
	buf[namelen] = 0;
        return;
}
