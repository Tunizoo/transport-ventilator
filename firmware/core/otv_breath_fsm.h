/* OTV — machine d'états respiratoire, mode PC/PAC (v0).
 * Porte : le cadencement des cycles (REQ-PERF-002), le trigger patient
 * (REQ-PERF-008), la ventilation d'apnée (REQ-MODE-002) et la coupure
 * pression haute (REQ-PERF-004, VENT-01).
 * C portable, pas d'allocation, pas de dépendance.
 * SPDX-License-Identifier: Apache-2.0 */
#ifndef OTV_BREATH_FSM_H
#define OTV_BREATH_FSM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    OTV_PHASE_STANDBY = 0,
    OTV_PHASE_INSP,
    OTV_PHASE_EXP,
} otv_phase_t;

/* Alarmes émises par le cœur (sous-ensemble v0 ; REQ-ALRM-001 : générées ici,
 * jamais par l'IHM). */
#define OTV_ALARM_HIGH_PRESSURE (1u << 0)
#define OTV_ALARM_APNEA         (1u << 1)

typedef struct {
    float rate_cpm;      /* fréquence machine, c/min (2–60) */
    float ti_s;          /* temps inspiratoire, s */
    float p_insp_cmh2o;  /* pression inspiratoire AU-DESSUS de la PEEP */
    float peep_cmh2o;    /* PEEP */
    float trig_flow_lpm; /* seuil de trigger en débit ; <= 0 désactive */
    float apnea_delay_s; /* délai d'apnée (15–60 s) */
    float p_max_cmh2o;   /* limite logicielle (REQ-PERF-004), ex. 60 */
} otv_settings_t;

typedef struct {
    otv_phase_t phase;
    float t_phase_s;        /* temps écoulé dans la phase courante */
    float t_since_insp_s;   /* temps depuis le début de la dernière inspiration */
    bool  breath_spontaneous; /* l'inspiration courante vient d'un trigger */
    bool  backup_active;      /* ventilation d'apnée engagée */
    uint32_t alarms;          /* bitfield OTV_ALARM_* (non verrouillé en v0) */
} otv_fsm_t;

void otv_fsm_init(otv_fsm_t *f);
void otv_fsm_start(otv_fsm_t *f);

/* Un pas : dt en s, paw = pression voies aériennes (cmH2O), flow = débit
 * patient (L/min, > 0 vers le patient). Retourne la consigne de pression
 * absolue (cmH2O) à passer au régulateur. */
float otv_fsm_step(otv_fsm_t *f, const otv_settings_t *s,
                   float dt, float paw, float flow);

#endif
