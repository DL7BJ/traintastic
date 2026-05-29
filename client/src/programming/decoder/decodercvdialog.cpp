#include "decodercvdialog.hpp"
#include "../../network/connection.hpp"
#include "../../network/error.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QStatusBar>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTabBar>

#include <QScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>

#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QFormLayout>


DecoderCvDialog::DecoderCvDialog(std::shared_ptr<Connection> connection,
                                 const QString& interfaceName,
                                 const QString& cvJsonPath,
                                 QWidget* parent)
    : QDialog(parent)
    , m_connection{std::move(connection)}
    , m_requestId{Connection::invalidRequestId}
    , m_interfaceName{interfaceName}
    , m_cvJsonPath{cvJsonPath}
{
    // Das magische Qt-Attribut: Fenster löscht sich beim Schließen selbst aus dem RAM!
    setAttribute(Qt::WA_DeleteOnClose);

    // Basis-Fenstereinstellungen
    setWindowTitle(tr("CV Editor - %1").arg(m_interfaceName));
    resize(600, 400);

    setupUi();
    loadCvDefinitionJson();
}

DecoderCvDialog::~DecoderCvDialog()
{
    if (m_requestId != Connection::invalidRequestId) {
        m_connection->cancelRequest(m_requestId);
    }
    qDebug() << "DecoderCvDialog für" << m_cvJsonPath << "wurde zerstörungsfrei geschlossen.";
}


void DecoderCvDialog::setupUi()
{
    // Großzügige Standardgröße für die neue Raumaufteilung
    resize(850, 550);

    // ==========================================
    // LINKS: Die Seitenliste im Tab-Design
    // ==========================================
    m_sideList = new QListWidget(this);
    m_sideList->setFixedWidth(200);
    m_sideList->setFrameShape(QFrame::NoFrame);

    // Edles Stylesheet: Große Klickflächen, Hover-Effekt und aktiver "Tab-Marker"
    m_sideList->setStyleSheet(
        "QListWidget {"
        "  background-color: transparent;" // Nutzt das Traintastic-Theme im Hintergrund
        "  border-right: 1px solid #444;"  // Dezente Trennlinie nach rechts
        "  padding-top: 10px;"
        "}"
        "QListWidget::item {"
        "  height: 45px;"
        "  padding-left: 15px;"
        "  margin-bottom: 4px;"
        "  border-left: 4px solid transparent;" // Platzhalter für den aktiven Balken
        "}"
        "QListWidget::item:hover {"
        "  background-color: rgba(255, 255, 255, 0.05);"
        "}"
        "QListWidget::item:selected {"
        "  background-color: rgba(255, 255, 255, 0.1);"
        "  color: #fff;"
        "  border-left: 4px solid #3498db;" // Schicker blauer "Tab"-Indikator links
        "}"
    );

    // RECHTS: Der Container für die grafischen CV-Widgets
    m_stackedWidget = new QStackedWidget(this);

    // Splitter verbinden
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_sideList);
    splitter->addWidget(m_stackedWidget);
    splitter->setCollapsible(0, false);

    // BUTTONS UNTEN
    m_readBtn  = new QPushButton(tr("Lesen"), this);
    m_writeBtn = new QPushButton(tr("Schreiben"), this);
    m_stopBtn  = new QPushButton(tr("Stopp"), this);
    m_statusBar = new QStatusBar(this);

    // Button-Layout (horizontal zentriert)
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_readBtn);
    btnLayout->addWidget(m_writeBtn);
    btnLayout->addWidget(m_stopBtn);
    btnLayout->addStretch();

    // HAUPTLAYOUT DES DIALOGS
    auto* mainLayout = new QVBoxLayout(this);

    // Hier passiert die Dynamik-Magie:
    // Wir fügen den Splitter mit Stretch-Faktor 1 hinzu, das Button-Layout mit 0.
    // Dadurch dehnt sich NUR der obere Inhalt aus, wenn du das Fenster ziehst!
    mainLayout->addWidget(splitter, 1);
    mainLayout->addLayout(btnLayout, 0);
    mainLayout->addWidget(m_statusBar, 0);
    setLayout(mainLayout);

    // Signale verbinden
    connect(m_readBtn, &QPushButton::clicked, this, &DecoderCvDialog::onReadClicked);
    connect(m_writeBtn, &QPushButton::clicked, this, &DecoderCvDialog::onWriteClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &DecoderCvDialog::onStopClicked);

    connect(m_sideList, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(m_sideList, &QListWidget::currentRowChanged, this, &DecoderCvDialog::updateButtonsEnabled);

    updateButtonsEnabled();
}

//!
//! \brief DecoderCvDialog::loadCvDefinitionJson
//!
void DecoderCvDialog::loadCvDefinitionJson()
{
    QFile file(m_cvJsonPath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonObject protocolObj = root["firmware"].toObject()["protocol"].toObject();
    QJsonArray cvStructure = protocolObj["cvStructure"].toArray();
    QJsonArray allCvs = protocolObj["cvs"].toArray();

    for (const QJsonValue& structVal : cvStructure) {
        QJsonObject groupObj = structVal.toObject();

        // 1. Gruppenname finden
        QString groupName = groupObj["descriptions"].toArray().first().toObject()["text"].toString();

        // 2. ScrollArea vorbereiten
        auto* scrollArea = new QScrollArea(this);
        auto* contentWidget = new QWidget();
        auto* contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setAlignment(Qt::AlignTop);

        // 3. Gruppe rendern
        for (const QJsonValue& itemVal : groupObj["items"].toArray()) {
            QString targetId = itemVal.toObject()["id"].toString();

            for (const QJsonValue& cvVal : allCvs) {
                QJsonObject cvObj = cvVal.toObject();

                if (cvObj["id"].toString() == targetId) {
                    if(cvObj["number"].toInt() == 29) {
                        QJsonObject filteredCv = filterCv29ForSpeed(cvObj);
                        renderCvElement(filteredCv, contentLayout);
                    }
                    // Prüfung: Ist das eine Kennlinie? (Wir prüfen das Feld "type" des Objekts)
                    if (cvObj["type"].toString() == "dccSpeedCurve") {
                        renderSpeedCurve(allCvs, contentLayout);
                    } else {
                        renderCvElement(cvObj, contentLayout);
                    }
                    break; // Suche in allCvs beenden, da wir das Item gefunden haben
                }
            }
        }
        scrollArea->setWidget(contentWidget);
        scrollArea->setWidgetResizable(true);
        m_sideList->addItem(groupName);
        m_stackedWidget->addWidget(scrollArea);
    }
}
//!
//! \brief DecoderCvDialog::filterCv29ForSpeed
//! \param originalCv
//! \return
//!
QJsonObject DecoderCvDialog::filterCv29ForSpeed(const QJsonObject& originalCv) {
    QJsonObject filtered = originalCv;
    QJsonArray allItems = originalCv["items"].toArray();
    QJsonArray speedRelevantItems;

    // Angenommen, das Bit für die Kennlinie hat eine bestimmte ID oder Beschreibung
    for (const QJsonValue& item : allItems) {
        QJsonObject itemObj = item.toObject();
        // Hier das Kriterium anpassen, z.B. anhand einer ID oder eines Bit-Index
        if (itemObj["id"].toString() == "DEIN_BIT_ID_FUER_KENNLINIE") {
            speedRelevantItems.append(item);
        }
    }
    filtered["items"] = speedRelevantItems;
    return filtered;
}
//!
//! \brief DecoderCvDialog::renderSpeedCurve
//! \param allCvs
//! \param layout
//!
void DecoderCvDialog::renderSpeedCurve(const QJsonArray& allCvs, QVBoxLayout* layout) {
    auto* box = new QGroupBox(tr("Geschwindigkeitskennlinie (CV 67-94)"));
    auto* hLayout = new QHBoxLayout(box);

    for (int cvNum = 67; cvNum <= 94; ++cvNum) {
        int defaultValue = 0;

        // Wert für diesen CV suchen
        for (const QJsonValue& cvVal : allCvs) {
            QJsonObject c = cvVal.toObject();
            if (c["number"].toInt() == cvNum) {
                defaultValue = c["defaultValue"].toInt();
                break;
            }
        }

        // Vertikales Layout für das "Paket" aus Slider, Spinbox und Label
        auto* vCol = new QVBoxLayout();

        auto* slider = new QSlider(Qt::Vertical);
        slider->setRange(0, 255);
        slider->setValue(defaultValue);
        slider->setFixedHeight(100);

        auto* spin = new QSpinBox();
        spin->setRange(0, 255);
        spin->setValue(defaultValue);
        spin->setFixedWidth(40);

        // Label für die CV-Nummer (z.B. "67")
        auto* label = new QLabel(QString::number(cvNum));
        label->setAlignment(Qt::AlignCenter);

        // Synchronisation
        connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);

        vCol->addWidget(slider);
        vCol->addWidget(spin);
        vCol->addWidget(label);

        hLayout->addLayout(vCol);
    }

    // Damit das Fenster bei 28 Slidern nicht gesprengt wird,
    // könnte man hier später noch eine ScrollArea in die GroupBox setzen.
    layout->addWidget(box);
}
//!
//! \brief DecoderCvDialog::renderCvElement
//! \param cv
//! \param layout
//!
void DecoderCvDialog::renderCvElement(const QJsonObject& cv, QVBoxLayout* layout)
{
    QString type = cv["type"].toString();
    QString groupType = cv["groupType"].toString();
    int number = cv["number"].toInt();
    int defVal = cv["defaultValue"].toInt();

    QString label = tr("Unbekannt");
    QJsonArray descArray = cv["descriptions"].toArray();
    if (!descArray.isEmpty()) {
        label = descArray.first().toObject()["text"].toString();
    }

    switch (stringToType(type, groupType)) {

        case CvType::Select: {
            auto* box = new QGroupBox(QString("%1 (CV %2)").arg(label).arg(number));
            auto* vLayout = new QVBoxLayout(box);
            for (const auto& item : cv["items"].toArray()) {
                QJsonObject itemObj = item.toObject();
                if (itemObj["type"] == "bit") {
                    vLayout->addWidget(new QCheckBox(itemObj["descriptions"].toArray().first().toObject()["text"].toString()));
                } else {
                    auto* bg = new QButtonGroup(box);
                    for (const auto& opt : itemObj["options"].toArray()) {
                        auto* rb = new QRadioButton(opt.toObject()["descriptions"].toArray().first().toObject()["text"].toString());
                        bg->addButton(rb);
                        vLayout->addWidget(rb);
                    }
                }
            }
            layout->addWidget(box);
            break;
        }

        case CvType::Byte: {
            QFormLayout* form = getOrCreateFormLayout(layout);

            auto* hBox = new QHBoxLayout(); // Hier wird hBox definiert
            auto* slider = new QSlider(Qt::Horizontal);
            auto* spin = new QSpinBox();

            slider->setRange(0, 255);
            spin->setRange(0, 255);
            slider->setValue(defVal);
            spin->setValue(defVal);

            connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);

            hBox->addWidget(slider);
            hBox->addWidget(spin);

            // hBox ist jetzt innerhalb dieses {}-Blocks für den Compiler sichtbar!
            form->addRow(QString("%1 (CV %2):").arg(label).arg(number), hBox);
            break;
        }
        default:
            break;
    }
}


//!
//! \brief DecoderCvDialog::getOrCreateFormLayout
//! \param layout
//! \return
//!
QFormLayout* DecoderCvDialog::getOrCreateFormLayout(QVBoxLayout* layout) {
    QLayoutItem* lastItem = (layout->count() > 0) ? layout->itemAt(layout->count() - 1) : nullptr;
    QFormLayout* form = (lastItem) ? qobject_cast<QFormLayout*>(lastItem->layout()) : nullptr;

    if (!form) {
        form = new QFormLayout();
        layout->addLayout(form);
    }
    return form;
}
//!
//! \brief DecoderCvDialog::stringToType
//! \param typeStr
//! \param groupTypeStr
//! \return
//!
CvType DecoderCvDialog::stringToType(const QString& typeStr, const QString& groupTypeStr) {
    if (groupTypeStr == "dccSpeedCurve") return CvType::DccSpeedCurve;
    if (typeStr == "byte") return CvType::Byte;
    if (typeStr == "select") return CvType::Select;
    if (typeStr == "signedByte") return CvType::SignedByte;
    return CvType::Unknown;
}
//!
//! \brief DecoderCvDialog::onReadClicked
//!
void DecoderCvDialog::onReadClicked()
{
    // TODO: Dein Netzwerkcode für das Lesen ("read"-Methode des Objekts)
    m_statusBar->showMessage(tr("Lese CV..."));
}

void DecoderCvDialog::onWriteClicked()
{
    // TODO: Dein Netzwerkcode für das Schreiben ("write"-Methode des Objekts)
    m_statusBar->showMessage(tr("Schreibe CV..."));
}

void DecoderCvDialog::onStopClicked()
{
    // TODO: Sende Stop-Befehl an das Objekt
    m_statusBar->clearMessage();
}

void DecoderCvDialog::updateButtonsEnabled()
{
    // Wir prüfen einfach, ob im linken ListWidget überhaupt ein Bereich ausgewählt ist
    if (m_sideList && m_sideList->currentRow() >= 0) {
        m_readBtn->setEnabled(true);
        m_writeBtn->setEnabled(true);
    } else {
        // Falls die Liste leer sein sollte (Sicherheitsnetz)
        m_readBtn->setEnabled(false);
        m_writeBtn->setEnabled(false);
    }
}
