/**
  ******************************************************************************
  * @file    protocol.h
  * @brief   上位机指令包解析：AA RUN DIR STEP_L STEP_H CHECK（6 字节）
  ******************************************************************************
  */
#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>

/* 按字节流直接映射的结构体，方便按字段取用 */
typedef struct __attribute__((packed))
{
  uint8_t header;     /* 0xAA */
  uint8_t run;        /* 0x00 暂停 / 0x01 运行 */
  uint8_t dir;        /* 0x00 正向 / 0x01 反向 */
  uint8_t step_l;     /* 间隔低字节 */
  uint8_t step_h;     /* 间隔高字节 */
  uint8_t check;      /* 校验和 */
} CmdPacket_t;

typedef enum
{
  PROTO_NONE = 0,     /* 还没凑够一包，继续喂字节 */
  PROTO_OK,           /* 收到完整包且校验、字段都合法 */
  PROTO_ERR_CHECK,    /* 校验和不通过，已丢弃 */
  PROTO_ERR_FIELD     /* 校验通过但字段非法（如 RUN=2、间隔超范围） */
} ProtoResult_t;

void          Proto_Reset(void);
ProtoResult_t Proto_Feed(uint8_t byte, CmdPacket_t *out);
uint8_t       Proto_Checksum(const uint8_t *buf, uint8_t len_before_check);
uint16_t      Proto_GetStepMs(const CmdPacket_t *pkt);

#endif /* __PROTOCOL_H */
