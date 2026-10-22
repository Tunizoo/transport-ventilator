# Simulation de la boucle de contrôle

`simulate.py` ferme la boucle **PID (100 Hz) → turbine (1er ordre, 40 ms) →
poumon monocompartimental (C, R)** en mode pression contrôlée et vérifie les
critères du cahier des charges sur les gabarits adulte de l'ISO 80601-2-12 :

- exactitude du plateau ±(2 cmH2O + 4 %) — REQ-PERF-003
- tenue de la PEEP ±(2 cmH2O + 4 %) — REQ-PERF-005
- montée à 90 % < 200 ms (cible interne de réactivité)

```bash
pip install numpy matplotlib
python control-sim/simulate.py          # écrit results/*.png + métriques
```

Code de retour non nul si un critère échoue : le script sert de banc de
vérification automatisé en CI.

Limites du modèle v0 (assumées) : turbine idéalisée (pas de saturation débit ni
de bruit capteur), pas d'effort patient, valve expiratoire confondue avec la
source de pression. Le banc pneumatique réel calibrera tau et les gains.
