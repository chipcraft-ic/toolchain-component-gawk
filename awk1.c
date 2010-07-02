/*
 * awk1 -- Expression tree constructors and main program for gawk. 
 *
 * Copyright (C) 1986 Free Software Foundation Written by Paul Rubin, August
 * 1986 
 *
 * $Log:	awk1.c,v $
 * Revision 1.34  89/03/24  15:57:21  david
 * hashnode changes to NODE
 * 
 * Revision 1.33  89/03/21  10:58:26  david
 * cleanup and movement of code to awk.y
 * 
 * Revision 1.32  89/03/15  22:00:35  david
 * old case stuff removed
 * new case stuff added
 * add new escape sequences
 * 
 * Revision 1.31  89/03/15  21:31:10  david
 * purge obstack stuff
 * 
 * Revision 1.30  88/12/15  12:56:18  david
 * changes from Jay to compile under gcc and fixing a bug in treatment of
 * input files
 * 
 * Revision 1.29  88/12/08  15:57:41  david
 * *** empty log message ***
 * 
 * Revision 1.28  88/12/07  20:00:15  david
 * changes for incorporating source filename into error messages
 * 
 * Revision 1.27  88/12/01  15:05:26  david
 * changes to allow source line number printing in error messages
 * 
 * Revision 1.26  88/11/28  20:12:30  david
 * unbuffer stdout if compiled with DEBUG
 * 
 * Revision 1.25  88/11/23  21:39:57  david
 * Arnold: set strict if invoked as "awk"
 * 
 * Revision 1.24  88/11/22  13:47:40  david
 * Arnold: changes for case-insensitive matching
 * 
 * Revision 1.23  88/11/15  10:18:57  david
 * Arnold: cleanup; allow multiple -f options; if invoked as awk disable
 * -v option for compatability
 * 
 * Revision 1.22  88/11/14  21:54:27  david
 * Arnold: cleanup
 * 
 * Revision 1.21  88/11/03  15:22:22  david
 * revised flags
 * 
 * Revision 1.20  88/11/01  11:47:24  david
 * mostly cleanup and code movement
 * 
 * Revision 1.19  88/10/19  21:56:00  david
 * replace malloc with emalloc
 * 
 * Revision 1.18  88/10/17  20:57:19  david
 * Arnold: purge FAST
 * 
 * Revision 1.17  88/10/14  22:11:24  david
 * pathc from Hack to get gcc to work
 * 
 * Revision 1.16  88/10/13  21:55:08  david
 * purge FAST; clean up error messages
 * 
 * Revision 1.15  88/10/06  21:55:20  david
 * be more careful about I/O errors on exit
 * Arnold's fixes for command line processing
 * 
 * Revision 1.14  88/10/06  15:52:24  david
 * changes from Arnold: use getopt; ifdef -v option; change semantics of = args.
 * 
 * Revision 1.13  88/09/26  10:16:35  david
 * cleanup from Arnold
 * 
 * Revision 1.12  88/09/19  20:38:29  david
 * added -v option
 * set FILENAME to "-" if stdin
 * 
 * Revision 1.11  88/08/09  14:49:23  david
 * reorganized handling of command-line arguments and files
 * 
 * Revision 1.10  88/06/13  18:08:25  david
 * delete \a and -R options
 * separate exit value from flag indicating that exit has been called
 * [from Arnold]
 * 
 * Revision 1.9  88/06/05  22:19:41  david
 * func_level goes away; param_counter is used to count order of local vars.
 * 
 * Revision 1.8  88/05/31  09:21:56  david
 * Arnold's portability changes (vprintf)
 * fixed handling of function parameter use inside function
 * 
 * Revision 1.7  88/05/30  09:51:17  david
 * exit after yyparse() if any errors were encountered
 * mk_re_parse() parses C escapes in regexps.
 * do panic() properly with varargs
 * clean up and simplify pop_var()
 * 
 * Revision 1.6  88/05/26  22:46:19  david
 * minor changes: break out separate case for making regular expressions
 *   from parser vs. from strings
 * 
 * Revision 1.5  88/05/13  22:02:31  david
 * moved BEGIN and END block merging into parse-phase (sorry Arnold)
 * if there is only a BEGIN block and nothing else, don't read any files
 * cleaned up func_install a bit
 * 
 * Revision 1.4  88/05/04  12:18:28  david
 * make_for_loop() now returns a NODE *
 * pop_var() now returns the value of the node being popped, to be used
 * in func_call() if the variable is an array (call by reference)
 * 
 * Revision 1.3  88/04/15  13:12:19  david
 * small fix to arg reading code
 * 
 * Revision 1.2  88/04/14  14:40:25  david
 * Arnold's changes to read program from a file
 * 
 * Revision 1.1  88/04/08  15:14:52  david
 * Initial revision
 *  Revision 1.6  88/04/08  14:48:30  david changes from
 * Arnold Robbins 
 *
 * Revision 1.5  88/03/28  14:13:33  david *** empty log message *** 
 *
 * Revision 1.4  88/03/23  22:17:33  david mostly delinting -- a couple of bug
 * fixes 
 *
 * Revision 1.3  88/03/18  21:00:09  david Baseline -- hoefully all the
 * functionality of the new awk added. Just debugging and tuning to do. 
 *
 * Revision 1.2  87/11/19  14:40:17  david added support for user-defined
 * functions 
 *
 * Revision 1.1  87/10/27  15:23:26  david Initial revision 
 *
 */

/*
 * GAWK is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY.  No author or distributor accepts responsibility to anyone for
 * the consequences of using it or for whether it serves any particular
 * purpose or works at all, unless he says so in writing. Refer to the GAWK
 * General Public License for full details. 
 *
 * Everyone is granted permission to copy, modify and redistribute GAWK, but
 * only under the conditions described in the GAWK General Public License.  A
 * copy of this license is supposed to have been given to you along with GAWK
 * so you can know your rights and responsibilities.  It should be in a file
 * named COPYING.  Among other things, the copyright notice and this notice
 * must be preserved on all copies. 
 *
 * In other words, go ahead and share GAWK, but don't try to stop anyone else
 * from sharing it farther.  Help stamp out software hoarding! 
 */

#include "awk.h"

extern int yyparse();
extern void init_vars();
extern void set_fs();
extern int close_io();
extern void init_args();
extern void init_fields();
extern int inrec();
extern int getopt();
extern int re_set_syntax();
extern NODE *node();

static void do_file();
static void usage();

NODE *variables[HASHSIZE];

/*
 * The parse tree and field nodes are stored here.  Parse_end is a dummy item
 * used to free up unneeded fields without freeing the program being run 
 */
int errcount = 0;	/* error counter, used by yyerror() */
int param_counter;

/* The global null string */
NODE *Nnull_string;

/* The special variable that contains the name of the current input file */
extern NODE *ARGC_node;
extern NODE *ARGV_node;

/* The name the program was invoked under, for error messages */
char *myname;

/* A block of AWK code to be run before running the program */
NODE *begin_block = 0;

/* A block of AWK code to be run after the last input file */
NODE *end_block = 0;

FILE *input_file;		/* Where to read from */

int exiting = 0;		/* Was an "exit" statement executed? */
int exit_val = 0;		/* optional exit value */

#ifdef DEBUG
/* non-zero means in debugging is enabled.  Probably not very useful */
int debugging = 0;

#endif

int tempsource = 0;		/* source is in a temp file */
char **sourcefile = NULL;	/* source file name(s) */
int numfiles = -1;		/* how many source files */

int strict = 0;			/* turn off gnu extensions */

NODE *expression_value;

main(argc, argv)
int argc;
char **argv;
{
#ifdef DEBUG
	/* Print out the parse tree.   For debugging */
	register int dotree = 0;
	extern int yydebug;

#endif
	extern char *version_string;
	FILE *fp;
	static char template[] = "/tmp/gawk.XXXXX";
	int c;
	extern int opterr, optind;
	extern char *optarg;
	char *cp;
 	extern char *rindex();
 	extern char *mktemp();
	/*
	 * for strict to work, legal options must be first
	 */
#define EXTENSIONS	5	/* where to clear */
#ifdef DEBUG
	char *awk_opts = "F:f:vVdD";
#else
	char *awk_opts = "F:f:vV";
#endif

#ifdef DEBUG
	/*malloc_debug(2);*/
#endif
	myname = argv[0];
	if (argc < 2)
		usage();

	/* Tell the regex routines how they should work. . . */
	(void) re_set_syntax(RE_SYNTAX_AWK);

	/* initialize the null string */
	Nnull_string = make_string("", 0);
	Nnull_string->numbr = 0.0;
	Nnull_string->type = Node_val;
	Nnull_string->flags = (PERM|STR|NUM);

	/* Set up the special variables */

	/*
	 * Note that this must be done BEFORE arg parsing else -F
	 * breaks horribly 
	 */
	init_vars();

	/* worst case */
	emalloc(sourcefile, char **, argc * sizeof(char *), "main");


#ifdef STRICT	/* strict Unix awk compatibility */
	strict = 1;
#else
	/* if invoked as 'awk', also behave strictly */
	if ((cp = rindex(myname, '/')) != NULL)
		cp++;
	else
		cp = myname;
	if (STREQ(cp, "awk"))
		strict = 1;
#endif

	if (strict)
		awk_opts[EXTENSIONS] = '\0';

	while ((c = getopt (argc, argv, awk_opts)) != EOF) {
		switch (c) {
#ifdef DEBUG
		case 'd':
			debugging++;
			dotree++;
			break;

		case 'D':
			debugging++;
			yydebug = 2;
			break;
#endif

		case 'F':
			set_fs(optarg);
			break;

		case 'f':
			/*
			 * a la MKS awk, allow multiple -f options.
			 * this makes function libraries real easy.
			 * most of the magic is in the scanner.
			 */
			sourcefile[++numfiles] = optarg;
			break;

		case 'v':
			fprintf(stderr, "%s", version_string);
			break;

		case 'V':
			copyleft();
			break;

		case '?':
		default:
			/* getopt will print a message for us */
			/* S5R4 awk ignores bad options and keeps going */
			break;
		}
	}
#ifdef DEBUG
	setbuf(stdout, (char *) NULL);	/* make debugging easier */
#endif
	/* No -f option, use next arg */
	/* write to temp file and save sourcefile name */
	if (numfiles == -1) {
		int i;

		if (optind > argc - 1)	/* no args left */
			usage();
		numfiles++;
		sourcefile[0] = mktemp (template);
		i = strlen (argv[optind]);
		if ((fp = fopen (sourcefile[0], "w")) == NULL)
			fatal("could not save source prog in temp file (%s)",
			sys_errlist[errno]);
		if (fwrite (argv[optind], 1, i, fp) == 0)
			fatal(
			"could not write source program to temp file (%s)",
			sys_errlist[errno]);
		if (argv[optind][i-1] != '\n')
			putc ('\n', fp);
		(void) fclose (fp);
		tempsource++;
		optind++;
	}
	init_args(optind, argc, myname, argv);

	/* Read in the program */
	if (yyparse() || errcount)
		exit(1);

#ifdef DEBUG
	if (dotree)
		print_parse_tree(expression_value);
#endif
	/* Set up the field variables */
	init_fields();

	if (begin_block)
		(void) interpret(begin_block);
	if (!exiting && (expression_value || end_block)) {
		if(input_file)
			do_file(input_file);
		while ((fp = nextfile()) != NULL) {
			do_file(fp);
			if (exiting)
				break;
		}
	}
	if (end_block)
		(void) interpret(end_block);
	if (flush_io() != 0 && exit_val == 0)
		exit_val = 1;
	if (close_io() != 0 && exit_val == 0)
		exit_val = 1;
	exit(exit_val);
}

static void
do_file(fp)
FILE *fp;
{
	input_file = fp;
	/* This is where it spends all its time.  The infamous MAIN LOOP */
	if (inrec() == 0)
		while (interpret(expression_value) && inrec() == 0)
			;
	if (fp != stdin)
		(void) fclose(fp);
}

static void
usage()
{
#ifdef STRICT
	char *opt1 = "[ -Ffs ] -f progfile [ -- ]";
	char *opt2 = "[ -Ffs ] [ -- ] 'program'";
#else
	char *opt1 = "[ -v ] [ -Ffs ] -f progfile [ -- ]";
	char *opt2 = "[ -v ] [ -Ffs ] [ -- ] 'program'";
#endif

	fprintf(stderr, "usage: %s %s file ...\n       %s %s file ...\n",
		myname, opt1, myname, opt2);
	exit(11);
}

/* Generate compiled regular expressions */
struct re_pattern_buffer *
make_regexp(s, ignorecase)
NODE *s;
int ignorecase;
{
	struct re_pattern_buffer *rp;
	char *err;

	emalloc(rp, struct re_pattern_buffer *, sizeof(*rp), "make_regexp");
	bzero((char *) rp, sizeof(*rp));
	emalloc(rp->buffer, char *, 8, "make_regexp");
	rp->allocated = 8;
	emalloc(rp->fastmap, char *, 256, "make_regexp");

	if (! strict && ignorecase)
		rp->translate = casetable;
	else
		rp->translate = NULL;
	if ((err = re_compile_pattern(s->stptr, s->stlen, rp)) != NULL)
		fatal("%s: /%s/", err, s->stptr);
	free_temp(s);
	return rp;
}

FILE *
nextfile()
{
	static int i = 1;
	static int files = 0;
	char *arg;
	char *cp;
	FILE *fp;

	for (; i < (int) (ARGC_node->lnode->numbr); i++) {
		arg = (*assoc_lookup(ARGV_node, tmp_number((AWKNUM) i)))->stptr;
		if (*arg == '\0')
			continue;
		cp = index(arg, '=');
		if (cp != NULL) {
			*cp++ = '\0';
			variable(arg)->var_value = make_string(cp, strlen(cp));
		} else {
			extern NODE *deref;

			files++;
			if (STREQ(arg, "-"))
				fp = stdin;
			else
				fp = fopen(arg, "r");
			if (fp == NULL)
				fatal("cannot open file `%s' for reading (%s)",
					arg, sys_errlist[errno]);
				/* NOTREACHED */
			/* This is a kludge.  */
			deref = FILENAME_node->var_value;
			do_deref();
			FILENAME_node->var_value =
				make_string(arg, strlen(arg));
			FNR_node->var_value->numbr = 0.0;
			i++;
			return fp;
		}
	}
	if (files == 0) {
		files++;
		/* no args. -- use stdin */
		/* FILENAME is init'ed to "-" */
		/* FNR is init'ed to 0 */
		return stdin;
	}
	return NULL;
}

/* Name points to a variable name.  Make sure its in the symbol table */
NODE *
variable(name)
char *name;
{
	register NODE *r;

	if ((r = lookup(variables, name)) == NULL)
		r = install(variables, name,
			node(Nnull_string, Node_var, (NODE *) NULL));
	return r;
}

/* Create a special variable */
NODE *
spc_var(name, value)
char *name;
NODE *value;
{
	register NODE *r;

	if ((r = lookup(variables, name)) == NULL)
		r = install(variables, name, node(value, Node_var, (NODE *) NULL));
	return r;
}

struct re_pattern_buffer *
mk_re_parse(s, ignorecase)
char *s;
int ignorecase;
{
	register char *src, *dest;
	int c;

	for (dest = src = s; *src != '\0'; src++) {
		if (*src == '\\') {
			c = *++src;
			switch (c) {
			case 'a':
				if (strict)
					goto def;
				else
					*dest++ = BELL;
				break;
			case 'b':
				*dest++ = '\b';
				break;
			case 'f':
				*dest++ = '\f';
				break;
			case 'n':
				*dest++ = '\n';
				break;
			case 'r':
				*dest++ = '\r';
				break;
			case 't':
				*dest++ = '\t';
				break;
			case 'v':
				if (strict)
					goto def;
				else
					*dest++ = '\v';
				break;
			case 'x':
				if (strict)
					goto def;
				else {
					register int i = 0;
					register int c;

					while ((c = *++src)) {
						if (! isxdigit(c))
							break;
						if (isdigit(c))
							i += c - '0';
						else if (isupper(c))
							i += c - 'A' + 10;
						else
							i += c - 'a' + 10;
					}
					*dest++ = i;
				}
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				{
				register int i = c - '0';
				register int count = 0;
	
				while (++count < 3) {
					if ((c = *++src) >= '0' && c <= '7') {
						i *= 8;
						i += c - '0';
					} else
						break;
				}
				*dest++ = i;
				}
				break;
			default:
			def:
				*dest++ = '\\';
				*dest++ = c;
				break;
			}
		} else if (*src == '/')
			break;
		else
			*dest++ = *src;
	}
	return make_regexp(tmp_string(s, dest-s), ignorecase);
}

copyleft ()
{
	extern char *version_string;
	char *cp;
	static char blurb[] =
".\nCopyright (C) 1989, Free Software Foundation.\n\
GNU Awk comes with ABSOLUTELY NO WARRANTY.  This is free software, and\n\
you are welcome to distribute it under the terms of the GNU General\n\
Public License, which covers both the warranty information and the\n\
terms for redistribution.\n\
\n\
You should have received a copy of the GNU General Public License along\n\
with this program; if not, write to the Free Software Foundation, Inc.,\n\
675 Mass Ave, Cambridge, MA 02139, USA.\n";

	for (cp = version_string; *cp && *cp != '\n'; cp++)
		putc (*cp, stderr);
	fprintf(stderr, blurb);
}