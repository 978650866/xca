/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2009 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */

#ifndef __XCA_PKCS11_H
#define __XCA_PKCS11_H

#include "pkcs11_lib.h"
#include "opensc-pkcs11.h"
#include <QtCore/QStringList>
#include <QtCore/QString>
#include <QtCore/QList>

#include <ltdl.h>

#include "pk11_attribute.h"

#define WAITCURSOR_START QApplication::setOverrideCursor(QCursor(Qt::WaitCursor))
#define WAITCURSOR_END QApplication::restoreOverrideCursor()

class tkInfo
{
private:
	CK_TOKEN_INFO token_info;
public:
	tkInfo(const CK_TOKEN_INFO *ti)
	{
		set(ti);
	}
	tkInfo(const tkInfo &tk)
	{
		set(&tk.token_info);
	}
	void set(const CK_TOKEN_INFO *ti)
	{
		memcpy(&token_info, ti, sizeof(token_info));
		// sanitize strings
		for (int i=0; i<32; i++) {
			if (token_info.label[i] == 0)
				token_info.label[i] = ' ';
			if (token_info.manufacturerID[i] == 0)
				token_info.manufacturerID[i] = ' ';
		}
		for (int i=0; i<16; i++) {
			if (token_info.model[i] == 0)
				token_info.model[i] = ' ';
			if (token_info.serialNumber[i] == 0)
				token_info.serialNumber[i] = ' ';
		}
	}
	QString label() const
	{
		return UTF8QSTRING(token_info.label, 32);
	}
	QString manufacturerID() const
	{
		return UTF8QSTRING(token_info.manufacturerID, 32);
	}
	QString model() const
	{
		return UTF8QSTRING(token_info.model, 16);
	}
	QString serial() const
	{
		return ASCIIQSTRING(token_info.serialNumber, 16);
	}
	bool protAuthPath() const
	{
		return !!(token_info.flags & CKF_PROTECTED_AUTHENTICATION_PATH);
	}
	bool tokenInitialized() const
	{
		return !!(token_info.flags & CKF_TOKEN_INITIALIZED);
	}
	QString pinInfo() const
	{
		return QObject::tr("Required PIN size: %1 - %2").
			arg(token_info.ulMinPinLen).
			arg(token_info.ulMaxPinLen);
	}
};

class pkcs11
{
	friend class pk11_attribute;
	friend class pk11_attr_ulong;
	friend class pk11_attr_data;

	private:
		static pkcs11_lib_list libs;
		slotid p11slot;
		CK_SESSION_HANDLE session;
		CK_OBJECT_HANDLE p11obj;

	public:
		pkcs11();
		~pkcs11();

		static bool loaded() {
			return libs.count() != 0;
		}
		static pkcs11_lib *load_lib(QString fname, bool silent);
		static pkcs11_lib *get_lib(QString fname)
		{
			return libs.get_lib(fname);
		}
		static bool remove_lib(QString fname)
		{
			return libs.remove_lib(fname);
		}
		static void load_libs(QString list, bool silent)
		{
			QStringList errs;
			foreach(QString l, list.split('\n')) {
				try {
					pkcs11::load_lib(l, silent);
				} catch (errorEx &err) {
					errs << err.getString();
				}
			}
			if (errs.count())
				throw errorEx(errs.join("\n"));
		}
		static pkcs11_lib_list get_libs()
		{
			return libs;
		}
		tkInfo tokenInfo(slotid slot);
		tkInfo tokenInfo()
		{
			return tokenInfo(p11slot);
		}
		QString driverInfo(slotid slot)
		{
			return slot.lib->driverInfo();
		}
		slotidList getSlotList()
		{
			return libs.getSlotList();
		}

		bool selectToken(slotid *slot, QWidget *w);
		void changePin(slotid slot, bool so);
		void initPin(slotid slot);
		void initToken(slotid slot, unsigned char *pin,
			int pinlen, QString label);
		QList<CK_MECHANISM_TYPE> mechanismList(slotid slot);
		void mechanismInfo(slotid slot, CK_MECHANISM_TYPE m,
			CK_MECHANISM_INFO *info);
		void startSession(slotid slot, bool rw = false);

		/* Session based functions */
		void loadAttribute(pk11_attribute &attribute,
				   CK_OBJECT_HANDLE object);
		void storeAttribute(pk11_attribute &attribute,
				   CK_OBJECT_HANDLE object);
		QList<CK_OBJECT_HANDLE> objectList(pk11_attlist &atts);
		QString tokenLogin(QString name, bool so, bool force=false);
		void logout();
		bool needsLogin(bool so);
		void login(unsigned char *pin, unsigned long pinlen, bool so);

		void setPin(unsigned char *oldPin, unsigned long oldPinLen,
			unsigned char *pin, unsigned long pinLen);
		CK_OBJECT_HANDLE createObject(pk11_attlist &attrs);
		pk11_attr_data findUniqueID(unsigned long oclass);
		pk11_attr_data generateRSAKey(QString name, unsigned long bits);
		int deleteObjects(QList<CK_OBJECT_HANDLE> objects);
		EVP_PKEY *getPrivateKey(EVP_PKEY *pub, CK_OBJECT_HANDLE obj);
		int encrypt(int flen, const unsigned char *from,
					unsigned char *to, int tolen);
		int decrypt(int flen, const unsigned char *from,
					unsigned char *to, int tolen);

};

#endif
