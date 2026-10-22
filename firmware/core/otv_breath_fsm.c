/* SPDX-License-Identifier: Apache-2.0 */
#include "otv_breath_fsm.h"

/* Période réfractaire après le début de l'expiration pendant laquelle le
 * trigger est ignoré (artefacts de la vidange, REQ-PERF-008). */
#define OTV_TRIG_REFRACTORY_S 0.3f

void otv_fsm_init(otv_fsm_t *f)
{
    f->phase = OTV_PHASE_STANDBY;
    f->t_phase_s = 0.0f;
    f->t_since_insp_s = 0.0f;
    f->breath_spontaneous = false;
    f->backup_active = false;
    f->alarms = 0u;
}

static void begin_insp(otv_fsm_t *f, bool spontaneous, bool backup)
{
    f->phase = OTV_PHASE_INSP;
    f->t_phase_s = 0.0f;
    f->t_since_insp_s = 0.0f;
    f->breath_spontaneous = spontaneous;
    f->backup_active = backup;
}

static void begin_exp(otv_fsm_t *f)
{
    f->phase = OTV_PHASE_EXP;
    f->t_phase_s = 0.0f;
}

void otv_fsm_start(otv_fsm_t *f)
{
    begin_insp(f, false, false);
    f->alarms = 0u;
}

float otv_fsm_step(otv_fsm_t *f, const otv_settings_t *s,
                   float dt, float paw, float flow)
{
    if (f->phase == OTV_PHASE_STANDBY)
        return 0.0f;

    f->t_phase_s += dt;
    f->t_since_insp_s += dt;
    f->alarms &= ~OTV_ALARM_HIGH_PRESSURE; /* réévaluée à chaque pas */

    const float period_s = 60.0f / s->rate_cpm;
    const float te_s = period_s - s->ti_s;

    switch (f->phase) {
    case OTV_PHASE_INSP:
        /* Coupure pression haute : passage immédiat en expiration (VENT-01).
         * La soupape mécanique indépendante reste la barrière ultime. */
        if (paw >= s->p_max_cmh2o) {
            f->alarms |= OTV_ALARM_HIGH_PRESSURE;
            begin_exp(f);
            break;
        }
        if (f->t_phase_s >= s->ti_s)
            begin_exp(f);
        break;

    case OTV_PHASE_EXP:
        /* Trigger patient en débit, hors période réfractaire. */
        if (s->trig_flow_lpm > 0.0f &&
            f->t_phase_s >= OTV_TRIG_REFRACTORY_S &&
            flow >= s->trig_flow_lpm) {
            begin_insp(f, true, false);
            break;
        }
        /* Ventilation d'apnée (REQ-MODE-002) : prioritaire sur le cycle
         * machine si le délai d'apnée est plus court que la période. */
        if (f->t_since_insp_s >= s->apnea_delay_s) {
            f->alarms |= OTV_ALARM_APNEA;
            begin_insp(f, false, true);
            break;
        }
        /* Cycle machine. */
        if (f->t_phase_s >= te_s)
            begin_insp(f, false, false);
        break;

    case OTV_PHASE_STANDBY:
    default:
        break;
    }

    return (f->phase == OTV_PHASE_INSP)
               ? s->peep_cmh2o + s->p_insp_cmh2o
               : s->peep_cmh2o;
}
