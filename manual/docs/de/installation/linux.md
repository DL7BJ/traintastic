# Linux Installation

Traintastic liefert Debian Pakete für **Ubuntu (amd64/arm64)** und für den **Raspberry Pi (armel/arm64)**.
Es stehen drei Installationspakete zur Verfügung:

- **traintastic-data** – benötigte Daten (muss immer installiert werden)
- **traintastic-server** – der Server muss auf einem Computer installiert werden
- **traintastic-client** – Grafische Benutzeroberfläche, kann auf mehreren Computern installiert werden

## Installation

1. Download der `.deb` Pakete von Traintastic.org/download](https://traintastic.org/download).
2. Installiere die nötigen Pakete mit `apt` (preferred) or `dpkg`. Beispiel:

```bash
   sudo apt update
   sudo apt install ./traintastic-data_<version>_all.deb \
                    ./traintastic-server_<version>_<arch>.deb \
                    ./traintastic-client_<version>_<arch>.deb
```

Ersetze `<version>` mit der Release Nummer (e.g. `0.4.0`) und `<arch>` mit Deiner Plattform (`amd64`, `arm64`, `armhf`).

- amd64 - Computer mit AMD oder Intel Prozessor
- arm64 - Computer mit einem ARM Prozessor (z.B. Apple Mac)
- armhf - Computer mit einem ARM Controller, wie dem Raspberry


## Wähle das Installatios Paket

- Für den Computer, der Deine Modellbahn steuert → installiere `traintastic-server` + `traintastic-data`.
- Für den oder die Computer mit der grafischen Oberfläche für die Bedienung → installiere `traintastic-client` + `traintastic-data`.
- Für die meisten Desktop/Notebook Installationen → installiere alle drei Pakete (`traintastic-server` + `traintastic-client` + `traintastic-data`).

## Starte den Server (systemd)

Wenn die Installation mit einem Debian Paket erfolgte, wird der Traintastic Server als systemd Dienst eingerichtet, ist aber per Default nicht aktiviert.

Starte den Server:
```bash
sudo systemctl start traintastic-server.service
```

Stoppe den Server:
```bash
sudo systemctl stop traintastic-server.service
```

Du benötigst entsprechende Berechtigungen (normalerweise root), um systemd Dienste einzurichten.

### Starte den Server automatisch mit dem Neustart des Rechners

Aktiviere den Server zum automatischen Start beim Starten des Rechners:
```bash
sudo systemctl enable traintastic-server.service
```

Deaktivere den automatischen Start:
```bash
sudo systemctl disable traintastic-server.service
```

## Starte den Client

Wenn Du das Client Paket installiert hast kannst Du den Client über das Menüsystem der jeweiligen Benutzeroberfläche starten (suche nach Traintastic).

---

Nach der Installation kannst Du hier weitermachen: [Die Schnellstart-Anleitungen](../quickstart/index.md).
