/* SPDX-License-Identifier: Apache-2.0 */
#include "otv_pid.h"

void otv_pid_init(otv_pid_t *p, float kp, float ki, float kd,
                  float out_min, float out_max)
{
    p->kp = kp; p->ki = ki; p->kd = kd;
    p->out_min = out_min; p->out_max = out_max;
    otv_pid_reset(p);
}

void otv_pid_reset(otv_pid_t *p)
{
    p->integ = 0.0f;
    p->prev_meas = 0.0f;
    p->primed = false;
}

float otv_pid_step(otv_pid_t *p, float setpoint, float measure, float dt)
{
    const float err = setpoint - measure;

    float deriv = 0.0f;
    if (p->primed && dt > 0.0f)
        deriv = -(measure - p->prev_meas) / dt;
    p->prev_meas = measure;
    p->primed = true;

    float out = p->kp * err + p->integ + p->kd * deriv;

    /* Intégration conditionnelle : on n'accumule pas quand la commande sature
     * dans le sens de l'erreur (anti-windup). */
    const bool sat_hi = out >= p->out_max && err > 0.0f;
    const bool sat_lo = out <= p->out_min && err < 0.0f;
    if (!sat_hi && !sat_lo)
        p->integ += p->ki * err * dt;

    out = p->kp * err + p->integ + p->kd * deriv;
    if (out > p->out_max) out = p->out_max;
    if (out < p->out_min) out = p->out_min;
    return out;
}
