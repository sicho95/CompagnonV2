# CompagnonV2

PWA locale/offline a la racine du repo pour utiliser et synchroniser CompagnonV2.

Le firmware Arduino est dans `firmware/CompagnonV2/`.

## Usage local

```bash
python3 -m http.server 8765
```

Puis ouvrir `http://localhost:8765/`.

Web Bluetooth demande un navigateur compatible, typiquement Chrome ou Edge. Le service worker rend l'interface disponible hors ligne apres le premier chargement.

## Service BLE

Service principal: `12345678-1234-1234-1234-1234567890ab`

| Caracteristique | UUID | Sens |
| --- | --- | --- |
| `WIFI_SCAN` | `00002002-0000-1000-8000-00805f9b34fb` | PWA ecrit `{cmd:"scan"}`, ESP32 notifie une liste JSON |
| `WIFI_PROVISION` | `00002003-0000-1000-8000-00805f9b34fb` | PWA ecrit `{ssid, pass}` |
| `AGENT_SYNC` | `00002004-0000-1000-8000-00805f9b34fb` | Commandes JSON NVS/config |
| `TEXT_INPUT` | `00002005-0000-1000-8000-00805f9b34fb` | Texte libre vers l'orchestrateur |
| `LLM_RELAY` | `00002006-0000-1000-8000-00805f9b34fb` | Relais LLM/internet applicatif |
| `DEVICE_STATUS` | `00002007-0000-1000-8000-00805f9b34fb` | Statut JSON notifie par l'ESP32 |
| `GPS` | `00002008-0000-1000-8000-00805f9b34fb` | PWA ecrit `{lat, lon, alt, speed, accuracy, ts}` |

## Commandes `AGENT_SYNC`

```json
{ "cmd": "set_api_key", "key": "groq_key", "value": "..." }
{ "cmd": "set_config", "ns": "system", "key": "timezone", "value": "CET-1CEST,M3.5.0,M10.5.0/3" }
{ "cmd": "list_api_keys" }
{ "cmd": "status" }
```

Les cles API autorisees sont celles de `firmware/CompagnonV2/src/config/nvs_config.h`.
