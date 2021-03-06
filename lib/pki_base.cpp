/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2001 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */


#include "func.h"
#include "pki_base.h"
#include "exception.h"
#include <QtCore/QString>

int pki_base::pki_counter = 0;

pki_base::pki_base(const QString name, pki_base *p)
{
	desc = name;
	class_name = "pki_base";
	parent = p;
	pki_counter++;
	childItems.clear();
	dataVersion=0;
	pkiType=none;
}

int pki_base::getVersion()
{
	return dataVersion;
}

enum pki_type pki_base::getType()
{
	return pkiType;
}

pki_base::~pki_base(void)
{
	while((childCount()))
		delete takeFirst();
	pki_counter--;
}


QString pki_base::getIntName() const
{
	return desc;
}

QString pki_base::getUnderlinedName() const
{
	QString a = getIntName();
	int i, l = a.length();

	for (i=0; i<l; i++) {
		if (a[i] == ' ' || a[i] == '&' || a[i] == ';' || a[i] == '`')
			a[i] = '_';
	}

	return a;
}

int pki_base::get_pki_counter()
{
	return pki_counter;
}

QString pki_base::getClassName()
{
	QString x = class_name;
	return x;
}

void pki_base::setIntName(const QString &d)
{
	desc = d;
}

void pki_base::fopen_error(const QString fname)
{
	QString txt = tr("Error opening file: '%1': %2").arg(fname).
			arg(strerror(errno));
	throw errorEx(txt, class_name);
}

void pki_base::my_error(const QString error) const
{
	if (!error.isEmpty()) {
		fprintf(stderr, "%s\n", CCHAR(tr("Error: ") + error));
		throw errorEx(error, class_name);
	}
}

QString pki_base::rmslashdot(const QString &s)
{
	QByteArray a = s.toAscii();
	int r = a.lastIndexOf('.');
#ifdef WIN32
	int l = a.lastIndexOf('\\');
#else
	int l = a.lastIndexOf('/');
#endif
	//printf("r=%d, l=%d, s='%s', mid='%s'\n",r,l,(const char*)a,
	//		CCHAR(s.mid(l+1,r-l-1)));
	return s.mid(l+1,r-l-1);
}

pki_base *pki_base::getParent()
{
	return parent;
}

void pki_base::setParent(pki_base *p)
{
	parent = p;
}

pki_base *pki_base::child(int row)
{
	return childItems.value(row);
}

void pki_base::append(pki_base *item)
{
	childItems.append(item);
	item->setParent(this);
}

void pki_base::insert(int row, pki_base *item)
{
	childItems.insert(row, item);
	item->setParent(this);
}

int pki_base::childCount()
{
	return childItems.count();
}

int pki_base::row(void) const
{
	if (parent)
		return parent->childItems.indexOf(const_cast<pki_base*>(this));
	return 0;
}

pki_base *pki_base::iterate(pki_base *pki)
{
	if (pki == NULL)
		pki = (childItems.isEmpty()) ? NULL : childItems.first();
	else
		pki = childItems.value(pki->row()+1);

	if (pki) {
		return pki;
	}
	if (!parent) {
		return NULL;
	}
	return parent->iterate(this);
}

void pki_base::takeChild(pki_base *pki)
{
	childItems.takeAt(pki->row());
}

pki_base *pki_base::takeFirst()
{
	return childItems.takeFirst();
}

QVariant pki_base::column_data(int id)
{
	switch (id) {
	case HD_internal_name:
		return QVariant(getIntName());
	}
	return QVariant();
}

QVariant pki_base::getIcon(int)
{
	return QVariant();
}

uint32_t pki_base::intFromData(QByteArray &ba)
{
	/* For import "oldFromData" use the endian dependent version */
	uint32_t ret;
	if ((unsigned)(ba.count()) < sizeof(uint32_t)) {
		ba.clear();
		return 0;
	}
	memcpy(&ret, ba.constData(), sizeof(uint32_t));
	ba = ba.mid(sizeof(uint32_t));
	return ret;
}

bool pki_base::compare(pki_base *refcrl)
{
	bool ret;
	ret = (i2d() == refcrl->i2d());
	pki_openssl_error();
	return ret;
}
