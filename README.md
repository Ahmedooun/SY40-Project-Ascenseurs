# SY40-Project-Ascenseurs
# Simulation de 2 ascenseurs — Programmation système (C)

Simulation d’un système de gestion d’ascenseurs dans un immeuble (RDC + 10 étages), avec :
- **Processus** : 2 ascenseurs (créés via `fork()`)
- **Threads** : génération des demandes usagers + simulation de pannes
- **IPC System V** : communication via files de messages (`msgget/msgsnd/msgrcv`)
- **Planification** : choix de l’ascenseur le plus adapté
- **Pannes / réparations** : ascenseur hors service, refus + réinsertion des demandes (aucune perte)
- **Statistiques** : export `stats.csv` (durée totale par demande + résumé)

> ⚠️ Le projet nécessite un environnement **Linux** (ou **WSL** sous Windows).  
> SysV IPC (`<sys/ipc.h>`, `<sys/msg.h>`) n’est pas supporté en Windows natif.

---

## Structure du projet

