/* OTV — régulateur PID de la boucle pression (REQ-PERF-009).
 * C portable, pas d'allocation, pas de dépendance : exécutable sur cible et sur hôte.
 * SPDX-License-Identifier: Apache-2.0 */
#ifndef OTV_PID_H
#define OTV_PID_H

#include <stdbool.h>

typedef struct {
    float kp, ki, kd;
    float out_min, out_max;
    float integ;      /* terme intégral accumulé */
    float prev_meas;  /* dérivée sur la mesure (évite le kick de consigne) */
    bool  primed;
} otv_pid_t;

void  otv_pid_init(otv_pid_t *p, float kp, float ki, float kd,
                   float out_min, float out_max);
void  otv_pid_reset(otv_pid_t *p);

/* Un pas de régulation. dt en secondes (> 0). Retourne la commande bornée
 * [out_min, out_max]. Anti-windup par intégration conditionnelle. */
float otv_pid_step(otv_pid_t *p, float setpoint, float measure, float dt);

#endif
