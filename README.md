# SY40 – Projet Ascenseurs

Simulation concurrente d’un système de gestion d’ascenseurs, réalisée en C dans le cadre du module SY40 – Architecture des systèmes.

Le projet met en œuvre des processus, threads, IPC System V, et des mécanismes de synchronisation pour simuler un immeuble avec plusieurs ascenseurs, des usagers, des pannes et des statistiques.

# Objectifs du projet

**Simuler le fonctionnement de plusieurs ascenseurs**

**Gérer des demandes d’usagers (automatiques ou manuelles)**

**Implémenter un algorithme de planification**

**Gérer des pannes et réparations d’ascenseurs**

**Collecter des statistiques de performance**

**Illustrer l’utilisation de :**

-processus (fork)

-threads (pthread)

-files de messages System V

-mutex / conditions

-programmation concurrente

# Architecture du projet
.
├── include/
│   ├── ascenseur.h
│   ├── common.h
│   ├── ipc.h
│   ├── modelisation.h
│   ├── pannes.h
│   ├── planification.h
│   ├── statistiques.h
│   ├── usagers.h
│   └── interface.h
│
├── src/
│   ├── ascenseur.c
│   ├── ipc.c
│   ├── modelisation.c
│   ├── pannes.c
│   ├── planification.c
│   ├── statistiques.c
│   ├── usagers.c
│   ├── interface.c
│   └── main.c
│
├── Makefile
├── README.md
└── stats.csv

# Description des modules
# main

Rôle de contrôleur central

Lance les ascenseurs (processus)

Lance les threads (usagers, pannes, interface)

Gère la boucle principale

Traite les événements (EVT_DROPOFF, EVT_PANNE, etc.)

Déclenche l’arrêt propre et génère les statistiques

# ascenseur

Chaque ascenseur est un processus indépendant

Reçoit des missions via IPC

Simule le déplacement

Gère les pannes / réparations

N’écrit jamais directement sur stdout
→ les logs sont envoyés au contrôleur (EVT_LOG)

# ipc

Encapsulation des files de messages System V

Envoi / réception de :

missions

événements

Séparation claire :

contrôleur → ascenseurs

ascenseurs → contrôleur

# usagers

Génère des demandes d’ascenseurs

Fonctionne uniquement en mode automatique

Génération pseudo-aléatoire

Nombre de demandes strictement limité

File de demandes thread-safe (mutex + conditions)

# planification

Choisit l’ascenseur le plus adapté à une demande

Basé sur :

état des ascenseurs

direction

distance

Logique déterministe et centralisée

# pannes

Thread indépendant

Déclenche aléatoirement des pannes

Gère la durée de panne et la réparation

Envoie des événements au contrôleur

Peut être stoppé proprement

# interface

Interface en ligne de commande

Fonctionne dans un thread séparé

Commandes disponibles :

help

status

call <from> <to> [prio] (mode manuel uniquement)

quit

Non bloquante (poll)

Permet un arrêt immédiat même si la file est vide

# modelisation

Définit les constantes du système (immeuble, étages)

Fonctions de validation (étages valides, directions)

Support conceptuel de la simulation

# statistiques

Collecte des données :

temps de réponse

nombre de demandes

refus

pannes

Génère :

stats.csv

résumé console en fin de simulation

# Modes de simulation
 **Mode automatique**

Génération automatique des demandes

Scénario aléatoire à chaque exécution

Nombre de missions strictement limité

Arrêt automatique après la dernière mission terminée

 **Mode manuel**

Aucune génération automatique

Toutes les demandes sont saisies via l’interface

Arrêt via la commande quit

# Aléatoire et reproductibilité

Les scénarios utilisent un générateur pseudo-aléatoire

La seed est initialisée avec time(NULL)

Chaque exécution produit un scénario différent

La seed est affichée au démarrage pour permettre une analyse

# Résultats

À la fin de la simulation :

**Un fichier stats.csv est généré**

**Un résumé statistique est affiché dans le terminal**

**Tous les threads et processus sont arrêtés proprement**


# Conclusion

Ce projet illustre concrètement :

**la communication inter-processus**

**la synchronisation**

**la gestion d’événements**

**la simulation de systèmes complexes**
