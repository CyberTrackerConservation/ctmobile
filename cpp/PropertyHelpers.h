#pragma once
#include <QObject>

// Find the cheapest type between T and const T &.
template<typename T> struct CheapestType          { typedef const T & type_def; };
template<>           struct CheapestType<bool>    { typedef bool      type_def; };
template<>           struct CheapestType<quint8>  { typedef quint8    type_def; };
template<>           struct CheapestType<quint16> { typedef quint16   type_def; };
template<>           struct CheapestType<quint32> { typedef quint32   type_def; };
template<>           struct CheapestType<quint64> { typedef quint64   type_def; };
template<>           struct CheapestType<qint8>   { typedef qint8     type_def; };
template<>           struct CheapestType<qint16>  { typedef qint16    type_def; };
template<>           struct CheapestType<qint32>  { typedef qint32    type_def; };
template<>           struct CheapestType<qint64>  { typedef qint64    type_def; };
template<>           struct CheapestType<float>   { typedef float     type_def; };
template<>           struct CheapestType<double>  { typedef double    type_def; };
template<typename T> struct CheapestType<T *>     { typedef T *       type_def; };

#define MAKE_GETTER_NAME(name) name

#define QML_AUTO_GETTER(type, name) \
    CheapestType<type>::type_def MAKE_GETTER_NAME(name) (void) const { \
        return m_##name; \
    }

#define QML_AUTO_SETTER(type, name, prefix) \
    bool prefix##name (CheapestType<type>::type_def name) { \
        if (m_##name != name) { \
            m_##name = name; \
            emit name##Changed (); \
            return true; \
        } \
        else { \
            return false; \
        } \
    }

#define QML_AUTO_NOTIFIER(type, name) \
    void name##Changed (void);

#define QML_AUTO_MEMBER(type, name) \
    type m_##name;

#define QML_WRITABLE_AUTO_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ MAKE_GETTER_NAME(name) WRITE set_##name NOTIFY name##Changed) \
    protected: \
        QML_AUTO_MEMBER (type, name) \
    public: \
        QML_AUTO_GETTER (type, name) \
        QML_AUTO_SETTER (type, name, set_) \
    Q_SIGNALS: \
        QML_AUTO_NOTIFIER (type, name) \
    private:

#define QML_READONLY_AUTO_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ MAKE_GETTER_NAME(name) NOTIFY name##Changed) \
    protected: \
        QML_AUTO_MEMBER (type, name) \
    public: \
        QML_AUTO_GETTER (type, name) \
        QML_AUTO_SETTER (type, name, update_) \
    Q_SIGNALS: \
        QML_AUTO_NOTIFIER (type, name) \
    private:

#define QML_CONSTANT_AUTO_PROPERTY(type, name) \
    protected: \
        Q_PROPERTY (type name READ MAKE_GETTER_NAME(name) CONSTANT) \
    protected: \
        QML_AUTO_MEMBER (type, name) \
    public: \
        QML_AUTO_GETTER (type, name) \
    private:

#define QML_DOUBLE_SETTER(type, name, prefix) \
    bool prefix##name (CheapestType<type>::type_def name) { \
        if (std::isnan(m_##name) || !qFuzzyIsNull(m_##name -  name)) { \
            m_##name = name; \
            emit name##Changed (); \
            return true; \
        } \
        else { \
            return false; \
        } \
    }

#define QML_WRITABLE_DOUBLE_PROPERTY(name) \
    protected: \
        Q_PROPERTY (double name READ MAKE_GETTER_NAME(name) WRITE set_##name NOTIFY name##Changed) \
    protected: \
        QML_AUTO_MEMBER (double, name) \
    public: \
        QML_AUTO_GETTER (double, name) \
        QML_DOUBLE_SETTER (double, name, set_) \
    Q_SIGNALS: \
        QML_AUTO_NOTIFIER (double, name) \
    private:

// Simple no properties - no notifiers.
#define Q_AUTO_GETTER(type, name) \
    CheapestType<type>::type_def MAKE_GETTER_NAME(name) (void) const { \
        return m_##name; \
    }

#define Q_AUTO_SETTER(type, name, prefix) \
    void prefix##name (CheapestType<type>::type_def name) { \
        m_##name = name; \
    }

#define Q_AUTO_MEMBER(type, name) \
    type m_##name;


#define Q_WRITABLE_AUTO_PROPERTY(type, name) \
    protected: \
        Q_AUTO_MEMBER (type, name) \
    public: \
        Q_AUTO_GETTER (type, name) \
        Q_AUTO_SETTER (type, name, set_) \
    private:

#define Q_READONLY_AUTO_PROPERTY(type, name) \
    protected: \
        Q_AUTO_MEMBER (type, name) \
    public: \
        Q_AUTO_GETTER (type, name) \
        Q_AUTO_SETTER (type, name, update_) \
    private:

// Settings.

template <typename T> class variantType
{};

template <> class variantType<bool>
{
public:
    static bool toType(const QVariant& value) { return value.toBool(); }
    static bool equals(bool a, bool b) { return a == b; }
};

template <> class variantType<int>
{
public:
    static int toType(const QVariant& value) { return value.toInt(); }
    static bool equals(int a, int b) { return a == b; }
};

template <> class variantType<qlonglong>
{
public:
    static qlonglong toType(const QVariant& value) { return value.toLongLong(); }
    static bool equals(qlonglong a, qlonglong b) { return a == b; }
};

template <> class variantType<float>
{
public:
    static float toType(const QVariant& value) { return value.toFloat(); }
    static bool equals(float a, float b) { return a == b; }
};

template <> class variantType<QString>
{
public:
    static QString toType(const QVariant& value) { return value.toString(); }
    static bool equals(const QString& a, const QString& b) { return a == b; }
};

template <> class variantType<QVariantMap>
{
public:
    static QVariantMap toType(const QVariant& value) { return value.toMap(); }
    static bool equals(const QVariantMap& a, const QVariantMap& b) { return a == b; }
};

template <> class variantType<QVariantList>
{
public:
    static QVariantList toType(const QVariant& value) { return value.toList(); }
    static bool equals(const QVariantList& a, const QVariantList& b) { return a == b; }
};

#define QML_SETTING(type, name, defaultValue) \
    protected: \
        Q_PROPERTY (type name READ name WRITE set_##name NOTIFY name##Changed) \
    Q_SIGNALS: \
        void name##Changed(); \
    public: \
        type name() const { \
            return variantType<type>::toType(getSetting(#name, defaultValue)); \
        } \
        void set_##name(type value) { \
            if (!variantType<type>::equals(value, name())) { \
                setSetting(#name, value); \
                emit name##Changed(); \
            } \
        } \
    private: \
