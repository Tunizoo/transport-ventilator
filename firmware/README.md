# Firmware

## `core/` — cœur temps réel portable

C99 strict, aucune allocation dynamique, aucune dépendance (pas même la libc
hors `<math.h>` dans les tests) : le même code tourne sur hôte (tests CI) et
tournera sur la cible (STM32 pressenti).

| Fichier | Rôle | Exigences |
| --- | --- | --- |
| `otv_pid.c` | Régulateur PID avec anti-windup (intégration conditionnelle), dérivée sur mesure | REQ-PERF-009 |
| `otv_breath_fsm.c` | Machine d'états respiratoire mode PC : cadence machine, trigger débit + réfractaire, ventilation d'apnée, coupure pression haute | REQ-PERF-002/-008, REQ-MODE-002, REQ-PERF-004 |

## `tests/` — tests hôte

```bash
gcc -std=c99 -Wall -Wextra -Werror -o test_core \
    firmware/tests/test_core.c firmware/core/otv_pid.c firmware/core/otv_breath_fsm.c -lm
./test_core
```

Exécutés en CI (`.github/workflows/ci.yml`). La validation en boucle fermée sur
modèle de poumon est dans `control-sim/`.

## Hors périmètre v0 (à venir)

Pilotes capteurs/turbine, gestionnaire d'alarmes complet (REQ-ALRM-002),
watchdog (REQ-SAFE-002), autotest (REQ-SAFE-003), mémoire de réglages
(REQ-SAFE-004), protocole télémétrie vers IHM, modes VC/SIMV/VNI/VEB.
