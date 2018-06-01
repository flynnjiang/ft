#ifndef __FT_TYPES_H__
#define __FT_TYPES_H__


/* 错误码 */
#define FT_OK            0
#define FT_ESYS         -1
#define FT_EFULL        -2
#define FT_EEMPTY       -3
#define FT_ENOMEM       -4
#define FT_EBADPARAM    -5
#define FT_ETIMEOUT     -6
#define FT_EEXIST       -7
#define FT_ENOEXIST     -8
#define FT_EUNKNOWN     -255

/* 便利宏 */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#endif /* __FT_TYPES_H__ */
