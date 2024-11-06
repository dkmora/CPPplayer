#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(QTFFPLAY_LIB)
#  define QTFFPLAY_EXPORT Q_DECL_EXPORT
# else
#  define QTFFPLAY_EXPORT Q_DECL_IMPORT
# endif
#else
# define QTFFPLAY_EXPORT
#endif
