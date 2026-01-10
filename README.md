# SY40 Projet-Ascenseurs

Projet réalisé dans le cadre du module **SY40 – Architecture des systèmes**.  
L’objectif est de simuler le fonctionnement d’un système de gestion d’ascenseurs
en utilisant la programmation concurrente en langage C.

---

## Objectifs du projet

- Simuler plusieurs ascenseurs dans un immeuble
- Gérer des demandes d’usagers (automatiques ou manuelles)
- Implémenter un algorithme de planification
- Simuler des pannes et réparations d’ascenseurs
- Collecter et analyser des statistiques
- Mettre en pratique :
  - processus (`fork`)
  - threads (`pthread`)
  - IPC System V
  - mutex et variables de condition

---

## Architecture du projet

.
├── include/
│ ├── ascenseur.h
│ ├── common.h
│ ├── ipc.h
│ ├── modelisation.h
│ ├── pannes.h
│ ├── planification.h
│ ├── statistiques.h
│ ├── usagers.h
│ └── interface.h
│
├── src/
│ ├── ascenseur.c
│ ├── ipc.c
│ ├── modelisation.c
│ ├── pannes.c
│ ├── planification.c
│ ├── statistiques.c
│ ├── usagers.c
│ ├── interface.c
│ └── main.c
│
├── Makefile
├── README.md
└── stats.csv
---

## Description des modules

### main
Contrôleur central de la simulation :
- lance les processus ascenseurs
- lance les threads (usagers, pannes, interface)
- traite les événements
- gère l’arrêt propre de la simulation
- génère les statistiques finales

---

### ascenseur
- Chaque ascenseur est un processus indépendant
- Reçoit des missions via IPC
- Simule les déplacements
- Gère les pannes et réparations
- Envoie les événements au contrôleur

---

### ipc
- Encapsulation des files de messages System V
- Communication contrôleur ↔ ascenseurs
- Gestion des missions et des événements

---

### usagers
- Génération des demandes d’ascenseurs
- Actif uniquement en mode automatique
- Génération pseudo-aléatoire
- Nombre de demandes strictement limité
- File de demandes thread-safe

---

### planification
- Choix de l’ascenseur le plus adapté
- Basé sur la distance, l’état et la direction
- Algorithme déterministe

---

### pannes
- Thread indépendant
- Déclenchement aléatoire de pannes
- Gestion de la réparation
- Envoi d’événements au contrôleur

---

### interface
- Interface en ligne de commande
- Fonctionne dans un thread séparé
- Commandes disponibles :
  - help
  - status
  - call <from> <to> [prio] (mode manuel)
  - quit
- Non bloquante
- Permet un arrêt immédiat

---

### modelisation
- Définition des constantes du système
- Validation des étages
- Fonctions utilitaires liées au modèle de l’immeuble

---

### statistiques
- Collecte des données de la simulation
- Temps de réponse
- Nombre de demandes, refus et pannes
- Génération du fichier stats.csv
- Affichage d’un résumé final

---

## Modes de simulation

### Mode automatique
- Génération automatique des demandes
- Scénario aléatoire à chaque exécution
- Nombre de missions limité
- Arrêt automatique à la fin

### Mode manuel
- Aucune demande automatique
- Les demandes sont saisies par l’utilisateur
- Arrêt avec la commande quit

---

## Aléatoire et reproductibilité

- Les scénarios utilisent un générateur pseudo-aléatoire
- La seed est initialisée avec time(NULL)
- Chaque exécution produit un scénario différent
- La seed est affichée au démarrage

---

## Résultats

À la fin de la simulation :
- le fichier stats.csv est généré
- un résumé est affiché dans le terminal
- tous les processus et threads sont arrêtés proprement

---

## Conclusion

Ce projet illustre l’utilisation de la programmation concurrente,
de la communication inter-processus et de la synchronisation
à travers la simulation d’un système d’ascenseurs.
