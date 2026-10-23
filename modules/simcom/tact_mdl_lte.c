/*******************************************************************************
 *              _____      _       ____   _____                                *
 *      ====== |_   _|    / \     / ___| |_   _| ======    (c)15.10.2026       *
 *      ======   | |     / _ \   | |       | |   ======        v1.0.0          *
 *      ======   | |    / ___ \  | |___    | |   ======                        *
 *      ======   |_|   /_/   \_\  \____|   |_|   ======                        *
 *                                                                             *
 ******************************************************************************/
/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "tact_mdl_lte.h"

#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
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
 * @brief  Read the current SIM lock state.
 * @details The parsed CPIN value is stored in TACT-owned callback data.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The query was queued.
 * @retval false TACT is unavailable or could not queue the query.
 ******************************************************************************/
bool tact_mdl_lte_sim_state(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+CPIN?"TACT_CMD_CRLF, "+CPIN", TACT_PARCE_SIMCOM, 2, 300, 0, 0, NULL, "+CPIN: %15[^\r\n]", TACT_ARG(tact_mdl_lte_sim_state_t, state))
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, sizeof(tact_mdl_lte_sim_state_t), meta);
}

/*******************************************************************************
 * @brief  Check that the selected SIM is unlocked and ready.
 * @details Completes successfully only for the modem response "+CPIN: READY".
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The query was queued.
 * @retval false TACT is unavailable or could not queue the query.
 ******************************************************************************/
bool tact_mdl_lte_sim_ready(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+CPIN?"TACT_CMD_CRLF, "+CPIN: READY", TACT_PARCE_SIMCOM, 10, 300, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Submit one SIM PIN and query the resulting SIM state.
 * @details Echo is disabled before the PIN command and remains disabled for the
 *          current modem session. The next modem initialization restores echo.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_lte_sim_pin_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The PIN transaction was queued.
 * @retval false The PIN is invalid or TACT rejected the transaction.
 ******************************************************************************/
bool tact_mdl_lte_sim_unlock(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  const tact_mdl_lte_sim_pin_t *configuration = (const tact_mdl_lte_sim_pin_t *)param;
  const char *terminator;
  size_t pin_length;
  char command[32];
  int command_length;
  if(!ctx || !tact_get_init(ctx).init || !configuration || !configuration->pin) return false;
  terminator = (const char *)memchr(configuration->pin, '\0', 9U);
  if(!terminator) return false;
  pin_length = (size_t)(terminator - configuration->pin);
  if(pin_length < 4U || pin_length > 8U) return false;
  for(size_t index = 0U; index < pin_length; index++) if(configuration->pin[index] < '0' || configuration->pin[index] > '9') return false;
  command_length = snprintf(command, sizeof(command), "%sAT+CPIN=\"%s\"%s", TACT_CMD_SAVE, configuration->pin, TACT_CMD_CRLF);
  if((command_length < 0) || ((size_t)command_length >= sizeof(command))) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("ATE0"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 0, 300, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM(command, TACT_CMD_OK, TACT_PARCE_RAW, 0, 1200, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+CPIN?"TACT_CMD_CRLF, "+CPIN", TACT_PARCE_SIMCOM, 10, 300, 0, 0, NULL, "+CPIN: %15[^\r\n]", TACT_ARG(tact_mdl_lte_sim_state_t, state))
  };
  return tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(tact_mdl_lte_sim_state_t), meta);
}

/*******************************************************************************
 * @brief  Initialize automatic LTE network selection and packet attachment.
 * @details TACT item delays are specified in 10 ms units. Operator selection
 *          may take up to 60 seconds; registration and packet attachment are
 *          polled with bounded retry counts before reporting failure.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The LTE initialization sequence was queued.
 * @retval false TACT is unavailable or could not queue the sequence.
 ******************************************************************************/
bool tact_mdl_lte_init(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+CEREG=0"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 2, 300, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+COPS?"TACT_CMD_CRLF, "+COPS: 0", TACT_PARCE_SIMCOM, 2, 300, 1, 2, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+COPS=0"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 1, 6000, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+CEREG?"TACT_CMD_CRLF, "+CEREG: 0,1|+CEREG: 0,5", TACT_PARCE_SIMCOM, 20, 900, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+CGATT?"TACT_CMD_CRLF, "+CGATT: 1", TACT_PARCE_SIMCOM, 10, 900, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Configure PDP context one using the selected SIM's APN.
 * @details The bounded APN is copied into a temporary command and then into
 *          TACT-owned storage before this function returns.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_lte_pdp_context_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The command was queued.
 * @retval false The APN is invalid or TACT could not queue the command.
 ******************************************************************************/
bool tact_mdl_lte_pdp_context(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  const tact_mdl_lte_pdp_context_t *configuration = (const tact_mdl_lte_pdp_context_t *)param;
  const char *terminator;
  char command[96];
  int command_length;
  if(!ctx || !tact_get_init(ctx).init || !configuration || !configuration->apn) return false;
  terminator = (const char *)memchr(configuration->apn, '\0', 64U);
  if(!terminator) return false;
  command_length = snprintf(command, sizeof(command), "%sAT+CGDCONT=1,\"IP\",\"%s\"%s", TACT_CMD_SAVE, configuration->apn, TACT_CMD_CRLF);
  if((command_length < 0) || ((size_t)command_length >= sizeof(command))) return false;
  tact_item_t items[] =
  {
    TACT_ITEM(command, TACT_CMD_OK, TACT_PARCE_RAW, 2, 300, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Configure authentication for PDP context one.
 * @details A76XX expects the password before the user name in AT+CGAUTH. Empty
 *          credentials clear the modem's saved authentication. PAP credentials
 *          are sent with echo disabled, then normal command echo is restored.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_lte_pdp_auth_t with terminated strings.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The authentication sequence was queued.
 * @retval false Credentials are invalid or TACT rejected the sequence.
 ******************************************************************************/
bool tact_mdl_lte_pdp_auth(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  const tact_mdl_lte_pdp_auth_t *configuration = (const tact_mdl_lte_pdp_auth_t *)param;
  char command[144];
  int command_length;
  if(!ctx || !tact_get_init(ctx).init || !configuration || !configuration->user || !configuration->password) return false;
  if((configuration->user[0] == '\0') && (configuration->password[0] == '\0'))
  {
    tact_item_t items[] =
    {
      TACT_ITEM("AT+CGAUTH=1,0"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 2, 900, 0, 0, NULL, NULL, TACT_NO_ARG)
    };
    return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
  }
  if((configuration->user[0] == '\0') || (configuration->password[0] == '\0')) return false;
  if((strlen(configuration->user) > 50U) || (strlen(configuration->password) > 50U)) return false;
  if(strpbrk(configuration->user, "\"\r\n") || strpbrk(configuration->password, "\"\r\n")) return false;
  command_length = snprintf(command, sizeof(command), "%sAT+CGAUTH=1,1,\"%s\",\"%s\"%s", TACT_CMD_SAVE, configuration->password, configuration->user, TACT_CMD_CRLF);
  if((command_length < 0) || ((size_t)command_length >= sizeof(command))) return false;
  tact_item_t items[] =
  {
    // Prevent modem echo from copying credentials into the ordinary RX logs.
    TACT_ITEM("ATE0"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 1, 300, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM(command, TACT_CMD_OK, TACT_PARCE_RAW, 2, 900, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("ATE1"TACT_CMD_CRLF, TACT_CMD_OK, TACT_PARCE_RAW, 2, 300, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Start the modem's packet-data call.
 * @details TACT leaves bytes following the CONNECT line in the shared UART
 *          receive ring for the caller's binary stream handler.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The dial command was queued.
 * @retval false TACT is unavailable or could not queue the command.
 ******************************************************************************/
bool tact_mdl_lte_data_call(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("ATD*99***1#"TACT_CMD_CRLF, "CONNECT", TACT_PARCE_SIMCOM, 1, 3000, 0, 0, NULL, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, 1U, cb, 0U, meta);
}

/*******************************************************************************
 * @brief  Parse neighboring-cell information from the CENG response.
 * @details The first reported entry is a header; valid subsequent entries are
 *          copied into the bounded LTE runtime-data array.
 * @param[in] rs_data Bounded CENG response slice.
 * @param[in] result true when TACT accepted the response.
 * @param[out] data TACT-owned tact_mdl_lte_rtd_t callback storage.
 ******************************************************************************/
static void tact_mdl_lte_ceng_cb(ringslice_t rs_data, bool result, void* const data)
{
  bool header_skip = false;
  tact_mdl_lte_rtd_t *rtd = (tact_mdl_lte_rtd_t *)data;
  if(!result || !rtd || ringslice_is_empty(&rs_data)) return;
  while((rtd->lbs_cnt < 7) && ringslice_scanf(&rs_data, "+CENG: %d,\"%d,%d,%x,%x,",
    &rtd->modem_lbs[rtd->lbs_cnt].cell, &rtd->modem_lbs[rtd->lbs_cnt].mcc,
    &rtd->modem_lbs[rtd->lbs_cnt].mnc, &rtd->modem_lbs[rtd->lbs_cnt].lac,
    &rtd->modem_lbs[rtd->lbs_cnt].cell_id))
  {
    if(header_skip && (rtd->modem_lbs[rtd->lbs_cnt].cell_id != 0) && (rtd->modem_lbs[rtd->lbs_cnt].lac != 0)) ++rtd->lbs_cnt;
    if(rtd->lbs_cnt >= 7) break;
    ringslice_t line_end = ringslice_strstr(&rs_data, header_skip ? "\r\n" : "\r\n\r\n");
    if(ringslice_is_empty(&line_end)) break;
    rs_data = ringslice_subslice_after(&rs_data, &line_end, 0U);
    if(ringslice_is_empty(&rs_data)) break;
    header_skip = true;
  }
}

/*******************************************************************************
 * @brief  Read modem, SIM, network, and neighboring-cell runtime information.
 * @details The parsed record is owned by TACT and is valid in the completion
 *          callback. Cellular and CENG values may be unavailable before attach.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback receiving the parsed record.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The runtime-data commands were queued.
 * @retval false TACT is unavailable or rejected the request.
 ******************************************************************************/
bool tact_mdl_lte_rtd(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta)
{
  (void)param;
  if(!ctx || !tact_get_init(ctx).init) return false;
  tact_item_t items[] =
  {
    TACT_ITEM("AT+CGSN"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "%15[^\x0d]", TACT_ARG(tact_mdl_lte_rtd_t, modem_imei)),
    TACT_ITEM("AT+CGMM"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "%15[^\x0d]", TACT_ARG(tact_mdl_lte_rtd_t, modem_id)),
    TACT_ITEM("AT+CGMR"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "+CGMR: %29[^\x0d]", TACT_ARG(tact_mdl_lte_rtd_t, modem_rev)),
    TACT_ITEM("AT+CCLK?"TACT_CMD_CRLF, "+CCLK", TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "+CCLK: \"%21[^\"]\"", TACT_ARG(tact_mdl_lte_rtd_t, modem_clock)),
    TACT_ITEM("AT+CCID"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "%21[^\x0d]", TACT_ARG(tact_mdl_lte_rtd_t, sim_iccid)),
    TACT_ITEM("AT+COPS?"TACT_CMD_CRLF, "+COPS", TACT_PARCE_SIMCOM, 20, 100, 0, 1, NULL, "+COPS: 0, 0,\"%49[^\"]\"", TACT_ARG(tact_mdl_lte_rtd_t, sim_operator)),
    TACT_ITEM("AT+CSQ"TACT_CMD_CRLF, "+CSQ", TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, "+CSQ: %d", TACT_ARG(tact_mdl_lte_rtd_t, sim_rssi)),
    TACT_ITEM("AT+CENG=3"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 1, NULL, NULL, TACT_NO_ARG),
    TACT_ITEM("AT+CENG?"TACT_CMD_CRLF, NULL, TACT_PARCE_SIMCOM, 10, 100, 0, 0, tact_mdl_lte_ceng_cb, NULL, TACT_NO_ARG)
  };
  return tact_entity_enqueue(ctx, items, sizeof(items)/sizeof(items[0]), cb, sizeof(tact_mdl_lte_rtd_t), meta);
}
