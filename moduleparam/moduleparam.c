/* Helpers for initial module or kernel cmdline parsing
   Copyright (C) 2001 Rusty Russell.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "moduleparam.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef DEBUG
#define printk printf
#define DEBUGP printk
#else
#define printk(fmt, a...)
#define DEBUGP(fmt, a...)
#endif

char *skip_spaces(const char *str)
{
    while (isspace(*str))
        ++str;
    return (char *)str;
}

static inline char dash2underscore(char c)
{
	if (c == '-')
		return '_';
	return c;
}

static inline int parameq(const char *input, const char *paramname)
{
	unsigned int i;
	for (i = 0; dash2underscore(input[i]) == paramname[i]; i++)
		if (input[i] == '\0')
			return 1;
	return 0;
}

static int parse_one(char *param,
		     char *val,
		     struct param_info *params, 
		     unsigned num_params,
		     int (*handle_unknown)(char *param, char *val))
{
	unsigned int i;

	/* Find parameter */
	for (i = 0; i < num_params; i++) {
		if (parameq(param, params[i].name)) {
			//DEBUGP("They are equal!  Calling %p\n", params[i].set);
			return params[i].set(val, &params[i]);
		}
	}

	if (handle_unknown) {
		DEBUGP("Unknown argument: calling %p\n", handle_unknown);
		return handle_unknown(param, val);
	}

	DEBUGP("Unknown argument '%s'\n", param);
	return -ENOENT;
}

/* You can use " around spaces, but can't escape ". */
/* Hyphens and underscores equivalent in parameter names. */
static char *next_arg(char *args, char **param, char **val)
{
	unsigned int i, equals = 0;
	int in_quote = 0, quoted = 0;
	char *next;

	if (*args == '"') {
		args++;
		in_quote = 1;
		quoted = 1;
	}

	for (i = 0; args[i]; i++) {
		if (isspace(args[i]) && !in_quote)
			break;
		if (equals == 0) {
			if (args[i] == '=')
				equals = i;
		}
		if (args[i] == '"')
			in_quote = !in_quote;
	}

	*param = args;
	if (!equals)
		*val = NULL;
	else {
		args[equals] = '\0';
		*val = args + equals + 1;

		/* Don't include quotes in value. */
		if (**val == '"') {
			(*val)++;
			if (args[i-1] == '"')
				args[i-1] = '\0';
		}
		if (quoted && args[i-1] == '"')
			args[i-1] = '\0';
	}

	if (args[i]) {
		args[i] = '\0';
		next = args + i + 1;
	} else
		next = args + i;

	/* Chew up trailing spaces. */
	return skip_spaces(next);
}

/* Args looks like "foo=bar,bar2 baz=fuz wiz". */
int parse_args(struct param_info *params,
        int num,
        int argc,
        char **argv,
        int (*unknown)(char *param, char *val))
{
	char *param, *val;

    /* count args len */
    int i;
    int len = 1;
    for (i = 0;i < argc; ++i) {
        len += strlen(argv[i]);
    }

    char *args = (char *)malloc(len);
    char *orig_args = args;
    args[0] = '\0';

    for (i = 0;i < argc; ++i) {
        strncat(args, argv[i], len);
        if(i < argc - 1)
            strcat(args, " ");
    }

	DEBUGP("Parsing ARGS: %s\n", args);

	/* Chew leading spaces */
	args = skip_spaces(args);

	int ret = 0;
	while (*args) {
		args = next_arg(args, &param, &val);
		ret = parse_one(param, val, params, num, unknown);

		switch (ret) {
            case -ENOENT:
                printk("Unknown parameter '%s'\n", param);
                goto error;
            case -ENOSPC:
                printk("'%s' too large for parameter '%s'\n", val ?: "", param);
                goto error;
            case 0:
                break;
            default:
                printk("'%s' invalid for parameter '%s'\n", val ?: "", param);
                goto error;
		}
	}

error:
    free(orig_args);
	return ret;
}

/*
    strict_strtoul converts a string to an unsigned long only if 
    the string is really an unsigned long string, 
    any string containing any invalid char at the tail will be 
    rejected and -EINVAL is returned, only a newline char at the tail 
    is acceptible because people generally
*/
int strict_strtoul(const char *cp, unsigned int base, unsigned long *res)
{
    char *tail;
    unsigned long val;
    size_t len;

    *res = 0;
    len = strlen(cp);
    if (len == 0)
        return -EINVAL;

    val = strtoul(cp, &tail, base);
    if (tail == cp)
        return -EINVAL;

    if ((*tail == '\0') ||
        ((len == (size_t)(tail - cp) + 1) && (*tail == '\n'))) {
        *res = val;
        return 0;
    }
    
    return -EINVAL;
}

int strict_strtol(const char *cp, unsigned int base, long *res)
{
    int ret;
    if (*cp == '-') {
        ret = strict_strtoul(cp + 1, base, (unsigned long *)res);
        if (!ret)
            *res = -(*res);
    } else {
        ret = strict_strtoul(cp, base, (unsigned long *)res);
    }

    return ret;
}

/* Lazy bastard, eh? */
#define STANDARD_PARAM_DEF(name, type, format, tmptype, strtolfn)      	\
	int param_set_##name(const char *val, struct param_info *kp)	\
	{								\
		tmptype l;						\
		int ret;						\
									\
		if (!val) return -EINVAL;				\
		ret = strtolfn(val, 0, &l);				\
		if (ret == -EINVAL || ((type)l != l))			\
			return -EINVAL;					\
		*((type *)kp->arg) = l;					\
		return 0;						\
	}								\
	int param_get_##name(char *buffer, struct param_info *kp)	\
	{								\
		return sprintf(buffer, format, *((type *)kp->arg));	\
	}

STANDARD_PARAM_DEF(byte, unsigned char, "%c", unsigned long, strict_strtoul);
STANDARD_PARAM_DEF(short, short, "%hi", long, strict_strtol);
STANDARD_PARAM_DEF(ushort, unsigned short, "%hu", unsigned long, strict_strtoul);
STANDARD_PARAM_DEF(int, int, "%i", long, strict_strtol);
STANDARD_PARAM_DEF(uint, unsigned int, "%u", unsigned long, strict_strtoul);
STANDARD_PARAM_DEF(long, long, "%li", long, strict_strtol);
STANDARD_PARAM_DEF(ulong, unsigned long, "%lu", unsigned long, strict_strtoul);

/* Actually could be a bool or an int, for historical reasons. */
int param_set_bool(const char *val, struct param_info *kp)
{
	bool v;

	/* No equals means "set"... */
	if (!val) val = "1";

	/* One of =[yYnN01] */
	switch (val[0]) {
	case 'y': case 'Y': case '1':
		v = 1;
		break;
	case 'n': case 'N': case '0':
		v = 0;
		break;
	default:
		return -EINVAL;
	}

	if (kp->flags & PARAM_ISBOOL)
		*(bool *)kp->arg = v;
	else
		*(int *)kp->arg = v;
	return 0;
}

int param_get_bool(char *buffer, struct param_info *kp)
{
	bool val;
	if (kp->flags & PARAM_ISBOOL)
		val = *(bool *)kp->arg;
	else
		val = *(int *)kp->arg;

	/* Y and N chosen as being relatively non-coder friendly */
	return sprintf(buffer, "%c", val ? 'Y' : 'N');
}

/* This one must be bool. */
int param_set_invbool(const char *val, struct param_info *kp)
{
	int ret;
	bool boolval;
	struct param_info dummy;

	dummy.arg = &boolval;
	dummy.flags = PARAM_ISBOOL;
	ret = param_set_bool(val, &dummy);
	if (ret == 0)
		*(bool *)kp->arg = !boolval;
	return ret;
}

int param_get_invbool(char *buffer, struct param_info *kp)
{
	return sprintf(buffer, "%c", (*(bool *)kp->arg) ? 'N' : 'Y');
}

/* We break the rule and mangle the string. */
static int param_array(const char *name,
		       const char *val,
		       unsigned int min, unsigned int max,
		       void *elem, int elemsize,
		       int (*set)(const char *, struct param_info *kp),
		       uint16_t flags,
		       unsigned int *num)
{
	int ret;
	struct param_info kp;
	char save;

	/* Get the name right for errors. */
	kp.name = name;
	kp.arg = elem;
	kp.flags = flags;

	/* No equals sign? */
	if (!val) {
		printk("%s: expects arguments\n", name);
		return -EINVAL;
	}

	*num = 0;
	/* We expect a comma-separated list of values. */
	do {
		int len;

		if (*num == max) {
			printk("%s: can only take %i arguments\n",
			       name, max);
			return -EINVAL;
		}
		len = strcspn(val, ",");

		/* nul-terminate and parse */
		save = val[len];
		((char *)val)[len] = '\0';
		ret = set(val, &kp);

		if (ret != 0)
			return ret;
		kp.arg += elemsize;
		val += len+1;
		(*num)++;
	} while (save == ',');

	if (*num < min) {
		printk("%s: needs at least %i arguments\n",
		       name, min);
		return -EINVAL;
	}
	return 0;
}

int param_array_set(const char *val, struct param_info *kp)
{
	const struct param_array *arr = kp->arr;
	unsigned int temp_num;

	return param_array(kp->name, val, 1, arr->max, arr->elem,
			   arr->elemsize, arr->set, kp->flags,
			   arr->num ?: &temp_num);
}

int param_array_get(char *buffer, struct param_info *kp)
{
	int i, off, ret;
	const struct param_array *arr = kp->arr;
	struct param_info p;

	p = *kp;
	for (i = off = 0; i < (arr->num ? *arr->num : arr->max); i++) {
		if (i)
			buffer[off++] = ',';
		p.arg = arr->elem + arr->elemsize * i;
		ret = arr->get(buffer + off, &p);
		if (ret < 0)
			return ret;
		off += ret;
	}
	buffer[off] = '\0';
	return off;
}

int param_set_copystring(const char *val, struct param_info *kp)
{
	const struct param_string *kps = kp->str;

	if (!val) {
		printk("%s: missing param set value\n", kp->name);
		return -EINVAL;
	}
	if (strlen(val)+1 > kps->maxlen) {
		printk("%s: string doesn't fit in %u chars.\n",
		       kp->name, kps->maxlen-1);
		return -ENOSPC;
	}
	strcpy(kps->string, val);
	return 0;
}

/**
 * strlcpy - Copy a %NUL terminated string into a sized buffer
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @size: size of destination buffer
 *
 * Compatible with *BSD: the result is always a valid
 * NUL-terminated string that fits in the buffer (unless,
 * of course, the buffer size is zero). It does not pad
 * out the result like strncpy() does.
 */
size_t strlcpy(char *dest, const char *src, size_t size)
{
    size_t ret = strlen(src);

    if (size) {
        size_t len = (ret >= size) ? size - 1 : ret;
        memcpy(dest, src, len);
        dest[len] = '\0';
    }
    return ret;
}

int param_get_string(char *buffer, struct param_info *kp)
{
	const struct param_string *kps = kp->str;
	return strlcpy(buffer, kps->string, kps->maxlen);
}


