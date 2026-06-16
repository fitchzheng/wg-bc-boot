#include "wg_com.h"
#include "bsp_usart.h"
#include "my_math.h"
#include "stdint.h"
#include "section.h"
#include "stdio.h"

static uint8_t wg_com_buffer[MAX_FRAME_SIZE];

static uint8_t wg_com_cal_sum(wg_com_frame_t *wg_com_frame)
{
    uint8_t checksum = 0;
    checksum = wg_com_frame->addr + wg_com_frame->length + wg_com_frame->cid;
    uint8_t len = BCD_TO_DEC(wg_com_frame->length);
    for (uint8_t i = 1; i < len; i++)
    {
        checksum += wg_com_frame->info[i - 1];
    }
    checksum = DEC_TO_BCD(checksum);
    return checksum;
}

static uint8_t wg_com_checksum(wg_com_frame_t *wg_com_frame)
{
    uint8_t checksum = 0;
    checksum = wg_com_frame->addr + wg_com_frame->length + wg_com_frame->cid;
    uint8_t len = BCD_TO_DEC(wg_com_frame->length);
    for (uint8_t i = 1; i < len; i++)
    {
        checksum += wg_com_frame->info[i - 1];
    }
    checksum = DEC_TO_BCD(checksum);
    if (checksum == wg_com_frame->checksum)
    {
        return 1; // 校验和正确
    }
    else
    {
        return 0; // 校验和错误
    }
}

void wg_com_run(void)
{
    static uint32_t rx_data_cnt = USART2_RX_BUFFER_SIZE;
    volatile uint8_t rx_data = 0;
    static uint32_t wg_com_buffer_cnt = 0;
    while (rx_data_cnt != dma_transfer_number_get(DMA0, DMA_CH2))
    {
        if (rx_data_cnt == 0)
        {
            rx_data_cnt = USART2_RX_BUFFER_SIZE;
        }
        rx_data = usart2_rx_buffer[USART2_RX_BUFFER_SIZE - rx_data_cnt];
        rx_data_cnt--;
        if (rx_data == FRAME_HEAD)
        {
            wg_com_buffer_cnt = 0;
        }
        wg_com_buffer[wg_com_buffer_cnt] = rx_data;
        wg_com_buffer_cnt++;
        if (wg_com_buffer_cnt >= MAX_FRAME_SIZE)
        {
            wg_com_buffer_cnt = 0; // 防止溢出
        }
        if (rx_data == FRAME_TAIL)
        {
            wg_com_frame_t wg_com_frame;

            wg_com_frame.soi = wg_com_buffer[0];
            wg_com_frame.addr = wg_com_buffer[1];
            wg_com_frame.length = wg_com_buffer[2];
            wg_com_frame.cid = wg_com_buffer[3];
            wg_com_frame.info = &wg_com_buffer[4];                          // INFO 起始位置
            wg_com_frame.checksum = wg_com_buffer[3 + wg_com_frame.length]; // 校验和位置
            wg_com_frame.eoi = wg_com_buffer[4 + wg_com_frame.length];      // 结束符位置
            if (((wg_com_frame.addr == HOST_ADDR) ||
                 (wg_com_frame.addr == BROADCAST_ADDR)) &&
                wg_com_checksum(&wg_com_frame))
            {
                extern reg_wg_com_t *p_wg_com_first;
                // 校验和正确，执行命令
                reg_wg_com_t *p_wg_com = p_wg_com_first;
                while (p_wg_com != NULL)
                {
                    if (p_wg_com->cmd == wg_com_frame.cid)
                    {
                        p_wg_com->p_func(&wg_com_frame); // 执行对应的函数
                        break;
                    }
                    p_wg_com = p_wg_com->p_next; // 移动到下一个命令
                }
            }
            else
            {
                // 校验和错误，处理错误
            }
        }
    }
}

REG_TASK(1, wg_com_run)

void wg_com_send(uint8_t cmd, uint8_t *data, uint32_t len)
{
    wg_com_frame_t wg_com_frame;
    wg_com_frame.soi = FRAME_HEAD;
    wg_com_frame.addr = HOST_ADDR;
    wg_com_frame.length = DEC_TO_BCD(len + 1);
    wg_com_frame.cid = cmd;
    wg_com_frame.info = data;
    wg_com_frame.checksum = wg_com_cal_sum(&wg_com_frame);

    printf("%c", wg_com_frame.soi);
    printf("%c", wg_com_frame.addr);   // 地址
    printf("%c", wg_com_frame.length); // CMD + INFO 长度
    printf("%c", cmd);
    for (int i = 0; i < len; i++)
    {
        printf("%c", data[i]);
    }
    printf("%c", wg_com_frame.checksum);
    printf("%c", FRAME_TAIL);
}
