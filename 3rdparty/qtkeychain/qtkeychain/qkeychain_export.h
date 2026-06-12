
#ifndef QKEYCHAIN_EXPORT_H
#define QKEYCHAIN_EXPORT_H

#ifdef QT6KEYCHAIN_STATIC_DEFINE
#  define QKEYCHAIN_EXPORT
#  define QT6KEYCHAIN_NO_EXPORT
#else
#  ifndef QKEYCHAIN_EXPORT
#    ifdef qt6keychain_EXPORTS
        /* We are building this library */
#      define QKEYCHAIN_EXPORT 
#    else
        /* We are using this library */
#      define QKEYCHAIN_EXPORT 
#    endif
#  endif

#  ifndef QT6KEYCHAIN_NO_EXPORT
#    define QT6KEYCHAIN_NO_EXPORT 
#  endif
#endif

#ifndef QT6KEYCHAIN_DEPRECATED
#  define QT6KEYCHAIN_DEPRECATED __declspec(deprecated)
#endif

#ifndef QT6KEYCHAIN_DEPRECATED_EXPORT
#  define QT6KEYCHAIN_DEPRECATED_EXPORT QKEYCHAIN_EXPORT QT6KEYCHAIN_DEPRECATED
#endif

#ifndef QT6KEYCHAIN_DEPRECATED_NO_EXPORT
#  define QT6KEYCHAIN_DEPRECATED_NO_EXPORT QT6KEYCHAIN_NO_EXPORT QT6KEYCHAIN_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef QT6KEYCHAIN_NO_DEPRECATED
#    define QT6KEYCHAIN_NO_DEPRECATED
#  endif
#endif

#endif /* QKEYCHAIN_EXPORT_H */
