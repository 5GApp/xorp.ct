%{
#include "libxorp/xorp.h"
#include <string.h>
#include "y.opcmd_tab.h"
%}
	int opcmd_linenum = 1;
	extern char* opcmdlval;
	string opcmd_parsebuf;
%option noyywrap
%option nounput
%option never-interactive
%x comment
%x string


%%

"{"	{
	return UPLEVEL;
	}

"}"	{
	return DOWNLEVEL;
	}

[ \t]+	/* whitespace */

"\n"	{
	/* newline is not significant */
	opcmd_linenum++;
	}

";"	{
	return END;
	}

":"	{
	return COLON;
	}

\<[a-zA-Z0-9\-_ \t]*\>	{
	opcmdlval = strdup(opcmdtext);
	return WILDCARD;
        }

"%module"	{
	return CMD_MODULE;
	}

"%command"	{
	return CMD_COMMAND;
	}

"%help"	{
	return CMD_HELP;
	}

"%opt_parameter"	{
	return CMD_OPT_PARAMETER;
	}

"%tag"	{
	return CMD_TAG;
	}

"%nomore_mode"	{
	return CMD_NOMORE_MODE;
	}

\$\([a-zA-Z@][a-zA-Z0-9\-_\.@\*]*\)	{
	opcmdlval = strdup(opcmdtext);
	return VARIABLE;
	}

[a-zA-Z0-9_/\.][a-zA-Z0-9\-_/\.]*	{
	/*
	 * Note that we explicitly allow a literal to start with not only
	 * by a letter and '/', but a digit, '_' or '.' .
	 * Also, allow '.' to be part of the literal elsewhere.
	 * Thus, we can specify more liberally a filename (e.g., now a filename
	 * can start with a digit, it can contain dots, etc).
	 */
	opcmdlval = strdup(opcmdtext);
	return LITERAL;
	}

\"			{
			BEGIN(string);
			/* XXX: include the original quote */
			opcmd_parsebuf="\"";
			}

<string>[^\\\n\"]*	/* normal text */ {
			opcmd_parsebuf += opcmdtext;
			}

<string>\\+\"		/* allow quoted quotes */ {
			opcmd_parsebuf += "\"";
			}

<string>\\+\\		/* allow quoted backslash */ {
			opcmd_parsebuf += "\\";
			}

<string>\n		/* allow unquoted newlines */ {
			opcmd_linenum++;
			opcmd_parsebuf += "\n";
			}

<string>\\+n		/* allow C-style quoted newlines */ {
			/* XXX: don't increment the line number */
			opcmd_parsebuf += "\n";
			}

<string>\"		{
			BEGIN(INITIAL);
			/* XXX: include the original quote */
			opcmd_parsebuf += "\"";
			opcmdlval = strdup(opcmd_parsebuf.c_str());
			return STRING;
			}

"/*"			BEGIN(comment);

<comment>[^*\n]* 	/* eat up anything that's not a '*' */

<comment>"*"+[^*/\n]* 	/* eat up '*'s not followed by "/"s */

<comment>\n		opcmd_linenum++;

<comment>"*"+"/"	BEGIN(INITIAL);

.	{
	/* everything else is a syntax error */
	return SYNTAX_ERROR;
	}


%%
