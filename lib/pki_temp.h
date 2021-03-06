/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2001 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */

#ifndef __PKI_TEMP_H
#define __PKI_TEMP_H

#include "pki_base.h"
#include "x509name.h"
#include "asn1time.h"
#include "pki_x509.h"

class pki_temp: public pki_x509name
{
		Q_OBJECT
	protected:
		int dataSize();
	public:
		static QPixmap *icon;
		x509name xname;
		QString subAltName, issAltName, crlDist, authInfAcc, certPol;
		QString nsComment, nsBaseUrl, nsRevocationUrl,
			nsCARevocationUrl, nsRenewalUrl, nsCaPolicyUrl,
			nsSslServerName, destination, adv_ext, eKeyUse, pathLen;
		bool bcCrit, keyUseCrit, eKeyUseCrit, subKey, authKey,
			validMidn, noWellDefined;
		int nsCertType, keyUse, ca;
		int validN, validM;

		// methods
		extList fromCert(pki_x509super *cert_or_req);

		pki_temp(const pki_temp *pk);
		pki_temp(const QString d = QString());
		void fload(const QString fname);
		void writeDefault(const QString fname);
		~pki_temp();
		void fromData(const unsigned char *p, int size, int version);
		void fromData(const unsigned char *p, db_header_t *head );
		void oldFromData(unsigned char *p, int size);

		QByteArray toData();
		bool compare(pki_base *ref);
		void writeTemp(QString fname);
		QVariant column_data(int id);
		QVariant getIcon(int id);
		virtual QString getMsg(msg_type msg);
		x509name getSubject() const;
};

#endif
