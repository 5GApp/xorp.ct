%{
#define YYSTYPE char*

#include <assert.h>
#include <stdio.h>

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"

#include "conf_tree.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"

/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize bootstacksize
#define yysslim bootsslim
%}

%token UPLEVEL
%token DOWNLEVEL
%token END
%left ASSIGN_OPERATOR
%token BOOL_VALUE
%token UINT_VALUE
%token IPV4_VALUE
%token IPV4NET_VALUE
%token IPV6_VALUE
%token IPV6NET_VALUE
%token MACADDR_VALUE
%token URL_FILE_VALUE
%token URL_FTP_VALUE
%token URL_HTTP_VALUE
%token URL_TFTP_VALUE
%token LITERAL
%token STRING
%token SYNTAX_ERROR


%%

input:		/* empty */
		| definition input
		| emptystatement input
		| syntax_error
		;

definition:	long_nodename nodegroup
		| short_nodename long_nodegroup
		;

short_nodename:	literal { push_path(); }
		;

long_nodename:	literals { push_path(); }
		;

literal:	LITERAL { extend_path($1, NODE_VOID); }
		;

literals:	literals literal
		| literal LITERAL { extend_path($2, NODE_TEXT); }
		| literal BOOL_VALUE { extend_path($2, NODE_BOOL); }
		| literal UINT_VALUE { extend_path($2, NODE_UINT); }
		| literal IPV4_VALUE { extend_path($2, NODE_IPV4); }
		| literal IPV4NET_VALUE { extend_path($2, NODE_IPV4NET); }
		| literal IPV6_VALUE { extend_path($2, NODE_IPV6); }
		| literal IPV6NET_VALUE { extend_path($2, NODE_IPV6NET); }
		| literal MACADDR_VALUE { extend_path($2, NODE_MACADDR); }
		| literal URL_FILE_VALUE { extend_path($2, NODE_URL_FILE); }
		| literal URL_FTP_VALUE { extend_path($2, NODE_URL_FTP); }
		| literal URL_HTTP_VALUE { extend_path($2, NODE_URL_HTTP); }
		| literal URL_TFTP_VALUE { extend_path($2, NODE_URL_TFTP); }
		;

nodegroup:	long_nodegroup
		| END { pop_path(); }
		;

long_nodegroup:	UPLEVEL statements DOWNLEVEL { pop_path(); }
		;

statements:	/* empty string */
		| statement statements
		;

statement:	terminal
		| definition
		| emptystatement
		;

emptystatement:	END
		;

terminal:	LITERAL END {
			terminal($1, strdup(""), NODE_VOID);
		}
		| LITERAL ASSIGN_OPERATOR STRING END {
			terminal($1, $3, NODE_TEXT);
		}
		| LITERAL ASSIGN_OPERATOR BOOL_VALUE END {
			terminal($1, $3, NODE_BOOL);
		}
		| LITERAL ASSIGN_OPERATOR UINT_VALUE END {
			terminal($1, $3, NODE_UINT);
		}
		| LITERAL ASSIGN_OPERATOR IPV4_VALUE END {
			terminal($1, $3, NODE_IPV4);
		}
		| LITERAL ASSIGN_OPERATOR IPV4NET_VALUE END {
			terminal($1, $3, NODE_IPV4NET);
		}
		| LITERAL ASSIGN_OPERATOR IPV6_VALUE END {
			terminal($1, $3, NODE_IPV6);
		}
		| LITERAL ASSIGN_OPERATOR IPV6NET_VALUE END {
			terminal($1, $3, NODE_IPV6NET);
		}
		| LITERAL ASSIGN_OPERATOR MACADDR_VALUE END {
			terminal($1, $3, NODE_MACADDR);
		}
		| LITERAL ASSIGN_OPERATOR URL_FILE_VALUE END {
			terminal($1, $3, NODE_URL_FILE);
		}
		| LITERAL ASSIGN_OPERATOR URL_FTP_VALUE END {
			terminal($1, $3, NODE_URL_FTP);
		}
		| LITERAL ASSIGN_OPERATOR URL_HTTP_VALUE END {
			terminal($1, $3, NODE_URL_HTTP);
		}
		| LITERAL ASSIGN_OPERATOR URL_TFTP_VALUE END {
			terminal($1, $3, NODE_URL_TFTP);
		}
		;

syntax_error:	SYNTAX_ERROR {
			booterror("syntax error");
		}
		;


%%

extern void boot_scan_string(const char *configuration);
extern int boot_linenum;
extern "C" int bootparse();
extern int bootlex();

static ConfigTree *config_tree = NULL;
static string boot_filename;
static string lastsymbol;


static void
extend_path(char *segment, int type)
{
    lastsymbol = segment;

    config_tree->extend_path(string(segment), type);
    free(segment);
}

static void
push_path()
{
    config_tree->push_path();
}

static void
pop_path()
{
    config_tree->pop_path();
}

static void
terminal(char *segment, char *value, int type)
{
    extend_path(segment, type);
    push_path();

    lastsymbol = value;

    config_tree->terminal_value(value, type);
    free(value);
    pop_path();
}

void
booterror(const char *s) throw (ParseError)
{
    string errmsg;

    if (! boot_filename.empty()) {
	errmsg = c_format("PARSE ERROR [Config File %s, line %d]: %s",
			  boot_filename.c_str(),
			  boot_linenum, s);
    } else {
	errmsg = c_format("PARSE ERROR [line %d]: %s", boot_linenum, s);
    }
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());

    xorp_throw(ParseError, errmsg);
}

int
init_bootfile_parser(const char *configuration,
		     const char *filename,
		     ConfigTree *ct)
{
    config_tree = ct;
    boot_filename = filename;
    boot_linenum = 1;
    boot_scan_string(configuration);
    return 0;
}

void
parse_bootfile() throw (ParseError)
{
    if (bootparse() != 0)
	booterror("unknown error");
}
