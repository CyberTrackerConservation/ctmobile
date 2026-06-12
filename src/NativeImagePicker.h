#pragma once
#include "pch.h"

class NativeImagePicker : public QObject
{
    Q_OBJECT

public:
    explicit NativeImagePicker(QObject *parent = nullptr);

    Q_INVOKABLE void open();

signals:
    void accepted(const QString &fileUrl);
    void rejected();
};
