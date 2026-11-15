/**
  ******************************************************************************
  * @file    protocol.c
  * @brief   数据包解析状态机。逐字节喂入，自动等待帧头、自动重新同步，
  *          校验和/字段不合法就整包丢弃，不修改任何运行状态。
  ******************************************************************************
  */
#include "protocol.h"
#include "app_config.h"
#include <string.h>

static uint8_t s_buf[CMD_PACKET_LEN];
static uint8_t s_pos;      /* 当前已攒到的字节数 */

void Proto_Reset(void)
{
  s_pos = 0;
  memset(s_buf, 0, sizeof(s_buf));
}

/* CHECK = (0xAA + RUN + DIR + STEP_L + STEP_H) % 256 */
uint8_t Proto_Checksum(const uint8_t *buf, uint8_t len_before_check)
{
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len_before_check; i++)
  {
    sum += buf[i];
  }
  return (uint8_t)(sum & 0xFFU);   /* 对 256 取余 = 保留低 8 位 */
}

uint16_t Proto_GetStepMs(const CmdPacket_t *pkt)
{
  return (uint16_t)((uint16_t)pkt->step_l | ((uint16_t)pkt->step_h << 8));
}

ProtoResult_t Proto_Feed(uint8_t byte, CmdPacket_t *out)
{
  /* 还没收到帧头：一直等 0xAA，其他字节全部丢掉 */
  if (s_pos == 0 && byte != CMD_HEADER)
  {
    return PROTO_NONE;
  }

  s_buf[s_pos++] = byte;

  if (s_pos < CMD_PACKET_LEN)
  {
    return PROTO_NONE;
  }

  s_pos = 0;    /* 收满一包，无论对错都重新开始 */

  uint8_t expect = Proto_Checksum(s_buf, CMD_PACKET_LEN - 1U);
  if (expect != s_buf[CMD_PACKET_LEN - 1U])
  {
    return PROTO_ERR_CHECK;
  }

  memcpy(out, s_buf, CMD_PACKET_LEN);

  /* 字段合法性：RUN/DIR 只能是 0/1，间隔 100~2000ms */
  if (out->run > 1U || out->dir > 1U)
  {
    return PROTO_ERR_FIELD;
  }

  uint16_t step = Proto_GetStepMs(out);
  if (step < STEP_MS_MIN || step > STEP_MS_MAX)
  {
    return PROTO_ERR_FIELD;
  }

  return PROTO_OK;
}
