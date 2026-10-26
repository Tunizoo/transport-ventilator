/* Tests hôte du cœur temps réel (gcc/clang, exécutés en CI).
 * SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../core/otv_pid.h"
#include "../core/otv_breath_fsm.h"

#define DT 0.001f /* simulation à 1 kHz, boucle de contrôle incluse */

static otv_settings_t default_settings(void)
{
    otv_settings_t s = {
        .rate_cpm = 15.0f, .ti_s = 1.0f,
        .p_insp_cmh2o = 15.0f, .peep_cmh2o = 5.0f,
        .trig_flow_lpm = 2.0f, .apnea_delay_s = 20.0f,
        .p_max_cmh2o = 60.0f,
    };
    return s;
}

/* Le PID doit amener un plant du premier ordre (tau 40 ms, type turbine) à la
 * consigne sans dépassement excessif ni erreur statique. */
static void test_pid_first_order_plant(void)
{
    otv_pid_t pid;
    otv_pid_init(&pid, 3.0f, 30.0f, 0.0f, 0.0f, 80.0f);

    float plant = 0.0f; /* pression turbine, cmH2O */
    const float tau = 0.040f, target = 20.0f;
    float peak = 0.0f;
    float t90 = -1.0f;

    for (int i = 0; i < 2000; i++) { /* 2 s */
        const float cmd = otv_pid_step(&pid, target, plant, DT);
        plant += (cmd - plant) / tau * DT;
        if (plant > peak) peak = plant;
        if (t90 < 0.0f && plant >= 0.9f * target) t90 = i * DT;
    }
    assert(fabsf(plant - target) < 0.5f);  /* erreur statique nulle de fait */
    assert(peak < target * 1.15f);         /* dépassement < 15 % */
    assert(t90 > 0.0f && t90 < 0.20f);     /* montée 90 % < 200 ms */
    printf("ok  pid: t90=%.0f ms, overshoot=%.1f %%\n",
           t90 * 1000.0f, (peak / target - 1.0f) * 100.0f);
}

/* Cadencement machine : à 15 c/min, Ti 1 s, la 2e inspiration démarre à 4 s. */
static void test_fsm_mandatory_cycling(void)
{
    otv_fsm_t f; otv_fsm_init(&f);
    otv_settings_t s = default_settings();
    s.trig_flow_lpm = 0.0f; /* pas de trigger : cadence machine pure */
    otv_fsm_start(&f);

    int insp_starts = 0;
    /* start() a déjà ouvert la 1re inspiration : on ne compte que les
     * transitions suivantes (cycles machine attendus à t = 4 s et 8 s). */
    otv_phase_t prev = f.phase;
    float t_second_insp = -1.0f;
    for (int i = 0; i < 9000; i++) { /* 9 s */
        otv_fsm_step(&f, &s, DT, 10.0f, 0.0f);
        if (f.phase == OTV_PHASE_INSP && prev != OTV_PHASE_INSP) {
            insp_starts++;
            if (insp_starts == 1) t_second_insp = i * DT;
        }
        prev = f.phase;
    }
    /* start() ouvre la 1re inspiration ; on doit en voir 2 autres (t=4 s, 8 s). */
    assert(insp_starts == 2);
    assert(fabsf(t_second_insp - 4.0f) < 0.01f);
    printf("ok  fsm: cadence machine (2e cycle a %.3f s)\n", t_second_insp);
}

/* Trigger : un effort patient (débit > seuil) pendant l'expiration démarre une
 * inspiration spontanée avant le cycle machine. */
static void test_fsm_patient_trigger(void)
{
    otv_fsm_t f; otv_fsm_init(&f);
    const otv_settings_t s = default_settings();
    otv_fsm_start(&f);

    bool triggered = false;
    for (int i = 0; i < 3000; i++) { /* 3 s : Ti=1 s puis expiration */
        const float t = i * DT;
        const float flow = (t > 2.0f && t < 2.1f) ? 5.0f : 0.0f; /* effort à 2 s */
        otv_fsm_step(&f, &s, DT, 10.0f, flow);
        if (f.phase == OTV_PHASE_INSP && f.breath_spontaneous) {
            triggered = true;
            assert(t > 2.0f && t < 2.2f);
            break;
        }
    }
    assert(triggered);
    printf("ok  fsm: trigger patient\n");
}

/* Réfractaire : le même effort placé 0,2 s après le début de l'expiration
 * (t = 1,2 s) doit être ignoré. */
static void test_fsm_trigger_refractory(void)
{
    otv_fsm_t f; otv_fsm_init(&f);
    const otv_settings_t s = default_settings();
    otv_fsm_start(&f);

    for (int i = 0; i < 1500; i++) { /* jusqu'a 1,5 s */
        const float t = i * DT;
        const float flow = (t > 1.15f && t < 1.25f) ? 5.0f : 0.0f;
        otv_fsm_step(&f, &s, DT, 10.0f, flow);
        assert(!f.breath_spontaneous);
    }
    printf("ok  fsm: periode refractaire\n");
}

/* Pression haute : paw >= p_max en inspiration coupe vers l'expiration et lève
 * l'alarme (VENT-01 / REQ-PERF-004). */
static void test_fsm_high_pressure_cutoff(void)
{
    otv_fsm_t f; otv_fsm_init(&f);
    const otv_settings_t s = default_settings();
    otv_fsm_start(&f);

    otv_fsm_step(&f, &s, DT, 10.0f, 0.0f);
    assert(f.phase == OTV_PHASE_INSP);
    otv_fsm_step(&f, &s, DT, 65.0f, 0.0f); /* surpression */
    assert(f.phase == OTV_PHASE_EXP);
    assert(f.alarms & OTV_ALARM_HIGH_PRESSURE);
    printf("ok  fsm: coupure pression haute\n");
}

/* Apnée : fréquence machine très basse + aucun effort -> ventilation de secours
 * au bout du délai d'apnée, avec alarme (REQ-MODE-002). */
static void test_fsm_apnea_backup(void)
{
    otv_fsm_t f; otv_fsm_init(&f);
    otv_settings_t s = default_settings();
    s.rate_cpm = 2.0f;        /* période 30 s */
    s.apnea_delay_s = 10.0f;
    otv_fsm_start(&f);

    float t_backup = -1.0f;
    for (int i = 0; i < 15000; i++) { /* 15 s */
        otv_fsm_step(&f, &s, DT, 10.0f, 0.0f);
        if (f.backup_active) { t_backup = i * DT; break; }
    }
    assert(t_backup > 0.0f);
    assert(fabsf(t_backup - 10.0f) < 0.01f);
    assert(f.alarms & OTV_ALARM_APNEA);
    printf("ok  fsm: ventilation d'apnee a %.2f s\n", t_backup);
}

int main(void)
{
    test_pid_first_order_plant();
    test_fsm_mandatory_cycling();
    test_fsm_patient_trigger();
    test_fsm_trigger_refractory();
    test_fsm_high_pressure_cutoff();
    test_fsm_apnea_backup();
    printf("tous les tests passent\n");
    return 0;
}
