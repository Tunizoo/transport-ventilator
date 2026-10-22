# OTV — Open Transport Ventilator (nom de travail)

Ventilateur mécanique de transport et de domicile, professionnel et open source.
Architecture à turbine asservie, revendications visées : transport (ISO 80601-2-84)
et patients ventilo-dépendants à domicile (ISO 80601-2-72), marquage CE classe IIb
et 510(k) FDA à terme.

> ⚠️ **Ce dépôt n'est pas un dispositif médical.** Il contient un développement
> d'ingénierie en cours, non certifié, non validé cliniquement. Rien ici ne doit
> être utilisé pour ventiler un patient. Seuls les appareils fabriqués sous le
> système qualité du fabricant légal et portant la marque certifiée pourront
> l'être un jour.

## Documents fondateurs

| Document | Contenu |
| --- | --- |
| Étude de l'existant | Analyse des projets open source (MakAir, PVP1…) et de l'état de l'art commercial |
| Cahier des charges (rev. A) | Usage prévu, modes, plages de performance, alarmes, normes |
| Analyse de risques préliminaire | 28 situations dangereuses cotées, mesures de maîtrise |

(Documents maintenus hors dépôt pour l'instant ; leur export Markdown rejoindra `docs/` à la rev. B.)

## Structure du dépôt

```
docs/            Exigences tracées (requirements.yaml), architecture
firmware/        Cœur temps réel portable (C, sans dépendance) : machine d'états, PID
control-sim/     Simulation Python de la boucle de contrôle sur gabarits de poumon ISO
electronics/     Électronique (KiCad) — à venir
mechanical/      Mécanique (CAO, formats natifs + STEP) — à venir
gui/             IHM (processeur séparé) — à venir
verification/    Bancs et protocoles de vérification — à venir
```

## État d'avancement

- [x] Étude de l'existant
- [x] Cahier des charges rev. A (transport + domicile + VEB)
- [x] Analyse de risques préliminaire (PHA)
- [x] Exigences machine-lisibles avec traçabilité (`docs/exigences/requirements.yaml`)
- [x] Cœur firmware v0 : machine d'états respiratoire + PID (portable, testé en CI)
- [x] Simulation de la boucle pression : mode PC sur gabarits adulte normal / restrictif
- [ ] Choix turbine + capteurs, banc pneumatique réel
- [ ] Électronique (carte contrôle + carte IHM)
- [ ] CAO mécanique
- [ ] SMQ ISO 13485, dossier technique

## Licences

- Logiciel : Apache-2.0 (`LICENSE`)
- Matériel : CERN-OHL — variante (P ou S) **en cours d'arbitrage**, voir `LICENSES.md`
- Documentation : CC-BY-4.0
- La marque et le logo du produit final resteront déposés : un fork est libre mais doit se renommer.

## Contribuer

Voir `CONTRIBUTING.md`. Toute contribution au code destiné à la branche produit
passe par revue d'un mainteneur qualifié ; le dépôt public alimente le dossier
technique mais ne le remplace pas.
