#include "decoderprogrammer.hpp"
#include "decoderprogramminglistmodel.hpp"
#include "decodercvdialog.hpp"
#include "../../network/connection.hpp"
#include "../../network/error.hpp"
#include "../../network/tablemodelptr.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QStatusBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <filesystem>

// Verweis auf deine globale Pfad-Funktion
extern std::filesystem::path getDecoderPath();

DecoderProgrammer::DecoderProgrammer(std::shared_ptr<Connection> connection, QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
  , m_connection{std::move(connection)}
  , m_requestId{Connection::invalidRequestId}
  , m_interface{new QComboBox(this)}
  , m_manufacturerList{new QListWidget(this)}
  , m_decoderList{new QListWidget(this)}
  , m_toggleFilterBtn{new QPushButton(this)}
  , m_openProgrammerBtn{new QPushButton(tr("CV Lesen/Schreiben Dialog öffnen"), this)}
  , m_statusBar{new QStatusBar(this)}
{
  setWindowTitle(tr("Decoder Auswahl"));

  // ExtendedSelection erlaubt normales Anklicken, sowie Shift/Strg für Filterauswahl
  m_manufacturerList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_openProgrammerBtn->setEnabled(false);

  // --- Layout-Aufbau ---
  auto* topForm = new QFormLayout();
  topForm->addRow(tr("Zentrale / Interface:"), m_interface);

  // Linke Spalte bekommt ein vertikales Layout für die Liste und den Button darunter
  auto* leftLayout = new QVBoxLayout();
  leftLayout->addWidget(m_manufacturerList);
  leftLayout->addWidget(m_toggleFilterBtn);

  auto* listsLayout = new QHBoxLayout();
  listsLayout->addLayout(leftLayout, 2);     // 2/5 Breite für Hersteller
  listsLayout->addWidget(m_decoderList, 3);  // 3/5 Breite für Decoder

  auto* buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(m_openProgrammerBtn);
  buttonLayout->addStretch();

  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(topForm);
  mainLayout->addLayout(listsLayout);
  mainLayout->addLayout(buttonLayout);
  mainLayout->addWidget(m_statusBar);
  setLayout(mainLayout);

  // --- Signal-Slot-Verbindungen ---
  connect(m_manufacturerList, &QListWidget::itemSelectionChanged, this, &DecoderProgrammer::onManufacturerChanged);
  connect(m_decoderList, &QListWidget::itemSelectionChanged, this, &DecoderProgrammer::onDecoderSelectionChanged);
  connect(m_openProgrammerBtn, &QPushButton::clicked, this, &DecoderProgrammer::onOpenProgrammerClicked);

  // Die Logik für den Filter-Button (Toggle)
  connect(m_toggleFilterBtn, &QPushButton::clicked, this, [this]() {
      if (m_isFilterActive) {
          // Filter aufheben: Liste leeren, Zustand zurücksetzen
          m_favoriteManufacturers.clear();
          m_isFilterActive = false;
      } else {
          // Filter setzen: Alle aktuell markierten Hersteller-IDs sichern
          m_favoriteManufacturers.clear();
          for (QListWidgetItem* item : m_manufacturerList->selectedItems()) {
              int uniqueKey = item->data(Qt::UserRole).toInt();
              m_favoriteManufacturers.append(m_manufacturers[uniqueKey].id);
          }
          m_isFilterActive = true;
      }

      // Zustand persistent speichern, Ansicht updaten und Button-Text anpassen
      saveUserSettings();
      loadManufacturersJSON();
      updateFilterButtonText();
  });

  // Automatisches Speichern des Interfaces, sobald sich der Text ändert
  connect(m_interface, &QComboBox::currentTextChanged, [this](const QString& text) {
      if (!text.isEmpty()) {
          saveUserSettings();
      }
  });

  // --- Daten laden ---
  loadInterfaces();
  loadUserSettings();
  updateFilterButtonText();

  if (loadManufacturersJSON()) {
      loadFirmwareDetailsJSON();
  }
}

DecoderProgrammer::~DecoderProgrammer()
{
  if(m_requestId != Connection::invalidRequestId)
    m_connection->cancelRequest(m_requestId);
}

void DecoderProgrammer::updateFilterButtonText()
{
    if (m_isFilterActive) {
        m_toggleFilterBtn->setText(tr("Filter aufheben (Alle anzeigen)"));
    } else {
        m_toggleFilterBtn->setText(tr("Markierte als Filter speichern"));
    }
}

void DecoderProgrammer::loadUserSettings()
{
    m_favoriteManufacturers.clear();
    m_defaultInterface.clear();
    m_isFilterActive = false;

    std::filesystem::path settingsPath = getDecoderPath() / "userSettings.json";
    QFile file(QString::fromStdString(settingsPath.string()));
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    m_defaultInterface = root["defaultInterface"].toString();

    QJsonArray favArray = root["favoriteManufacturers"].toArray();
    for (const QJsonValue& val : favArray) {
        m_favoriteManufacturers.append(val.toInt());
    }

    // Wenn geladene Favoriten vorhanden sind, wird der Filter direkt scharf geschaltet
    if (!m_favoriteManufacturers.isEmpty()) {
        m_isFilterActive = true;
    }
}

void DecoderProgrammer::saveUserSettings()
{
    std::filesystem::path settingsPath = getDecoderPath() / "userSettings.json";
    QFile file(QString::fromStdString(settingsPath.string()));
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonObject root;
    root["defaultInterface"] = m_interface->currentText();

    QJsonArray favArray;
    for (int id : m_favoriteManufacturers) {
        favArray.append(id);
    }
    root["favoriteManufacturers"] = favArray;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
}

bool DecoderProgrammer::loadManufacturersJSON()
{
    std::string fullPathStr = (getDecoderPath() / "Manufacturers.json").string();
    QFile file(QString::fromStdString(fullPathStr));
    if(!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject rootObj = doc.object();
    QJsonArray manufacturerArray = rootObj["manufacturers"].toArray();

    // Signale blockieren, damit das Löschen nicht flackert oder Nebeneffekte hat
    m_manufacturerList->blockSignals(true);
    m_manufacturers.clear();
    m_manufacturerList->clear();

    for (const QJsonValue& val : manufacturerArray) {
        QJsonObject manObj = val.toObject();
        ManufacturerInfo info;
        info.id         = manObj["id"].toInt();
        info.extendedId = manObj["extendedId"].toInt();
        info.Name       = manObj["name"].toString();
        info.shortName  = manObj["shortName"].toString();
        info.uri        = manObj["url"].toString();

        // Wenn der Filter aktiv ist, überspringen wir alle Nicht-Favoriten
        if (m_isFilterActive && !m_favoriteManufacturers.contains(info.id)) {
            continue;
        }

        int uniqueKey = (info.extendedId << 8) | info.id;
        m_manufacturers.insert(uniqueKey, info);

        auto* item = new QListWidgetItem(info.Name, m_manufacturerList);
        item->setData(Qt::UserRole, uniqueKey);
    }

    m_manufacturerList->sortItems();
    m_manufacturerList->blockSignals(false);

    // Automatisch die erste Zeile anwählen, damit rechts Daten erscheinen
    if (m_manufacturerList->count() > 0) {
        m_manufacturerList->setCurrentRow(0);
    } else {
        onManufacturerChanged();
    }
    return true;
}

void DecoderProgrammer::loadFirmwareDetailsJSON()
{
    std::string fullPathStr = (getDecoderPath() / "firmware/firmwareDetails.json").string();
    QFile file(QString::fromStdString(fullPathStr));
    if(!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_firmwareDetailsArray = doc.array();
}

void DecoderProgrammer::onManufacturerChanged()
{
    m_decoderList->clear();
    m_openProgrammerBtn->setEnabled(false);

    QList<QListWidgetItem*> selectedItems = m_manufacturerList->selectedItems();
    if (selectedItems.isEmpty()) return;

    // Unterstützt Einzel- sowie Mehrfachauswahl (z.B. beim Ziehen eines Blocks mit Shift)
    for (QListWidgetItem* item : selectedItems) {
        int uniqueKey = item->data(Qt::UserRole).toInt();
        if (m_manufacturers.contains(uniqueKey)) {
            ManufacturerInfo info = m_manufacturers.value(uniqueKey);
            filterDecodersForManufacturer(info.id, info.extendedId);
        }
    }
}

void DecoderProgrammer::filterDecodersForManufacturer(int manufacturerId, int extendedId)
{
    for (const QJsonValue& fwVal : m_firmwareDetailsArray)
    {
        QJsonObject fwObj = fwVal.toObject();

        if (fwObj["manufacturerId"].toInt() == manufacturerId &&
            fwObj["manufacturerExtendedId"].toInt() == extendedId)
        {
            QString subDir = (extendedId > 0)
                ? QString("%1_%2").arg(manufacturerId).arg(extendedId)
                : QString::number(manufacturerId);

            QString filename = fwObj["filename"].toString();
            std::filesystem::path fullPath = getDecoderPath() / "firmware" / subDir.toStdString() / filename.toStdString();
            QString cvJsonPath = QString::fromStdString(fullPath.string());

            QJsonArray decoderArray = fwObj["decoder"].toArray();
            for (const QJsonValue& decVal : decoderArray)
            {
                QJsonObject decObj = decVal.toObject();
                QString decoderName = decObj["name"].toString();

                if (!decoderName.isEmpty()) {
                    auto* item = new QListWidgetItem(decoderName, m_decoderList);
                    item->setData(Qt::UserRole, cvJsonPath);
                }
            }
        }
    }
    m_decoderList->sortItems();
}

void DecoderProgrammer::onDecoderSelectionChanged()
{
    bool hasSelection = !m_decoderList->selectedItems().isEmpty();
    m_openProgrammerBtn->setEnabled(hasSelection);
}

void DecoderProgrammer::onOpenProgrammerClicked()
{
    QListWidgetItem* selectedDecoder = m_decoderList->currentItem();
    if (!selectedDecoder) return;

    QString cvJsonPath = selectedDecoder->data(Qt::UserRole).toString();
    QString interfaceName = m_interface->currentText();

    // Instanziieren des neuen Dialogs. parent=nullptr macht es zu einem eigenständigen Fenster!
    auto* cvDialog = new DecoderCvDialog(m_connection, interfaceName, cvJsonPath, nullptr);

    // Anzeigen. Da es von QDialog/QWidget erbt, öffnet show() es völlig unblockiert (nicht-modal).
    cvDialog->show();

    m_statusBar->showMessage(tr("Öffne Programmierfenster für: %1").arg(selectedDecoder->text()), 2000);
}

void DecoderProgrammer::loadInterfaces()
{
    m_requestId = m_connection->getObject("world.decoder_programming_controllers",
      [this](const ObjectPtr& object, std::optional<const Error> /*error*/)
      {
        m_requestId = Connection::invalidRequestId;
        if(object) {
          m_requestId = m_connection->getTableModel(object,
            [this](const TableModelPtr& table, std::optional<const Error> /*error*/)
            {
              m_requestId = Connection::invalidRequestId;
              if(table) {
                  m_interface->setModel(new DecoderProgrammingListModel(table, m_interface));

                  if (!m_defaultInterface.isEmpty()) {
                      int idx = m_interface->findText(m_defaultInterface);
                      if (idx != -1) m_interface->setCurrentIndex(idx);
                  }
              }
            });
        }
      });
}
