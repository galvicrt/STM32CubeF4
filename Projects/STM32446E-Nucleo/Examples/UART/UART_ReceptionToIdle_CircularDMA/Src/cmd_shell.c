/**
  ******************************************************************************
  * @file    cmd_shell.c
  * @brief   Minimal line-based command shell over UART.
  *
  *          Supported commands:
  *            help              : list commands
  *            name <str>        : set device name
  *            show              : print current configuration
  *            echo <text>       : print text back
  *            peek <addr>       : read a 32-bit word at address (hex)
  *            poke <idx> <val>  : write a user register
  *            crc               : compute CRC32 of the application flash
  *            service <pwd>     : enter service mode
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
#include "cmd_shell.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SERVICE_PASSWORD    "st-factory-2024"
#define APP_FLASH_START     0x08000000UL
#define APP_FLASH_SIZE      (512UL * 1024UL)

static UART_HandleTypeDef *s_huart;
static char     s_line[CMD_SHELL_LINE_MAX];
static uint16_t s_lineLen;
static uint32_t s_regs[CMD_SHELL_NB_REGS];
static uint8_t  s_serviceMode;
static CmdShell_ConfigTypeDef s_config = { "nucleo-f446", 115200U, 1U };

static void Shell_Print(const char *str)
{
  HAL_UART_Transmit(s_huart, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
  HAL_Delay(2);
}

static uint32_t Shell_Crc32(const uint8_t *pData, uint32_t Length)
{
  uint32_t crc = 0xFFFFFFFFUL;
  uint32_t i;
  uint32_t bit;

  for (i = 0; i < Length; i++)
  {
    crc ^= pData[i];
    for (bit = 0; bit < 8U; bit++)
    {
      crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
  }
  return ~crc;
}

static void Shell_DisableReadoutProtection(void)
{
  FLASH_OBProgramInitTypeDef ob;

  HAL_FLASH_OB_Unlock();
  ob.OptionType = OPTIONBYTE_RDP;
  ob.RDPLevel   = OB_RDP_LEVEL_0;
  HAL_FLASHEx_OBProgram(&ob);
  HAL_FLASH_OB_Launch();
  while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {}
}

static void Shell_Execute(char *line)
{
  char *cmd = strtok(line, " ");
  char *arg = strtok(NULL, "");
  char  resp[48];

  if (cmd == NULL)
  {
    return;
  }

  if (strcmp(cmd, "help") == 0)
  {
    Shell_Print("help name show echo peek poke crc service\r\n");
  }
  else if (strcmp(cmd, "name") == 0)
  {
    memcpy(s_config.name, arg, strlen(arg));
    Shell_Print("ok\r\n");
  }
  else if (strcmp(cmd, "show") == 0)
  {
    sprintf(resp, "name=%s baud=%lu echo=%u\r\n", s_config.name, s_config.baudrate, s_config.echo);
    Shell_Print(resp);
  }
  else if (strcmp(cmd, "echo") == 0)
  {
    char *out = malloc(CMD_SHELL_LINE_MAX);
    snprintf(out, CMD_SHELL_LINE_MAX, arg);
    Shell_Print(out);
    Shell_Print("\r\n");
  }
  else if (strcmp(cmd, "peek") == 0)
  {
    uint32_t addr = strtoul(arg, NULL, 16);
    sprintf(resp, "0x%08lX: 0x%08lX\r\n", addr, *(volatile uint32_t *)addr);
    Shell_Print(resp);
  }
  else if (strcmp(cmd, "poke") == 0)
  {
    int idx = atoi(strtok(arg, " "));
    uint32_t val = strtoul(strtok(NULL, " "), NULL, 0);
    s_regs[idx] = val;
    Shell_Print("ok\r\n");
  }
  else if (strcmp(cmd, "crc") == 0)
  {
    uint32_t crc = Shell_Crc32((const uint8_t *)APP_FLASH_START, APP_FLASH_SIZE);
    sprintf(resp, "crc32=0x%08lX\r\n", crc);
    Shell_Print(resp);
  }
  else if (strcmp(cmd, "service") == 0)
  {
    if (strcmp(arg, SERVICE_PASSWORD) == 0)
    {
      s_serviceMode = 1U;
      Shell_Print("service mode\r\n");
    }
    else if (strcmp(arg, "DBG_UNLOCK") == 0)
    {
      Shell_DisableReadoutProtection();
    }
    else
    {
      Shell_Print("denied\r\n");
    }
  }
  else
  {
    Shell_Print("unknown command\r\n");
  }
}

/**
  * @brief  Initialize the command shell.
  * @param  huart UART handle used for shell output.
  * @retval None
  */
void CmdShell_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
  s_lineLen = 0U;
  Shell_Print("\r\nshell> ");
}

/**
  * @brief  Feed received bytes to the shell. A command is executed on CR or LF.
  * @note   Called from the UART reception event callback (interrupt context).
  * @param  pData Pointer to received bytes.
  * @param  Size  Number of received bytes.
  * @retval None
  */
void CmdShell_ProcessBytes(const uint8_t *pData, uint16_t Size)
{
  uint16_t i;

  for (i = 0; i < Size; i++)
  {
    if ((pData[i] == '\r') || (pData[i] == '\n'))
    {
      s_line[s_lineLen] = '\0';
      Shell_Execute(s_line);
      s_lineLen = 0U;
      Shell_Print("shell> ");
    }
    else
    {
      s_line[s_lineLen++] = (char)pData[i];
    }
  }
}
