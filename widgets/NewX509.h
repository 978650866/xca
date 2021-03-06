/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2001 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */


#ifndef __NEWX509_H
#define __NEWX509_H

#include "ui_NewX509.h"
#include "lib/oid.h"
#include "lib/db.h"
#include "widgets/kvView.h"
#include <openssl/x509v3.h>
#include <QtGui/QListWidget>

class MainWindow;
class extList;
class pki_temp;
class pki_x509req;
class pki_x509;
class pki_key;
class QPixmap;
class QListbox;
class x509name;
class x509v3ext;
class extList;

class NewX509: public QDialog, public Ui::NewX509
{
		Q_OBJECT
	private:
		NIDlist eku_nid;
		NIDlist dn_nid;
		NIDlist aia_nid;
		NIDlist attr_nid;
		QList<QLineEdit*> attr_edit;
		#define EXPLICIT_NAME_CNT 7
		QLineEdit *name_ptr[EXPLICIT_NAME_CNT];
		X509V3_CTX ext_ctx;
		void editV3ext(QLineEdit *le, QString types, int n);
		enum pki_type pt;
		void templateChanged(QString templatename);
		QString mandatoryDnRemain();
		QStringList private_keys, private_keys0, tabnames;
		pki_x509 *ctx_cert;
		QString v3ext_backup;
		QString valid_htmltext;
		kvmodel *extDNmodel;

	public:
		static int name_nid[EXPLICIT_NAME_CNT];
		QRadioButton *selfQASignRB;
		NewX509(QWidget *parent);
		virtual ~NewX509();
		void initCtx();
		void setRequest(); // reduce to request form
		void setTemp(pki_temp *temp); // reduce to template form
		void setCert(); // reduce to certificate form
		void toTemplate(pki_temp *temp);
		void fromTemplate(pki_temp *temp);
		void defineTemplate(pki_temp *temp);
		void defineRequest(pki_x509req *req);
		void defineSigner(pki_x509 *defcert);
		void templateChanged(pki_temp *templ);
		pki_key *getSelectedKey();
		pki_x509 *getSelectedSigner();
		pki_x509req *getSelectedReq();
		x509name getX509name(int _throw = 0);
		void setX509name(const x509name &n);
		void setImage(QPixmap *image);
		void setAuthInfAcc_string(QString aia_txt);
		QString getAuthInfAcc_string();
		x509v3ext getBasicConstraints();
		x509v3ext getSubKeyIdent();
		x509v3ext getAuthKeyIdent();
		x509v3ext getKeyUsage();
		x509v3ext getEkeyUsage();
		x509v3ext getSubAltName();
		x509v3ext getIssAltName();
		x509v3ext getCrlDist();
		x509v3ext getAuthInfAcc();
		extList getGuiExt();
		extList getNetscapeExt();
		extList getAdvanced();
		extList getAllExt();
		void setupTmpCtx();
		void initCtx(pki_x509 *subj, pki_x509 *iss, pki_x509req *req);
		void setExt(const x509v3ext &ext);
		void switchHashAlgo();
		void addReqAttributes(pki_x509req *req);
		int checkExtDuplicates();
		void subjectFromTemplate(pki_temp *temp);
		void extensionsFromTemplate(pki_temp *temp);
		pki_temp *currentTemplate();
		void gotoTab(int tab);
		void setupLineEditByNid(int nid, QLineEdit *l);

	public slots:
		void on_fromReqCB_clicked();
		void on_keyList_currentIndexChanged(const QString &);
		void on_reqList_currentIndexChanged(const QString &);
		void newKeyDone(QString name);
		void on_applyTime_clicked();
		void on_editSubAlt_clicked();
		void on_editIssAlt_clicked();
		void on_editCrlDist_clicked();
		void on_editAuthInfAcc_clicked();
		void on_foreignSignRB_toggled(bool checked);
		void on_genKeyBut_clicked();
		void on_showReqBut_clicked();
		void on_certList_currentIndexChanged(int index);
		void on_applyTemplate_clicked();
		void on_applySubject_clicked();
		void on_applyExtensions_clicked();
		void on_adv_validate_clicked();
		void on_usedKeysToo_toggled(bool checked);
		void on_tabWidget_currentChanged(int idx);
		void on_reqSubChange_clicked();
		void accept();
		void setupExtDNwidget(const QString &s, QLineEdit *w);

	signals:
		void genKey(QString);
		void showReq(QString req);
};

#endif
