# Manette Switch Pro en Bluetooth sur PS4 (OmniPad, expérimental)

OmniPad peut appairer une **manette Switch Pro** (ou une manette en mode Switch, comme une 8BitDo)
directement en Bluetooth sur la PS4. Ce mode est **expérimental** et **désactivé par défaut**.

## Ce que fait ce mode, en clair

- Sans réglage de ta part, il ne s'active pas : OmniPad gère la DualSense en Bluetooth comme d'habitude.
- Il ne s'active **que si tu crées un fichier** `switchpro_bt.enable` (voir plus bas).
- OmniPad modifie à chaque démarrage, en mémoire, la liste des appareils Bluetooth acceptés par la PS4
  (normalement pour la DualSense). En mode Switch Pro, il y écrit l'identité de la Switch Pro à la place.
  Seules deux identités connues sont acceptées.
- La manette n'envoie que ses boutons et sticks. **Rien n'est envoyé à la manette** (pas de vibration).

## Conditions

1. PS4 jailbreakée avec **GoldHEN 2.4b18.10 ou plus récent**, AutoRun activé
   (Paramètres > GoldHEN > PayLoader > Enable AutoRun).
2. Firmware **12.50, 13.02, 13.04, 13.50 ou 13.52** (Paramètres > Système > Informations système).
   Sur un autre firmware, le mode refuse de s'activer et affiche
   « Switch Pro BT not supported on this firmware ».
3. **Aucun autre programme de manettes** lancé en même temps (retire-le de `/data/GoldHEN/payloads.ini`).
4. Par prudence, sauvegarde tes sauvegardes de jeux (clé USB ou PS Plus).

## Pire cas et sécurité

- **Ta manette** : la PS4 lit seulement ses boutons et ne lui envoie rien. Elle ne peut pas être abîmée.
  Pour la réutiliser sur ta Switch, il suffit de la rebrancher une fois en USB sur la Switch (ou le dock).
- **Ta PS4** : la modification est faite uniquement en mémoire vive. Le jailbreak ne survit pas à un
  redémarrage, donc un redémarrage remet toujours un système 100 % d'origine. Le pire cas réaliste est
  un plantage avec redémarrage, éventuellement suivi d'une vérification du disque. Ni le système ni tes
  sauvegardes ne sont modifiés.

## 1. Installer OmniPad

1. Télécharge `omnipad_companion.pkg` depuis la page Releases du dépôt GitHub.
2. Copie-le à la racine d'une clé USB (exFAT ou FAT32) et installe-le avec le **Package Installer** de GoldHEN.
3. Ouvre une fois **OmniPad Companion** : elle installe le programme et le lance automatiquement au démarrage.

## 2. Activer le mode Switch Pro

1. Ouvre **OmniPad Companion**, onglet **Bluetooth**.
2. Sur la carte **Mode Switch Pro Bluetooth**, mets la valeur sur **Oui** (gauche / droite).
   L'appli vérifie ton firmware et refuse si le mode n'est pas compatible.
3. Redémarre la PS4, relance ton exploit et GoldHEN. Dans l'onglet Bluetooth, la ligne
   « Mode Bluetooth actif » doit afficher **Switch Pro**.

(Sans l'appli : crée un fichier vide `/data/omnipad/switchpro_bt.enable` par FTP.)

## 3. Appairer la manette

Il te faut une autre manette déjà connectée pour naviguer dans les menus.

1. Sur la PS4 : Paramètres > Périphériques > Périphériques Bluetooth.
2. Sur la Switch Pro : maintiens le petit bouton rond sur le dessus (à côté du port USB-C) jusqu'à ce que
   les LED défilent.
3. Choisis « Pro Controller » dans la liste.
4. Tu dois voir « OmniPad: Switch Pro BT (Player 1) », puis l'écran « Qui utilise cette manette ? ».

Les fois suivantes, appuie sur un bouton de la Switch Pro (par exemple Home) : elle se reconnecte seule.

Si l'appairage échoue, ouvre l'onglet **Journal** de l'appli et appuie sur **Carré** pour copier le
journal sur une clé USB (exFAT ou FAT32). Joins ce fichier à un ticket sur GitHub : il contient les
premiers messages bruts de la manette.

## Correspondance des boutons

| Switch Pro | PS4 |
|---|---|
| B / A / Y / X | Croix / Rond / Carré / Triangle (par position, comme sur la manette) |
| L / R | L1 / R1 |
| ZL / ZR | L2 / R2 (tout ou rien : la Switch Pro n'a pas de gâchettes analogiques) |
| − / + | Share / Options |
| Home | PS |
| Capture | Clic du pavé tactile |
| Clic des sticks | L3 / R3 |

Tu peux changer cette correspondance dans l'onglet **Remap Bluetooth** de l'appli (X pour enregistrer,
la modification s'applique tout de suite).

**Pas encore** : vibration, gyroscope, LED des joueurs, niveau de batterie.

**Limites connues :**
- C'est **soit DualSense, soit Switch Pro** en Bluetooth, pas les deux. Tant que le fichier
  `switchpro_bt.enable` existe, la DualSense en Bluetooth est désactivée. Les manettes USB ne sont pas concernées.
- Si tu ne touches à rien pendant 30 secondes, la manette est mise au repos (sticks au centre).
  Appuie sur un bouton et elle revient aussitôt.

## Revenir en arrière

- **Désactiver le mode Switch Pro** : onglet Bluetooth de l'appli, mets le mode sur **Non**, puis redémarre.
- **Désinstaller OmniPad** : supprime la ligne `/data/payloads/omnipad.bin` de `/data/GoldHEN/payloads.ini`,
  supprime l'appli OmniPad Companion, puis redémarre.
- **Si la console plante quand la manette se connecte** : laisse la Switch Pro éteinte (n'appuie sur aucun
  bouton), redémarre, supprime le fichier `switchpro_bt.enable`, puis supprime la manette dans
  Paramètres > Périphériques > Périphériques Bluetooth.
