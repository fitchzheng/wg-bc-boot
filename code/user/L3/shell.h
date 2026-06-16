#ifndef __SHELL_H
#define __SHELL_H

#ifdef IS_PLECS

#include "section.h"

#define REG_SHELL(name, var, type, def, func) \
    void reg_shell_##name##_init(void)        \
    {                                         \
        var = def;                            \
    }                                         \
    REG_INIT(reg_shell_##name##_init)

#else

#include "stdint.h"
#include "bsp_usart.h"
#include "stddef.h"

#define RX_BUFF_LEN USART2_RX_BUFFER_SIZE
#define USART_BUFF_LEN USART2_RX_BUFFER_SIZE

#define SHELL __attribute__((used, __section__(".shell")))
#define STR(x) #x

typedef enum
{
    SHELL_CMD,
    SHELL_DATA_UINT8_T,
    SHELL_DATA_UINT16_T,
    SHELL_DATA_UINT32_T,
    SHELL_DATA_INT8_T,
    SHELL_DATA_INT16_T,
    SHELL_DATA_INT32_T,
    SHELL_DATA_FLOAT,
} SHELL_TYPE_E;

typedef struct
{
    const char *p_name;
    void *p_var;
    const SHELL_TYPE_E shell_type;
    float default_value;
    void (*cmd_func)(void);
} REG_SHELL_DATA_TYPE_T;

#define REG_SHELL(name, var, type, def, func)              \
    const char str_##name[] = STR(name);                   \
    const REG_SHELL_DATA_TYPE_T reg_##name##_str SHELL = { \
        .p_name = &str_##name[0],                          \
        .p_var = (void *)&var,                             \
        .shell_type = type,                                \
        .default_value = (float)def,                       \
        .cmd_func = func,                                  \
    };

#endif

#endif
