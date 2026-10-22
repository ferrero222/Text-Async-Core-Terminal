/*******************************************************************************
 *              _____      _       ____   _____                                *
 *      ====== |_   _|    / \     / ___| |_   _| ======    (c)15.10.2026       *
 *      ======   | |     / _ \   | |       | |   ======        v1.0.0          *
 *      ======   | |    / ___ \  | |___    | |   ======                        *
 *      ======   |_|   /_/   \_\  \____|   |_|   ======                        *
 *                                                                             *
 ******************************************************************************/
#ifndef __TACT_MDL_LTE_H
#define __TACT_MDL_LTE_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "tact_core.h"

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
/** @brief Parsed serving or neighboring LTE cell information. */
typedef struct {
  int cell;
  int mcc;
  int mnc;
  int lac;
  int cell_id;
} tact_mdl_lte_rtd_lbs_t;

/** @brief SIM, network, modem, and neighboring-cell runtime information. */
typedef struct {
  char modem_imei[16];
  char modem_id[16];
  char modem_rev[30];
  char modem_clock[22];
  char sim_iccid[22];
  char sim_operator[50];
  int sim_rssi;
  tact_mdl_lte_rtd_lbs_t modem_lbs[7];
  int lbs_cnt;
} tact_mdl_lte_rtd_t;

/** @brief Parsed SIM lock state returned by AT+CPIN?. */
typedef struct {
  char state[16];
} tact_mdl_lte_sim_state_t;

/** @brief SIM PIN supplied when the selected SIM is PIN-locked. */
typedef struct {
  const char *pin;
} tact_mdl_lte_sim_pin_t;

/** @brief APN used to configure PDP context one. */
typedef struct {
  const char *apn;
} tact_mdl_lte_pdp_context_t;

/** @brief Optional PAP credentials used by PDP context one. */
typedef struct {
  const char *user;
  const char *password;
} tact_mdl_lte_pdp_auth_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief Read the current SIM lock state.
 * @details Stores the modem's CPIN response, such as READY or SIM PIN, in the
 *          callback data. The returned data is owned by TACT during the callback.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The query was queued.
 * @retval false TACT is unavailable or could not queue the query.
 */
bool tact_mdl_lte_sim_state(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Check that the selected SIM is unlocked and ready.
 * @details Completes successfully only for the modem response "+CPIN: READY".
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The query was queued.
 * @retval false TACT is unavailable or could not queue the query.
 */
bool tact_mdl_lte_sim_ready(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Submit a configured SIM PIN and verify the resulting SIM state.
 * @details Disables command echo, submits the PIN, and queries CPIN again. Echo
 *          is restored by the next modem initialization after a power cycle.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_lte_sim_pin_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The PIN sequence was queued.
 * @retval false The PIN is invalid or TACT rejected the sequence.
 */
bool tact_mdl_lte_sim_unlock(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Configure LTE registration and wait for packet-network attachment.
 * @details Disables unsolicited CEREG reporting, enables automatic operator
 *          selection when needed, waits for home or roaming registration, then
 *          confirms packet attachment with AT+CGATT?. A registration failure is
 *          reported to the completion callback after the bounded retries.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The LTE initialization sequence was queued.
 * @retval false TACT is unavailable or could not queue the sequence.
 */
bool tact_mdl_lte_init(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Configure PDP context one with the selected SIM's APN.
 * @details TACT copies the generated AT command into its queue before this call
 *          returns, so the caller may retain its settings normally.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_lte_pdp_context_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The PDP context command was queued.
 * @retval false The APN is invalid or TACT rejected the command.
 */
bool tact_mdl_lte_pdp_context(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/*******************************************************************************
 * @brief  Configure authentication for PDP context one.
 * @details Uses PAP when both credentials are non-empty, or disables stored
 *          authentication when both are empty. Echo is disabled while sending
 *          credentials and restored on success so RX logs do not expose them.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_lte_pdp_auth_t with terminated strings.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The authentication sequence was queued.
 * @retval false Credentials are invalid or TACT rejected the sequence.
 ******************************************************************************/
bool tact_mdl_lte_pdp_auth(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Start the modem's packet-data call.
 * @details Completes when the modem reports CONNECT; following PPP bytes remain
 *          available in the shared UART receive ring.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The data-call command was queued.
 * @retval false TACT is unavailable or could not queue the command.
 */
bool tact_mdl_lte_data_call(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Read modem, SIM, network, and neighboring-cell runtime information.
 * @details The callback receives a tact_mdl_lte_rtd_t record while TACT owns it.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback receiving the parsed record.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The runtime-data commands were queued.
 * @retval false TACT rejected the request.
 */
bool tact_mdl_lte_rtd(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

#endif /* __TACT_MDL_LTE_H */
