#include <stdio.h>
#include <string.h>

#include "optfetch.h"

typedef unsigned int uint;


static int countopts(const struct opttype *opts) {
	uint i;

	for (i = 0;
		/* longname and short name are both 0, OR */
		!(((opts[i].longname == NULL) && (opts[i].shortname == '\0')) ||
		/* output type was unspecified OR */
		((opts[i].type == 0) || (opts[i].type > OPTTYPE_STRING)) ||
		/* nowhere to output data */
		(opts[i].outdata == NULL));

		i++);

	return i;
}

static int get_option_index_short(char opt, const struct opttype *potentialopts, uint len) {
	uint i;

	for (i = 0; i < len; i++) {
		if (!potentialopts[i].shortname) {
			continue;
		} else if (potentialopts[i].shortname == opt) {
			return i;
		}
	}
	return -1;
}

static int get_option_index_long(const char *opt, const struct opttype *potentialopts, uint len) {
	uint i;

	for (i = 0; i < len; i++) {
		if (!potentialopts[i].longname) {
			continue;
		} else if (!strcmp(potentialopts[i].longname, opt)) {
			return i;
		}
	}
	return -1;
}


void fetchopts(int *argc, char ***argv, struct opttype *opts) {
	uint numopts = countopts(opts);

	int argindex;
	int newargc = 1;

	struct opttype *wasinarg = NULL;

	char *curropt;
	/* start at 1 because index 0 is the executable name */
	for (argindex = 1; argindex < *argc; argindex++) {
		if ((curropt = (*argv)[argindex]) == NULL) continue;

		/* Last argument was an option, now we're setting the actual value of that option */
		if (wasinarg != NULL) {
			char *format_specifier;

			switch (wasinarg->type) {
				/* We set the format specifier here then make
				 * one sscanf call with it.  We don't even need
				 * to cast it because it's already a pointer
				 * unless the user fucked something up which is
				 * their fault!
				 */
				case OPTTYPE_CHAR: format_specifier = "%c"; break;
				case OPTTYPE_SHORT: format_specifier = "%hi"; break;
				case OPTTYPE_USHORT: format_specifier = "%hu"; break;
				case OPTTYPE_INT: format_specifier = "%d"; break;
				case OPTTYPE_UINT: format_specifier = "%u"; break;
				case OPTTYPE_LONG: format_specifier = "%ld"; break;
				case OPTTYPE_ULONG: format_specifier = "%lu"; break;
#ifdef _WIN32
				case OPTTYPE_LONGLONG: format_specifier = "%l64d"; break;
				case OPTTYPE_ULONGLONG: format_specifier = "%l64u"; break;
#else
				case OPTTYPE_LONGLONG: format_specifier = "%lld"; break;
				case OPTTYPE_ULONGLONG: format_specifier = "%llu"; break;
#endif
				case OPTTYPE_FLOAT: format_specifier = "%f"; break;
				case OPTTYPE_DOUBLE: format_specifier = "%lf"; break;
				case OPTTYPE_LONGDOUBLE: format_specifier = "%Lf"; break;

				case OPTTYPE_STRING:
					*(char**)(wasinarg->outdata) = curropt;
					wasinarg = NULL;
					format_specifier = NULL;
					continue;
				/* OPTTYPE_BOOL already handled */
			}
			sscanf(curropt, format_specifier, wasinarg->outdata);
			wasinarg = NULL;
			format_specifier = NULL;
		} else {
			/* Has the user manually demanded that the option-parsing end now? */
			if (!strcmp(curropt, "--")) {
				argindex++;

				goto end;
			}

			/* in an option, getting warmer */
			if (curropt[0] == '-') {
				int option_index = -1;
				unsigned char oneoffset;

				/* was it a --foo or just a -foo? */
				if (curropt[1] == '-') {
					oneoffset = 2;
				} else {
					oneoffset = 1;
				}

				/* is it a short opt (e.g. -f) or a long one (e.g. -foo)? */
				if (strlen(curropt+oneoffset) == 1) {
					option_index = get_option_index_short(curropt[oneoffset], opts, numopts);
				}

				/* long opt OR nothing matched for short opt ('f' is a valid longname) */
				if (strlen(curropt+oneoffset) > 1 || option_index == -1) {
					option_index = get_option_index_long(curropt+oneoffset, opts, numopts);
				}

				/* not an option */
				if (option_index == -1) {
					(*argv)[newargc++] = curropt;
					continue;
				} else {
					/* it's a boolean option, so the next loop doesn't want to know about it */
					if ((opts[option_index]).type == OPTTYPE_BOOL) {
#if __STDC_VERSION__ >= 199901L
						*(_Bool*)opts[option_index].outdata = 1;
#else
						*(int*)opts[option_index].outdata = 1;
#endif
					/* let the next loop iteration get the value */
					} else {
						wasinarg = &opts[option_index];
					}
				}
			} else {
				(*argv)[newargc++] = curropt;
			}
		}
	}

end:
	{
		int i;

		for (i = argindex; i < *argc; i++) {
			/* -1, because argv starts at 1 (with 0 as program name), but newargv starts at 0 */
			(*argv)[newargc++] = (*argv)[i];
		}
	}

	*argc = newargc - 1;
}
