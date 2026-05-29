#ifndef TRAINTASTIC_CLIENT_PROGRAMMING_DECODERCVDIALOG_HPP
#define TRAINTASTIC_CLIENT_PROGRAMMING_DECODERCVDIALOG_HPP

#include <QDialog>
#include <memory>
#include "../../network/objectptr.hpp"

// Vorwärtsdeklarationen für den Compiler
class Connection;
class QListWidget;
class QStackedWidget;
class QPushButton;
class QStatusBar;
class QVBoxLayout;
class QFormLayout;

enum class CvType {
    Unknown,
    Bit,
    Byte,
    SignedByte,
    Select,
    DccSpeedCurve
};

class DecoderCvDialog final : public QDialog
{
    Q_OBJECT

  public:
    explicit DecoderCvDialog(std::shared_ptr<Connection> connection,
                             const QString& interfaceName,
                             const QString& cvJsonPath,
                             QWidget* parent = nullptr);
    ~DecoderCvDialog() final;

  private slots:
    void onReadClicked();
    void onWriteClicked();
    void onStopClicked();
    void updateButtonsEnabled();

  private:
    void setupUi();
    void loadCvDefinitionJson();
    void renderCvElement(const QJsonObject& cv, QVBoxLayout* layout);
    CvType stringToType(const QString& typeStr, const QString& groupTypeStr);
    QFormLayout* getOrCreateFormLayout(QVBoxLayout* layout);
    void renderSpeedCurve(const QJsonArray& allCvs, QVBoxLayout* layout);
    QJsonObject filterCv29ForSpeed(const QJsonObject& originalCv);

    std::shared_ptr<Connection> m_connection;
    int m_requestId;

    QString m_interfaceName;
    QString m_cvJsonPath;

    // --- NEU: Liste links und Stack rechts für die grafischen CVs ---
    QListWidget* m_sideList = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;

    // Die Buttons und die Statusbar bleiben wie gehabt
    QPushButton* m_readBtn = nullptr;
    QPushButton* m_writeBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QStatusBar* m_statusBar = nullptr;
};

#endif // TRAINTASTIC_CLIENT_PROGRAMMING_DECODERCVDIALOG_HPP
