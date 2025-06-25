#ifndef BLOONSFARM_BLOONSUIPOPUP_H
#define BLOONSFARM_BLOONSUIPOPUP_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QLabel>

class BloonsUIPopup : public QMainWindow {
public:
    BloonsUIPopup(const QString &title, const QString &popupMessage);

};

#endif