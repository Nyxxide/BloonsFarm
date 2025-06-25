#include "BloonsUIPopup.h"

BloonsUIPopup::BloonsUIPopup(const QString &title, const QString &popupMessage) {
    // Window setup
    this->setWindowTitle(title);
    this->setWindowIcon(QIcon("../resources/UI/btdfarmicon.ico"));
    this->setGeometry(550, 250, 300, 100);
    this->setWindowModality(Qt::ApplicationModal);

    // Layout setup
    auto* popupWindow = new QWidget(this);
    auto* popupLayout = new QVBoxLayout();
    auto* popupHBox = new QHBoxLayout();

    // Font setup
    auto font_id = QFontDatabase::addApplicationFont("../resources/UI/LuckiestGuy-Regular.ttf");
    auto font_name = QFontDatabase::applicationFontFamilies(font_id)[0];
    auto BTDFont = QFont(font_name);

    // Popup Message Label setup
    auto* popupLabel = new QLabel(popupMessage, this);
    BTDFont.setPointSize(15);
    popupLabel->setFont(BTDFont);

    // Assemble Layout
    popupHBox->addWidget(popupLabel);
    popupLayout->addLayout(popupHBox);
    popupWindow->setLayout(popupLayout);
    setCentralWidget(popupWindow);
}