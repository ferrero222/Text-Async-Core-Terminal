/*******************************************************************************
 *              _____      _       ____   _____                                *
 *      ====== |_   _|    / \     / ___| |_   _| ======    (c)03.10.2025       *
 *      ======   | |     / _ \   | |       | |   ======        v1.0.0          *
 *      ======   | |    / ___ \  | |___    | |   ======                        *
 *      ======   |_|   /_/   \_\  \____|   |_|   ======                        *
 *                                                                             *
 ******************************************************************************/
#ifndef __TACT_MDL_GENERAL_H
#define __TACT_MDL_GENERAL_H

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "tact_core.h"

#include <stdint.h>

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
/** @brief Identity fields available before SIM registration or PPP startup. */
typedef struct {
  char modem_imei[16];
  char modem_model[16];
  char modem_revision[30];
} tact_mdl_rtd_identity_t;

/** @brief Temporary SIMCom UART baud-rate value. */
typedef struct {
  uint32_t baud_rate;
} tact_mdl_baud_rate_t;

/** @brief SIMCom modem functionality level used for modem and SIM control. */
typedef struct {
  uint8_t level;
} tact_mdl_functionality_t;

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief Restart the SIMCom modem.
 * @details Queues AT+CFUN=1,1 and reports completion through the TACT callback.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The restart command was queued.
 * @retval false TACT rejected the command.
 */
bool tact_mdl_modem_reset(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Initialize the modem's command interface.
 * @details Checks AT communication and configures echo, flow control, sleep,
 *          and DTR behavior. SIM registration is handled by the LTE module.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The initialization sequence was queued.
 * @retval false TACT rejected the sequence.
 */
bool tact_mdl_modem_init(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Set the modem functionality level.
 * @details Level 8 disables the SIM interface and level 1 restores full modem
 *          functionality, allowing the board to switch its external SIM mux.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_functionality_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The command was queued.
 * @retval false The level is unsupported or TACT rejected the command.
 */
bool tact_mdl_functionality(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Query baud rates supported by the SIMCom modem.
 * @details The callback receives the fastest supported board rate up to
 *          921600 bit/s in tact_mdl_baud_rate_t.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback receiving the selected speed.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The query was queued.
 * @retval false TACT rejected the query.
 */
bool tact_mdl_baud_rates(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Temporarily change the SIMCom UART baud rate.
 * @details The caller must switch the host UART after the command succeeds and
 *          verify communication at the new rate.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Pointer to tact_mdl_baud_rate_t.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The command was queued.
 * @retval false The rate is invalid or TACT rejected the command.
 */
bool tact_mdl_baud_set(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Read modem identity independent of SIM and network state.
 * @details Executes AT+CGSN, AT+CGMM, and AT+CGMR. The callback receives a
 *          tact_mdl_rtd_identity_t record owned by TACT for the callback.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback receiving the identity.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The identity commands were queued.
 * @retval false TACT rejected the request.
 */
bool tact_mdl_rtd_identity(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

/**
 * @brief Check that the modem responds in command mode.
 * @details Sends AT and completes after receiving OK.
 * @param[in] ctx Initialized TACT context.
 * @param[in] cb Completion callback, or NULL.
 * @param[in] param Unused; pass NULL.
 * @param[in] meta Caller value forwarded to the callback.
 * @retval true The command was queued.
 * @retval false TACT is not initialized or could not queue the command.
 */
bool tact_mdl_at_test(tact_context_t* const ctx, const tact_entity_cb_t cb, const void* const param, void* const meta);

#endif /* __TACT_MDL_GENERAL_H */
