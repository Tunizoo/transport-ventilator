# Contribuer

Merci de votre intérêt ! Ce projet développe un dispositif médical : les règles
de contribution sont plus strictes qu'un projet open source classique.

## Principes

1. **Deux branches de vie** : `main` (communautaire, recherche) et, à terme, des
   branches `device/*` gelées sous configuration ISO 13485. Rien n'entre dans une
   branche `device/*` sans revue d'un mainteneur qualifié et sans tests.
2. **Traçabilité** : tout changement de comportement du firmware référence une
   exigence (`docs/exigences/requirements.yaml`, champ `id`) dans le message de
   commit, ou en propose une nouvelle.
3. **Code critique** (firmware/core) : C portable sans allocation dynamique,
   sans dépendance ; chaque fonction testée dans `firmware/tests/` ; CI verte
   obligatoire.
4. **Sécurité** : une vulnérabilité se signale en privé au mainteneur, pas en
   issue publique.

## Mécanique

- Fork + pull request, commits signés.
- DCO (`Signed-off-by`) exigé sur chaque commit ; un CLA pourra le remplacer
  (arbitrage juridique en cours).
- Style : `clang-format` (fichier à venir), Python ruff/PEP8.

## Ce dont le projet a besoin en priorité

- Revue clinique des exigences et des cotations de risque
- Expérience turbine/pneumatique (choix composant, banc d'essai)
- Électronique 60601 (alimentation, CEM, BMS)
- Affaires réglementaires (MDR, 510(k))
