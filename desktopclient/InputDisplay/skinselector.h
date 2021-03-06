#ifndef SKINSELECTOR_H
#define SKINSELECTOR_H

#include "inputdisplay.h"
#include "skinparser.h"
#include "../telnetconnection.h"
#include "../inputdecoder.h"

#include <QListView>
#include <QMainWindow>
#include <QSettings>
#include <QStandardItemModel>

namespace Ui {
class SkinSelector;
}

class SkinSelector : public QMainWindow
{
    Q_OBJECT

public:
    explicit SkinSelector(QWidget *parent = nullptr);
    ~SkinSelector();

private slots:
    void on_startButton_clicked();

    void on_pianoCheckBox_stateChanged(int arg1);

    void on_skinListView_clicked(const QModelIndex &index);

    void    onTelnetConnected();
    void    onTimerTimeout();

    void on_skinPathButton_clicked();

    void on_subSkinListView_clicked(const QModelIndex &index);

    void    onInputConnected();
    void    onDisplayClosed();

    void onInputNewLine(QByteArray data);
private:
    Ui::SkinSelector *ui;
    QStandardItemModel* listModel;
    QStandardItemModel* pianoModel;
    QStandardItemModel* subSkinModel;
    InputDisplay*       display;
    InputDecoder*       snesClassicDecoder;
    TelnetConnection*   testCo;
    TelnetConnection*   inputCo;
    QTimer              timer;
    QSettings*          m_settings;
    RegularSkin         currentSkin;

    void    setSkinPath(QString path);
    void    setPreviewScene(const RegularSkin &skin);
    void    saveSkinStarted();
    void    restoreLastSkin();
};

#endif // SKINSELECTOR_H
