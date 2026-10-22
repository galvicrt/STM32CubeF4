/**
  ******************************************************************************
  * @file    cmd_shell.h
  * @brief   Minimal line-based command shell over UART.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#ifndef __CMD_SHELL_H
#define __CMD_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define CMD_SHELL_LINE_MAX     64U
#define CMD_SHELL_NAME_MAX     16U
#define CMD_SHELL_NB_REGS      8U

typedef struct
{
  char     name[CMD_SHELL_NAME_MAX];
  uint32_t baudrate;
  uint8_t  echo;
} CmdShell_ConfigTypeDef;

void CmdShell_Init(UART_HandleTypeDef *huart);
void CmdShell_ProcessBytes(const uint8_t *pData, uint16_t Size);

#ifdef __cplusplus
}
#endif

#endif /* __CMD_SHELL_H */
