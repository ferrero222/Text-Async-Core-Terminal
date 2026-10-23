/*******************************************************************************
 *              _____      _       ____   _____                                *
 *      ====== |_   _|    / \     / ___| |_   _| ======    (c)03.10.2025       *
 *      ======   | |     / _ \   | |       | |   ======        v1.0.0          *
 *      ======   | |    / ___ \  | |___    | |   ======                        *
 *      ======   |_|   /_/   \_\  \____|   |_|   ======                        *
 *                                                                             *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "tact_core.h"
#include "tact_mdl_general.h"

#include <stdio.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void tact_mdl_baud_rates_cb(ringslice_t rs_data, bool result, void* const data);

/*******************************************************************************
 * Local types definitions
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/*******************************************************************************
 ** @brief  Function to restart the modem
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Here is NULL
 ** @param  param  input param if function is required them. Here is
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool tact_mdl_modem_reset(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+CFUN=1,1"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 2, 150, 0, 0, NULL, NULL, TACT_NO_ARG),
  };
  if(!tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 ** @brief  Function to init modem
 ** @param  ctx    core context
 ** @param  cb     cb when proc will be done. Can be NULL
 ** @param  param  input param if function is required them. Here is NULL
 ** @param  meta   Meta data of function execution. Will be passe to the cb by the
 **                end of execution. Can be NULL
 ** @return true - proc started, false - smthg is wrong
 ******************************************************************************/
bool tact_mdl_modem_init(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  tact_item_t items[] =
  {
    TACT_ITEM("AT"TACT_CMD_CRLF,         NULL,        TACT_PARCE_SIMCOM, 5, 100, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT"TACT_CMD_CRLF,         NULL,        TACT_PARCE_SIMCOM, 5, 100, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("ATE1"TACT_CMD_CRLF,       NULL,        TACT_PARCE_SIMCOM, 5, 100, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+CSCLK=0"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW,    2, 300, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+IFC=2,2"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW,    2, 300, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT&D2"TACT_CMD_CRLF,      TACT_CMD_OK, TACT_PARCE_RAW,    2, 300, 0, 0, NULL, NULL, TACT_NO_ARG),
  };
  if(!tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0, meta)) return false;
  return true;
}

/*******************************************************************************
 * @brief  Set the modem functionality level.
 * @details Submits AT+CFUN=<level> and waits for the modem's final OK response.
 *          This deactivates the SIM interface before switching the board's
 *          external SIM mux, then restores full modem functionality.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_functionality_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The command was queued.
 * @retval false The level is unsupported or TACT rejected the command.
 ******************************************************************************/
bool tact_mdl_functionality(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  const tact_mdl_functionality_t *functionality = (const tact_mdl_functionality_t *)param;
  char command[32];
  int length;
  if(!ctx || !tact_get_init(ctx).init || !functionality) return false;
  if((functionality->level != 0U) && (functionality->level != 1U) && (functionality->level != 4U) && (functionality->level != 5U) && (functionality->level != 6U) && (functionality->level != 7U) && (functionality->level != 8U)) return false;
  length = snprintf(command, sizeof(command), "%sAT+CFUN=%u%s", TACT_CMD_SAVE, (unsigned)functionality->level, TACT_CMD_CRLF);
  if((length < 0) || ((size_t)length >= sizeof(command))) return false;
  tact_item_t items[] =
  {
    TACT_ITEM(command, TACT_CMD_OK, TACT_PARCE_RAW, 2, 900, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Query the list of supported temporary modem baud rates.
 * @details TACT owns the response buffer until completion; the answer parser
 *          reduces its numeric list to one supported board rate.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback receiving the selected speed.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to cb.
 * @retval true The command was queued.
 * @retval false TACT could not queue the command.
 ******************************************************************************/
bool tact_mdl_baud_rates(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+IPR=?"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 2, 900, 0, 0, tact_mdl_baud_rates_cb, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, sizeof(tact_mdl_baud_rate_t), meta);
}

/*
 * @brief  Extract the fastest board-supported rate from an AT+IPR test reply.
 * @details Scans only the parenthesized speed list and ignores numbers in the
 *          echoed command or unrelated response lines. The temporary command
 *          supports 921600 as the board's current streaming ceiling.
 * @param[in] rs_data Bounded TACT response slice.
 * @param[in] result true when the response parser accepted the command.
 * @param[out] data TACT-owned tact_mdl_baud_rate_t callback storage.
 */
static void tact_mdl_baud_rates_cb(ringslice_t rs_data, bool result, void* const data)
{
  tact_mdl_baud_rate_t *rate = (tact_mdl_baud_rate_t *)data;
  ringslice_t prefix;
  uint32_t candidate = 0U;
  bool in_list = false;
  if(!result || !rate || ringslice_is_empty(&rs_data)) return;
  prefix = ringslice_strstr(&rs_data, "+IPR:");
  if(ringslice_is_empty(&prefix)) return;
  rs_data = ringslice_subslice_after(&rs_data, &prefix, 0U);
  for(ringslice_cnt_t i = 0U; i < ringslice_len(&rs_data); i++)
  {
    const uint8_t ch = ringslice_nth_byte(&rs_data, i);
    if(ch == '(') { in_list = true; candidate = 0U; continue; }
    if(ch == ')') break;
    if(!in_list) continue;
    if((ch >= '0') && (ch <= '9'))
    {
      if(candidate <= 10000000U) candidate = candidate * 10U + (uint32_t)(ch - '0');
    }
    else if(candidate != 0U)
    {
      if((candidate <= 921600U) && (candidate > rate->baud_rate)) rate->baud_rate = candidate;
      candidate = 0U;
    }
  }
  if((candidate <= 921600U) && (candidate > rate->baud_rate)) rate->baud_rate = candidate;
}

/*******************************************************************************
 * @brief  Temporarily change the SIMCom UART speed.
 * @details The modem and host UART must be switched in the same startup chain
 *          before further AT commands are issued.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback.
 * @param[in] param Pointer to tact_mdl_baud_rate_t with the selected rate.
 * @param[in] meta Caller value forwarded to cb.
 * @retval true The command was queued.
 * @retval false The rate is invalid or TACT rejected the command.
 ******************************************************************************/
bool tact_mdl_baud_set(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  const tact_mdl_baud_rate_t *rate = (const tact_mdl_baud_rate_t *)param;
  char command[32];
  int length;
  if(!ctx || !tact_get_init(ctx).init || !rate || (rate->baud_rate < 115200U) || (rate->baud_rate > 921600U)) return false;
  length = snprintf(command, sizeof(command), "%sAT+IPR=%lu%s", TACT_CMD_SAVE, (unsigned long)rate->baud_rate, TACT_CMD_CRLF);
  if((length < 0) || ((size_t)length >= sizeof(command))) return false;
  tact_item_t items[] =
  {
    TACT_ITEM(command, TACT_CMD_OK, TACT_PARCE_RAW, 1, 900, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Queue the three AT commands needed for modem identification.
 * @details Keeps identity discovery independent of SIM and radio availability.
 *          The parsed record is valid only during the completion callback.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback.
 * @param[in] param Unused.
 * @param[in] meta Caller value forwarded to cb.
 * @retval true Identity commands were queued.
 * @retval false TACT could not queue the commands.
 ******************************************************************************/
bool tact_mdl_rtd_identity(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+CGSN"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "%15[^\x0d]", TACT_ARG(tact_mdl_rtd_identity_t, modem_imei)),
    TACT_ITEM("AT+CGMM"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "%15[^\x0d]", TACT_ARG(tact_mdl_rtd_identity_t, modem_model)),
    TACT_ITEM("AT+CGMR"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "+CGMR: %29[^\x0d]", TACT_ARG(tact_mdl_rtd_identity_t, modem_revision))
  };
  return tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(tact_mdl_rtd_identity_t), meta);
}

/*******************************************************************************
 * @brief  Check that the modem has returned to command mode.
 * @details Sends one AT command and completes after the modem replies OK.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to cb.
 * @retval true The command was queued.
 * @retval false TACT is unavailable or the queue is full.
 ******************************************************************************/
bool tact_mdl_at_test(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("AT"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 1, 500, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
}
