#ifndef INPUTDECODER_H
#define INPUTDECODER_H

#include <QObject>
#include <QMap>

struct RawInputEvent
{
    quint32 time_s;
    quint32 time_us;
    quint16 type;
    quint16 code;
    quint32 value;
};


class InputDecoder : public QObject
{
    Q_OBJECT
public:
    enum SNESButton
    {
        Up = 0x2C2,
        Down = 0x2C3,
        Left = 0x2C0,
        Right = 0x2C1,
        X = 0x133,
        A = 0x130,
        B = 0x131,
        Y = 0x134,
        L = 2,
        R = 5,
        Start = 0x13B,
        Select = 0x13A
    };

    Q_ENUM(SNESButton)
    InputDecoder();

signals:
    void    buttonPressed(InputDecoder::SNESButton button);
    void    buttonReleased(InputDecoder::SNESButton button);

public slots:
    void    decodeHexdump(QString toDecode);

private:
    void processEvent(RawInputEvent ev);
    bool    Lpressed;
    bool    Rpressed;
};

#endif // INPUTDECODER_H
