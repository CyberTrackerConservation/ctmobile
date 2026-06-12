#include "fxTranslate.h"

#include <QCoreApplication>
#include <QSet>

static const char * const kClassicStrings[] = {
    QT_TRANSLATE_NOOP("Classic", "Please wait..."),
    QT_TRANSLATE_NOOP("Classic", "Tap to capture"),
    QT_TRANSLATE_NOOP("Classic", "Tap to retry"),
    QT_TRANSLATE_NOOP("Classic", "Disconnected"),
    QT_TRANSLATE_NOOP("Classic", "Connecting"),
    QT_TRANSLATE_NOOP("Classic", "Selection is required"),
    QT_TRANSLATE_NOOP("Classic", "Picture is required"),
    QT_TRANSLATE_NOOP("Classic", "Phone number not supported"),
    QT_TRANSLATE_NOOP("Classic", "GPS not acquired"),
    QT_TRANSLATE_NOOP("Classic", "Time not found"),
    QT_TRANSLATE_NOOP("Classic", "GPS needed for time"),
    QT_TRANSLATE_NOOP("Classic", "Download failed"),
    QT_TRANSLATE_NOOP("Classic", "Year"),
    QT_TRANSLATE_NOOP("Classic", "Month"),
    QT_TRANSLATE_NOOP("Classic", "Day"),
    QT_TRANSLATE_NOOP("Classic", "Today"),
    QT_TRANSLATE_NOOP("Classic", "Recording is required"),
    QT_TRANSLATE_NOOP("Classic", "Recording disabled"),
    QT_TRANSLATE_NOOP("Classic", "Delete existing recording?"),
    QT_TRANSLATE_NOOP("Classic", "Text is required"),
    QT_TRANSLATE_NOOP("Classic", "No Result: Text not saved"),
    QT_TRANSLATE_NOOP("Classic", "Flag not placed"),
    QT_TRANSLATE_NOOP("Classic", "Barcode is required"),
    QT_TRANSLATE_NOOP("Classic", "Value not set"),
    QT_TRANSLATE_NOOP("Classic", "Number not set"),
    QT_TRANSLATE_NOOP("Classic", "No Result: Value not saved"),
    QT_TRANSLATE_NOOP("Classic", "Tap to edit"),
};

static const QSet<QByteArray> &classicKeys()
{
    static const QSet<QByteArray> keys = [] {
        QSet<QByteArray> s;
        for (auto *p : kClassicStrings)
            s.insert(QByteArray(p));
        return s;
    }();
    return keys;
}

const char *fxTranslate(const char *text)
{
    if (!text) return "";

    if (!classicKeys().contains(QByteArray(text)))
    {
        return text;
    }

    static QByteArray result;
    result = QCoreApplication::translate("Classic", text).toUtf8();
    return result.constData();
}
