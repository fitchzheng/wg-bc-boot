#ifdef IS_PLECS
#else
#include "shell.h"
#include "gd32f30x.h"
#include "section.h"
#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "bsp_usart.h"

extern uint32_t Load$$SHELL$$Base;
extern uint32_t Load$$SHELL$$Limit;

static uint8_t rx_buff[RX_BUFF_LEN];
static uint32_t rx_buff_cnt = 0;
static uint32_t rx_name_cnt = 0;

static uint8_t rx_complete = 0;

volatile uint8_t usart_buff[USART_BUFF_LEN];

static void shell_init(void)
{
    REG_SHELL_DATA_TYPE_T *str;
    str = (REG_SHELL_DATA_TYPE_T *)&Load$$SHELL$$Base;
    uint8_t u8_temp = 0;
    uint16_t u16_temp = 0;
    uint32_t u32_temp = 0;
    int8_t i8_temp = 0;
    int16_t i16_temp = 0;
    int32_t i32_temp = 0;
    float f32_temp = 0.0f;
    for (; str < (REG_SHELL_DATA_TYPE_T *)&Load$$SHELL$$Limit; str++)
    {
        float default_value = str->default_value;
        switch (str->shell_type)
        {
        case SHELL_DATA_UINT8_T:
            u8_temp = (uint8_t)default_value;
            memcpy((void *)str->p_var, &u8_temp, sizeof(uint8_t));
            break;
        case SHELL_DATA_UINT16_T:
            u16_temp = (uint16_t)default_value;
            memcpy((void *)str->p_var, &u16_temp, sizeof(uint16_t));
            break;
        case SHELL_DATA_UINT32_T:
            u32_temp = (uint32_t)default_value;
            memcpy((void *)str->p_var, &u32_temp, sizeof(uint32_t));
            break;
        case SHELL_DATA_INT8_T:
            i8_temp = (int8_t)default_value;
            memcpy((void *)str->p_var, &i8_temp, sizeof(int8_t));
            break;
        case SHELL_DATA_INT16_T:
            i16_temp = (int16_t)default_value;
            memcpy((void *)str->p_var, &i16_temp, sizeof(int16_t));
            break;
        case SHELL_DATA_INT32_T:
            i32_temp = (int32_t)(default_value);
            memcpy((void *)str->p_var, &i32_temp, sizeof(int32_t));
            break;
        case SHELL_DATA_FLOAT:
            f32_temp = default_value;
            memcpy((void *)str->p_var, &f32_temp, sizeof(float));
            break;
        default:
            break;
        }
    }
}

REG_INIT_TP(shell_init);

static void shell_run(void)
{
    REG_SHELL_DATA_TYPE_T *str;
    str = (REG_SHELL_DATA_TYPE_T *)&Load$$SHELL$$Base;
    if (rx_complete == 1)
    {
        rx_complete = 0;
        uint8_t err = 0;
        uint8_t u8_temp = 0;
        uint16_t u16_temp = 0;
        uint32_t u32_temp = 0;
        int8_t i8_temp = 0;
        int16_t i16_temp = 0;
        int32_t i32_temp = 0;
        uint8_t data_is_negedge = 0;
        float f32_temp = 0.0f;
        float f32_integer = 0.0f;
        float f32_decimal = 0.0f;
        uint8_t is_decimal = 0;
        uint32_t match_cnt = 0;
        uint8_t is_match = 1;

        for (; str < (REG_SHELL_DATA_TYPE_T *)&Load$$SHELL$$Limit; str++)
        {
            is_match = 1;
            for (match_cnt = 0; match_cnt < rx_name_cnt; match_cnt++)
            {
                if (str->p_name[match_cnt] != rx_buff[match_cnt])
                {
                    is_match = 0;
                    break;
                }
            }
            if (is_match == 1)
            {
                err = 1;
                switch (str->shell_type)
                {
                case SHELL_CMD:
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_UINT8_T:
                    u8_temp = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        u8_temp *= 10;
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            u8_temp += rx_buff[i] - '0';
                        }
                        else
                        {
                            err = 1;
                        }
                    }
                    if (err == 0)
                    {
                        memcpy((void *)str->p_var, &u8_temp, sizeof(uint8_t));
                        printf("%s = %d\r\n", str->p_name, *(uint8_t *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %d\r\n", str->p_name, *(uint8_t *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_UINT16_T:
                    u16_temp = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        u16_temp *= 10;
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            u16_temp += rx_buff[i] - '0';
                        }
                        else
                        {
                            err = 1;
                        }
                    }
                    if (err == 0)
                    {
                        memcpy((void *)str->p_var, &u16_temp, sizeof(uint16_t));
                        printf("%s = %d\r\n", str->p_name, *(uint16_t *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %d\r\n", str->p_name, *(uint16_t *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_UINT32_T:
                    u32_temp = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        u32_temp *= 10;
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            u32_temp += rx_buff[i] - '0';
                        }
                        else
                        {
                            err = 1;
                        }
                    }
                    if (err == 0)
                    {
                        memcpy((void *)str->p_var, &u32_temp, sizeof(uint32_t));
                        printf("%s = %d\r\n", str->p_name, *(uint32_t *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %d\r\n", str->p_name, *(uint32_t *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_INT8_T:
                    i8_temp = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        i8_temp *= 10;
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            i8_temp += rx_buff[i] - '0';
                        }
                        else if ((rx_buff[i] == '-') &&
                                 (data_is_negedge == 0))
                        {
                            data_is_negedge = 1;
                        }
                        else
                        {
                            err = 1;
                        }
                    }
                    if (err == 0)
                    {
                        if (data_is_negedge == 1)
                        {
                            i8_temp = -i8_temp;
                        }
                        memcpy((void *)str->p_var, &i8_temp, sizeof(int8_t));
                        printf("%s = %d\r\n", str->p_name, *(int8_t *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %d\r\n", str->p_name, *(int8_t *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_INT16_T:
                    i16_temp = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        i16_temp *= 10;
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            i16_temp += rx_buff[i] - '0';
                        }
                        else if ((rx_buff[i] == '-') &&
                                 (data_is_negedge == 0))
                        {
                            data_is_negedge = 1;
                        }
                        else
                        {
                            err = 1;
                        }
                    }
                    if (err == 0)
                    {
                        if (data_is_negedge == 1)
                        {
                            i16_temp = -i16_temp;
                        }
                        memcpy((void *)str->p_var, &i16_temp, sizeof(int16_t));
                        printf("%s = %d\r\n", str->p_name, *(int16_t *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %d\r\n", str->p_name, *(int16_t *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_INT32_T:
                    i32_temp = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        i32_temp *= 10;
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            i32_temp += rx_buff[i] - '0';
                        }
                        else if ((rx_buff[i] == '-') &&
                                 (data_is_negedge == 0))
                        {
                            data_is_negedge = 1;
                        }
                        else
                        {
                            err = 1;
                            break;
                        }
                    }
                    if (err == 0)
                    {
                        if (data_is_negedge == 1)
                        {
                            i32_temp = -i32_temp;
                        }
                        memcpy((void *)str->p_var, &i32_temp, sizeof(int32_t));
                        printf("%s = %d\r\n", str->p_name, *(int32_t *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %d\r\n", str->p_name, *(int32_t *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                case SHELL_DATA_FLOAT:
                    f32_temp = 0.0f;
                    f32_decimal = 1.0f;
                    f32_integer = 0.0f;
                    is_decimal = 0;
                    for (uint32_t i = rx_name_cnt + 1; i < rx_buff_cnt - 2; i++)
                    {
                        if ((rx_buff[i] >= '0') &&
                            (rx_buff[i] <= '9'))
                        {
                            err = 0;
                            f32_integer *= 10.0f;
                            f32_integer += rx_buff[i] - '0';
                            if (is_decimal == 1)
                            {
                                f32_decimal /= 10.0f;
                            }
                        }
                        else if ((rx_buff[i] == '-') &&
                                 (data_is_negedge == 0))
                        {
                            data_is_negedge = 1;
                        }
                        else if ((rx_buff[i] == '.') &&
                                 (is_decimal == 0))
                        {
                            is_decimal = 1;
                        }
                        else
                        {
                            err = 1;
                            break;
                        }
                    }
                    f32_temp = f32_integer * f32_decimal;
                    if (data_is_negedge == 1)
                    {
                        f32_temp = -f32_temp;
                    }
                    if (err == 0)
                    {
                        memcpy((void *)str->p_var, &f32_temp, sizeof(float));
                        printf("%s = %f\r\n", str->p_name, *(float *)str->p_var);
                    }
                    else
                    {
                        printf("%s = %f\r\n", str->p_name, *(float *)str->p_var);
                    }
                    if (str->cmd_func != NULL)
                    {
                        printf("Executing %s\r\n", str->p_name);
                        str->cmd_func();
                    }
                    break;
                }
                break;
            }
        }
        if (is_match == 0)
        {
            printf("No matching fields found\r\n");
            printf("You can learn how to use this stupid thing through 'help'.\r\n");
        }
        rx_buff_cnt = 0;
        rx_name_cnt = 0;
    }
}

REG_TASK(10, shell_run);

static void list(void)
{
    REG_SHELL_DATA_TYPE_T *str;
    str = (REG_SHELL_DATA_TYPE_T *)&Load$$SHELL$$Base;

    for (; str < (REG_SHELL_DATA_TYPE_T *)&Load$$SHELL$$Limit; str++)
    {
        printf("%s\t", str->p_name);
        switch (str->shell_type)
        {
        case SHELL_CMD:
            printf("SHELL_CMD\r\n");
            break;
        case SHELL_DATA_UINT8_T:
            printf("SHELL_DATA_UINT8_T\t");
            printf("%d\r\n", *(uint8_t *)str->p_var);
            break;
        case SHELL_DATA_UINT16_T:
            printf("SHELL_DATA_UINT16_T\t");
            printf("%d\r\n", *(uint16_t *)str->p_var);
            break;
        case SHELL_DATA_UINT32_T:
            printf("SHELL_DATA_UINT32_T\t");
            printf("%d\r\n", *(uint32_t *)str->p_var);
            break;
        case SHELL_DATA_INT8_T:
            printf("SHELL_DATA_INT8_T\t");
            printf("%d\r\n", *(int8_t *)str->p_var);
            break;
        case SHELL_DATA_INT16_T:
            printf("SHELL_DATA_INT16_T\t");
            printf("%d\r\n", *(int16_t *)str->p_var);
            break;
        case SHELL_DATA_INT32_T:
            printf("SHELL_DATA_INT32_T\t");
            printf("%d\r\n", *(int32_t *)str->p_var);
            break;
        case SHELL_DATA_FLOAT:
            printf("SHELL_DATA_FLOAT\t");
            printf("%f\r\n", *(float *)str->p_var);
            break;
        }
    }
}

static void help(void)
{
    printf("Firstly, use 'list' to know what commands or variables are available for use.\r\n");
    printf("If it is a command, you can directly fill in and send the corresponding command, which will execute the corresponding function.\r\n");
    printf("If it is a variable, you can fill in the corresponding variable, then separate it with ':', and finally input data to modify the variable.\r\n");
    printf("'SHELL_CMD' is the command type, and other types are variables.\r\n");
    printf("\r\n");
    printf("Serial port configuration that needs to be known:\r\n");
    printf("    Open Send New Line\r\n");
    printf("    String sending\r\n");
    printf("    The baud rate is 1000000\r\n");
    printf("    Stop bit is 1\r\n");
    printf("    The data bit is 8\r\n");
    printf("    No check digit\r\n");
    printf("\r\n");
    printf("Wishing you a pleasant use.\r\n");
}

REG_SHELL(list, list, SHELL_CMD, 0, list);
REG_SHELL(help, help, SHELL_CMD, 0, help);

void shell_rx_data(void)
{
    static uint32_t rx_data_cnt = USART2_RX_BUFFER_SIZE;
    volatile uint8_t rx_data = 0;
    static uint8_t rx_data_last = 0;
    static uint32_t time_out = 0;
    while (rx_data_cnt != dma_transfer_number_get(DMA0, DMA_CH2))
    {
        time_out = 0;
        if (rx_data_cnt == 0)
        {
            rx_data_cnt = USART2_RX_BUFFER_SIZE;
        }
        rx_data = usart2_rx_buffer[USART2_RX_BUFFER_SIZE - rx_data_cnt];
        if ((rx_buff_cnt < RX_BUFF_LEN) &&
            (rx_complete) == 0)
        {
            rx_buff[rx_buff_cnt] = rx_data;
            rx_buff_cnt++;
        }
        if ((rx_name_cnt == 0) &&
            ((rx_data == '\r') ||
             (rx_data == ':')))
        {
            rx_name_cnt = rx_buff_cnt - 1;
        }
        if ((rx_data_last == '\r') &&
            (rx_data == '\n'))
        {
            rx_complete = 1;
        }

        rx_data_last = rx_data;
        rx_data_cnt--;
    }
    time_out++;
    if (time_out > 20)
    {
        time_out = 0;
        memset(rx_buff, 0, sizeof(rx_buff));
        rx_buff_cnt = 0;
    }
}

REG_TASK(1, shell_rx_data);

#endif
