#ifndef INPUTDECODER_H
#define INPUTDECODER_H

#include <QObject>
#include <QMap>

enum SNESButton
{
    Up,
    Down,
    Left,
    Right,
    X,
    A,
    B,
    Y,
    L,
    R,
    Start,
    Select
};

class InputDecoder : public QObject
{
    Q_OBJECT
public:
    InputDecoder();

signals:
    void    buttonPressed(SNESButton button);
    void    buttonReleased(SNESButton button);

public slots:
    void    decodeHexdump(QString toDecode);

private:
    QMap<QString, SNESButton>  strToKey;
};

#endif // INPUTDECODER_H
