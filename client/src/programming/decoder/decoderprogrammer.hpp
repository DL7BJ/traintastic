#ifndef TRAINTASTIC_CLIENT_PROGRAMMING_DECODERPROGRAMMER_HPP
#define TRAINTASTIC_CLIENT_PROGRAMMING_DECODERPROGRAMMER_HPP

#include <QWidget>
#include <QHash>
#include <QPair>
#include <QJsonArray>
#include <memory>
#include "../../network/objectptr.hpp"

class Connection;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class QPushButton;
class QStatusBar;

//! Struktur für Herstellerdaten
struct ManufacturerInfo {
    int id;
    int extendedId;
    QString Name;
    QString shortName;
    QString uri;
};
using ManufacturerKey = QPair<int, int>;

class DecoderProgrammer final : public QWidget
{
  Q_OBJECT

  public:
    explicit DecoderProgrammer(std::shared_ptr<Connection> connection, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~DecoderProgrammer() final;

  private slots:
    void onManufacturerChanged();
    void onDecoderSelectionChanged();
    void onOpenProgrammerClicked();

  private:
    void loadInterfaces();
    bool loadManufacturersJSON();
    void loadFirmwareDetailsJSON();
    void filterDecodersForManufacturer(int manufacturerId, int extendedId);
    void updateFilterButtonText();
    void loadUserSettings();
    void saveUserSettings();

    std::shared_ptr<Connection> m_connection;
    int m_requestId;

    // UI-Elemente für das Zwei-Spalten-Layout
    QComboBox* m_interface;
    QListWidget* m_manufacturerList;
    QListWidget* m_decoderList;
    QPushButton* m_toggleFilterBtn;
    QPushButton* m_openProgrammerBtn;
    QStatusBar* m_statusBar;

    // Daten-Speicher
    QHash<int, ManufacturerInfo> m_manufacturers;
    QJsonArray m_firmwareDetailsArray;
    bool m_isFilterActive = false;
    QString m_defaultInterface;
    QList<int> m_favoriteManufacturers;
};

#endif // TRAINTASTIC_CLIENT_PROGRAMMING_DECODERPROGRAMMER_HPP
