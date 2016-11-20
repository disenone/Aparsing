// parsing args with type

#ifndef GET_PARAMS
#define GET_PARAMS

#include <stdint.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
# ifdef DLL_EXPORTS
#  define EXPORTS_API _declspec(dllexport)
# else
#  define EXPORTS_API _declspec(dllimport)
# endif
#else
# define EXPORTS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ENOENT       2  /* No such file or directory */
#define ENOSPC      28  /* No space left on device */
#define EINVAL      22  /* Invalid argument */
#define ENOMEM      12  /* Out of memory */

struct param_info;
struct param_array;
struct param_string;

/* Returns 0, or -errno.  arg is in kp->arg. */
typedef int (*param_set_fn)(const char *val, struct param_info *kp);
/* Returns length written or -errno.  Buffer is 4k (ie. be short!) */
typedef int (*param_get_fn)(char *buffer, struct param_info *kp);

/* Flag bits for param_info.flags */
#define PARAM_ISBOOL		2

struct param_info {
	const char *name;
	uint16_t flags;
	param_set_fn set;
	param_get_fn get;
	union {
		void *arg;
		const struct param_string *str;
		const struct param_array *arr;
	};
};

/* Special one for strings we want to copy into */
struct param_string {
	unsigned int maxlen;
	char *string;
};

/* Special one for arrays */
struct param_array
{
	unsigned int max;
	unsigned int *num;
	param_set_fn set;
	param_get_fn get;
	unsigned int elemsize;
	void *elem;
};

typedef int bool;

/* On alpha, ia64 and ppc64 relocations to global data cannot go into
   read-only sections (which is part of respective UNIX ABI on these
   platforms). So 'const' makes no sense and even causes compile failures
   with some compilers. */
#if defined(CONFIG_ALPHA) || defined(CONFIG_IA64) || defined(CONFIG_PPC64)
#define __moduleparam_const
#else
#define __moduleparam_const const
#endif

#define MODULE_PARAM_PREFIX

/* Chosen so that structs with an unsigned long line up. */
#define MAX_PARAM_PREFIX_LEN (64 - sizeof(unsigned long))

#define MODULE_INIT_VARIABLE __module_params
#define MODULE_INIT_VARIABLE_INDEX __module_params_index
#define MODULE_INIT_VARIABLE_NUM __module_params_num

#define init_module_param(num)  \
    static int MODULE_INIT_VARIABLE_INDEX = 0;			\
	static const int MODULE_INIT_VARIABLE_NUM = num; 	\
    static struct param_info MODULE_INIT_VARIABLE[num];	\
    memset(&MODULE_INIT_VARIABLE, 0, sizeof(MODULE_INIT_VARIABLE))

#define BUILD_BUG_ON_ZERO(e) (sizeof(char[1 - 2 * !!(e)]) - 1)

/* This is the fundamental function for registering boot/module
   parameters.  perm sets the visibility in sysfs: 000 means it's
   not there, read bits mean it's readable, write bits mean it's
   writable. */
#define __module_param_call(prefix, vname, vset, vget, varg, isbool)  \
	assert(MODULE_INIT_VARIABLE_INDEX < MODULE_INIT_VARIABLE_NUM &&	\
		"module param num exceed max num, please edit init_module_param !!"); 	\
	static const char __param_str_##vname[] = prefix #vname;		\
    MODULE_INIT_VARIABLE[MODULE_INIT_VARIABLE_INDEX].name = __param_str_##vname; \
    MODULE_INIT_VARIABLE[MODULE_INIT_VARIABLE_INDEX].flags = isbool ? PARAM_ISBOOL : 0; \
    MODULE_INIT_VARIABLE[MODULE_INIT_VARIABLE_INDEX].set = vset; \
    MODULE_INIT_VARIABLE[MODULE_INIT_VARIABLE_INDEX].get = vget; \
    MODULE_INIT_VARIABLE[MODULE_INIT_VARIABLE_INDEX]varg; \
    ++MODULE_INIT_VARIABLE_INDEX

#define module_param_call(name, set, get, varg, isbool)     \
	__module_param_call(MODULE_PARAM_PREFIX,			    \
			    name, set, get, .arg = varg, 0)			    \

#define module_param_named(name, value, type)			    \
	module_param_call(name, param_set_##type, param_get_##type, &value, 0)

#define module_param(name, type)				\
	module_param_named(name, name, type)

#define module_param_bool(name)    \
    module_param_call(name, param_set_bool, param_get_bool, &name, 1)

/* Actually copy string: maxlen param is usually sizeof(string). */
#define module_param_string(name, string, len)			\
	static const struct param_string __param_string_##name		\
		= { len, string };					\
	__module_param_call(MODULE_PARAM_PREFIX, name,			\
			    param_set_copystring, param_get_string,	\
			    .str = &__param_string_##name, 0);	\

/* All the helper functions */
/* The macros to do compile-time type checking stolen from Jakub
   Jelinek, who IIRC came up with this idea for the 2.4 module init code. */
#define __param_check(name, p, type) \
	static inline type *__check_##name(void) { return (p); }

extern EXPORTS_API int param_set_byte(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_byte(char *buffer, struct param_info *kp);
#define param_check_byte(name, p) __param_check(name, p, unsigned char)

extern EXPORTS_API int param_set_short(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_short(char *buffer, struct param_info *kp);
#define param_check_short(name, p) __param_check(name, p, short)

extern EXPORTS_API int param_set_ushort(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_ushort(char *buffer, struct param_info *kp);
#define param_check_ushort(name, p) __param_check(name, p, unsigned short)

extern EXPORTS_API int param_set_int(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_int(char *buffer, struct param_info *kp);
#define param_check_int(name, p) __param_check(name, p, int)

extern EXPORTS_API int param_set_uint(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_uint(char *buffer, struct param_info *kp);
#define param_check_uint(name, p) __param_check(name, p, unsigned int)

extern EXPORTS_API int param_set_long(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_long(char *buffer, struct param_info *kp);
#define param_check_long(name, p) __param_check(name, p, long)

extern EXPORTS_API int param_set_ulong(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_ulong(char *buffer, struct param_info *kp);
#define param_check_ulong(name, p) __param_check(name, p, unsigned long)

extern EXPORTS_API int param_set_charp(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_charp(char *buffer, struct param_info *kp);
#define param_check_charp(name, p) __param_check(name, p, char *)

/* For historical reasons "bool" parameters can be (unsigned) "int". */
extern EXPORTS_API int param_set_bool(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_bool(char *buffer, struct param_info *kp);
#define param_check_bool(name, p)					\
	static inline void __check_##name(void)				\
	{								\
	}

extern EXPORTS_API int param_set_invbool(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_invbool(char *buffer, struct param_info *kp);
#define param_check_invbool(name, p) __param_check(name, p, bool)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* Comma-separated array: *nump is set to number they actually specified. */
#define module_param_array_named(name, array, type, nump)		\
	static const struct param_array __param_arr_##name		\
	= { ARRAY_SIZE(array), nump, param_set_##type, param_get_##type,\
	    sizeof(array[0]), array };					\
	__module_param_call(MODULE_PARAM_PREFIX, name,			\
			    param_array_set, param_array_get,		\
			    .arr = &__param_arr_##name,			\
			    0);		\

#define module_param_array(name, type, nump)		\
	module_param_array_named(name, name, type, nump)

extern EXPORTS_API int param_array_set(const char *val, struct param_info *kp);
extern EXPORTS_API int param_array_get(char *buffer, struct param_info *kp);

extern EXPORTS_API int param_set_copystring(const char *val, struct param_info *kp);
extern EXPORTS_API int param_get_string(char *buffer, struct param_info *kp);

extern EXPORTS_API int parse_args(struct param_info *params,
        int num,
        int argc,
        char **argv,
        int (*unknown)(char *param, char *val));

#define parse_params(argc, argv, func)      \
    parse_args(MODULE_INIT_VARIABLE, MODULE_INIT_VARIABLE_NUM, argc, argv, func)

#ifdef __cplusplus
}
#endif

#endif // GET_PARAMS
