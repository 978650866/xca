/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2001 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */

#ifndef __FUNC_H
#define __FUNC_H

#include <openssl/asn1.h>
#include <QtGui/QPixmap>
#include <QtCore/QByteArray>
#include "base.h"

class Validity;

QPixmap *loadImg(const char *name);
QString getPrefix();
QString getHomeDir();
QString getLibDir();
QString getDocDir();
QString getUserSettingsDir();
QString getFullFilename(const QString &filename, const QString &selectedFilter);

QByteArray filename2bytearray(const QString &fname);
QString filename2QString(const char *fname);

QString asn1ToQString(const ASN1_STRING *str, bool quote = false);
ASN1_STRING *QStringToAsn1(QString s, int nid);

const char *OBJ_ln2sn(const char *ln);
const char *OBJ_sn2ln(const char *sn);
const char *OBJ_obj2sn(ASN1_OBJECT *a);
QString OBJ_obj2QString(ASN1_OBJECT *a, int no_name = 0);

void inc_progress_bar(int, int, void *p);

#define openssl_error(x) _openssl_error(QString(x), __FILE__, __LINE__)
#define ign_openssl_error(x) _ign_openssl_error(QString(x), __FILE__, __LINE__)
void _openssl_error(const QString txt, const char *file, int line);
bool _ign_openssl_error(const QString txt, const char *file, int line);

QByteArray i2d_bytearray(int(*i2d)(const void*, unsigned char**), const void*);
void *d2i_bytearray(void *(*d2i)(void*, unsigned char**, long),
		QByteArray &ba);

#define I2D_VOID(a) ((int (*)(const void *, unsigned char **))(a))
#define D2I_VOID(a) ((void *(*)(void *, unsigned char **, long))(a))

#define QString2filename(str) filename2bytearray(str).constData()
#endif
