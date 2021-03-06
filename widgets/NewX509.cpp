/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2001 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */


#include "NewX509.h"
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QRadioButton>
#include <QtGui/QMessageBox>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QPushButton>
#include <QtGui/QValidator>
#include "MainWindow.h"
#include "v3ext.h"
#include "lib/x509name.h"
#include "lib/db_key.h"
#include "lib/db_x509req.h"
#include "lib/db_x509.h"
#include "lib/db_temp.h"
#include "lib/oid.h"
#include "lib/func.h"

int NewX509::name_nid[] = {
	NID_countryName,
	NID_stateOrProvinceName,
	NID_localityName,
	NID_organizationName,
	NID_organizationalUnitName,
	NID_commonName,
	NID_pkcs9_emailAddress
};

NewX509::NewX509(QWidget *parent)
	:QDialog(parent)
{
	int i;
	eku_nid = *MainWindow::eku_nid;
	dn_nid = *MainWindow::dn_nid;
	aia_nid = *MainWindow::aia_nid;
	attr_nid << NID_pkcs9_unstructuredName << NID_pkcs9_challengePassword;

	QStringList keys;

	setupUi(this);

	/* temporary storage for creating temporary X509V3_CTX */
	ctx_cert = NULL;

	for (i=0; i < dn_nid.count(); i++)
		keys << QString(OBJ_nid2ln(dn_nid[i]));

	extDNlist->setKeys(keys);
	extDNlist->setInfoLabel(extDNinfo);
	connect(extDNlist->itemDelegateForColumn(1),
		SIGNAL(setupLineEdit(const QString &, QLineEdit *)),
		this, SLOT(setupExtDNwidget(const QString &, QLineEdit *)));
	setWindowTitle(XCA_TITLE);

	for (i=0; i<tabWidget->count(); i++) {
		tabnames << tabWidget->tabText(i);
	}

	nsImg->setPixmap(*MainWindow::nsImg);
	serialNr->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]*"), this));
	QStringList strings;

	// are there any useable private keys  ?
	newKeyDone("");

	// any PKCS#10 requests to be used ?
	strings = MainWindow::reqs->getDesc();
	if (strings.isEmpty()) {
		fromReqCB->setDisabled(true);
		fromReqCB->setChecked(false);
	}
	else {
		reqList->insertItems(0, strings);
	}
	on_fromReqCB_clicked();

	// How about signing certificates ?
	strings = MainWindow::certs->getSignerDesc();
	if (strings.isEmpty()) {
		foreignSignRB->setDisabled(true);
	} else {
		certList->insertItems(0, strings);
	}
#ifdef WG_QA_SERIAL
	selfQASignRB = new QRadioButton(signerBox);
	setTabOrder(serialNr, selfQASignRB);
	setTabOrder(selfQASignRB, foreignSignRB);
	selfQASignRB->setText(tr(
			"Create a &self signed certificate with a MD5-hashed QA serial"));
	QBoxLayout *l = (QBoxLayout *)signerBox->layout();
	l->insertWidget(1, selfQASignRB);
#endif
	// set dates to now and now + 1 year
	validNumber->setText("1");
	validRange->setCurrentIndex(2);
	on_applyTime_clicked();

	// settings for the templates ....
	strings.clear();
	strings = MainWindow::temps->getDescPredefs();
	tempList->insertItems(0, strings);

	// setup Extended keyusage
	for (i=0; i < eku_nid.count(); i++)
		ekeyUsage->addItem(OBJ_nid2ln(eku_nid[i]));

	// setup Authority Info Access
	for (i=0; i < aia_nid.count(); i++)
		aiaOid->addItem(OBJ_nid2ln(aia_nid[i]));

	// init the X509 v3 context
	X509V3_set_ctx(&ext_ctx, NULL , NULL, NULL, NULL, 0);
	X509V3_set_ctx_nodb(&ext_ctx);

	QList<QLabel *> nameLabel;
	nameLabel << LcountryName << LstateOrProvinceName << LlocalityName <<
	LorganisationName << LorganisationalUnitName << LcommonName <<
	LemailAddress;

	for(int i=0; i<nameLabel.count(); i++) {
		nameLabel[i]->setText(OBJ_nid2ln(name_nid[i]));
		QString tt = nameLabel[i]->toolTip();
		nameLabel[i]->setToolTip(QString("[%1] %2").
			arg(OBJ_nid2sn(name_nid[i])).arg(tt));
		name_ptr[i] = (QLineEdit *)nameLabel[i]->buddy();
		setupLineEditByNid(name_nid[i], name_ptr[i]);
	}
	// Setup Request Attributes
	if (attrWidget->layout())
		delete attrWidget->layout();
	QGridLayout *attrLayout = new QGridLayout(attrWidget);
	attrLayout->setAlignment(Qt::AlignTop);
	attrLayout->setSpacing(6);
	attrLayout->setMargin(0);
	attr_edit.clear();
	for (i=0; i < attr_nid.count(); i++) {
		QLabel *label;
		QLineEdit *edit;
		int nid = attr_nid[i];
		label = new QLabel(this);
		label->setText(QString(OBJ_nid2ln(nid)));
		label->setToolTip(QString(OBJ_nid2sn(nid)));
		edit = new QLineEdit(this);
		attr_edit << edit;
		attrLayout->addWidget(label, i, 0);
		attrLayout->addWidget(edit, i, 1);
		setupLineEditByNid(nid, edit);
	}
	// last polish
	on_certList_currentIndexChanged(0);
	certList->setDisabled(true);
	tabWidget->setCurrentIndex(0);
	attrWidget->hide();
	pt = none;
	notAfter->setEndDate(true);
}

void NewX509::setRequest()
{
	reqWidget->hide();
	attrWidget->show();

	signerBox->setEnabled(false);
	timewidget->setEnabled(false);
	capt->setText(tr("Create Certificate signing request"));
	authKey->setEnabled(false);
	setImage(MainWindow::csrImg);
	pt = x509_req;
}

NewX509::~NewX509()
{
	if (ctx_cert)
		delete(ctx_cert);
}

void NewX509::setupExtDNwidget(const QString &s, QLineEdit *l)
{
	setupLineEditByNid(OBJ_txt2nid(CCHAR(s)), l);
}

void NewX509::setupLineEditByNid(int nid, QLineEdit *l)
{
	ASN1_STRING_TABLE *tab = ASN1_STRING_TABLE_get(nid);
	QValidator *validator = NULL;
	QStringList info;

	info << QString("[%1]").arg(OBJ_nid2sn(nid));

	if (tab) {
		if (tab->minsize > 1)
			info << tr("minimum size: %1").arg(tab->minsize);
		if (tab->maxsize != -1)
			info << tr("maximum size: %1").arg(tab->maxsize);
		if (tab->mask == B_ASN1_PRINTABLESTRING) {
			info << tr("only a-z A-Z 0-9 '()+,-./:=?");
			QRegExp rx("[a-zA-Z0-9'()+,-./:=?]+");
			validator = new QRegExpValidator(rx, this);
		} else if (tab->mask == B_ASN1_IA5STRING) {
			info << tr("only 7-bit clean characters");
		}
	}
	l->setToolTip(info.join(" "));
	l->setValidator(validator);
}

void NewX509::addReqAttributes(pki_x509req *req)
{
	for (int i=0; i < attr_nid.count(); i++) {
		req->addAttribute(attr_nid[i], attr_edit[i]->text());
	}
}

void NewX509::setTemp(pki_temp *temp)
{
	QString text = tr("Create");
	if (temp->getIntName() != "--") {
		description->setText(temp->getIntName());
		description->setDisabled(true);
		text = tr("Edit");
	}
	capt->setText(text + " " + tr("XCA template"));
	tabWidget->removeTab(0);
	privKeyBox->setEnabled(false);
	validityBox->setEnabled(false);
	setImage(MainWindow::tempImg);
	pt = tmpl;
}

void NewX509::setCert()
{
	capt->setText(tr("Create x509 Certificate"));
	setImage(MainWindow::certImg);
	pt = x509;
}

void NewX509::setImage(QPixmap *img)
{
	image->setPixmap(*img);
}

void NewX509::defineTemplate(pki_temp *temp)
{
	fromTemplate(temp);
	templateChanged(temp);
}

void NewX509::defineRequest(pki_x509req *req)
{
	if (!req)
		return;
	fromReqCB->setEnabled(true);
	fromReqCB->setChecked(true);
	QString reqname = req->getIntName();
	reqList->setCurrentIndex(reqList->findText(reqname));
	on_fromReqCB_clicked();
}

void NewX509::defineSigner(pki_x509 *defcert)
{
	int index;
	// suggested from: Andrey Brindeew <abr@abr.pp.ru>
	if (defcert && defcert->canSign() ) {
		QString name = defcert->getIntName();
		foreignSignRB->setChecked(true);
		certList->setEnabled(true);
		if ((index = certList->findText(name)) >= 0) {
			certList->setCurrentIndex(index);
		}
		if (!defcert->getTemplate().isEmpty()) {
			on_applyTemplate_clicked();
		}
	}
}


static int lb2int(QListWidget *lb)
{
	int i, x=0, c=lb->count();
	QListWidgetItem *item;

	for (i=0; i<c; i++) {
		item = lb->item(i);
		if (lb->isItemSelected(item)){
			x |= 1<<i;
		}
	}
	return x;
}

static void int2lb(QListWidget *lb, int x)
{
	int i, c=lb->count();
	QListWidgetItem *item;

	for (i=0; i<c; i++) {
		item = lb->item(i);
		lb->setItemSelected(item, (1<<i) & x);
	}
}

static void QString2lb(QListWidget *lb, QString x)
{
	QStringList li = x.split(", ");
	QList<QListWidgetItem *> items;

	for (int i=0; i<li.size(); i++) {
		QString lname = OBJ_sn2ln(CCHAR(li[i]));
		items = lb->findItems(lname, Qt::MatchExactly);
		if (items.size() > 0)
			lb->setItemSelected(items[0], 1);
	}
}

static QString lb2QString(QListWidget *lb)
{
	QStringList sl;

	for (int i=0; i<lb->count(); i++) {
		QListWidgetItem *item = lb->item(i);
		if (lb->isItemSelected(item)) {
			sl << QString(OBJ_ln2sn(CCHAR(item->text())));
		}
	}
	return sl.join(", ");
}

void NewX509::subjectFromTemplate(pki_temp *temp)
{
	if (temp)
		setX509name(temp->xname);
}

void NewX509::extensionsFromTemplate(pki_temp *temp)
{
	if (!temp)
		return;
	subAltName->setText(temp->subAltName);
	issAltName->setText(temp->issAltName);
	crlDist->setText(temp->crlDist);
	setAuthInfAcc_string(temp->authInfAcc);
	nsComment->setText(temp->nsComment);
	nsBaseUrl->setText(temp->nsBaseUrl);
	nsRevocationUrl->setText(temp->nsRevocationUrl);
	nsCARevocationUrl->setText(temp->nsCARevocationUrl);
	nsRenewalUrl->setText(temp->nsRenewalUrl);
	nsCaPolicyUrl->setText(temp->nsCaPolicyUrl);
	nsSslServerName->setText(temp->nsSslServerName);
	int2lb(nsCertType, temp->nsCertType);
	basicCA->setCurrentIndex(temp->ca);
	bcCritical->setChecked(temp->bcCrit);
	kuCritical->setChecked(temp->keyUseCrit);
	ekuCritical->setChecked(temp->eKeyUseCrit);
	subKey->setChecked(temp->subKey);
	authKey->setChecked(temp->authKey);
	int2lb(keyUsage, temp->keyUse);
	QString2lb(ekeyUsage, temp->eKeyUse);
	validNumber->setText(QString::number(temp->validN));
	validRange->setCurrentIndex(temp->validM);
	midnightCB->setChecked(temp->validMidn);
	basicPath->setText(temp->pathLen);
	nconf_data->document()->setPlainText(temp->adv_ext);
	noWellDefinedExpDate->setChecked(temp->noWellDefined);
	on_applyTime_clicked();
}

void NewX509::fromTemplate(pki_temp *temp)
{
	subjectFromTemplate(temp);
	extensionsFromTemplate(temp);
}

void NewX509::toTemplate(pki_temp *temp)
{
	temp->setIntName(description->text());
	temp->xname = getX509name();
	temp->subAltName = subAltName->text();
	temp->issAltName = issAltName->text();
	temp->crlDist = crlDist->text();
	temp->authInfAcc = getAuthInfAcc_string();
	temp->nsComment = nsComment->text();
	temp->nsBaseUrl = nsBaseUrl->text();
	temp->nsRevocationUrl = nsRevocationUrl->text();
	temp->nsCARevocationUrl = nsCARevocationUrl->text();
	temp->nsRenewalUrl = nsRenewalUrl->text();
	temp->nsCaPolicyUrl = nsCaPolicyUrl->text();
	temp->nsSslServerName = nsSslServerName->text();
	temp->nsCertType =  lb2int(nsCertType);
	temp->ca = basicCA->currentIndex();
	temp->bcCrit = bcCritical->isChecked();
	temp->keyUseCrit = kuCritical->isChecked();
	temp->eKeyUseCrit = ekuCritical->isChecked();
	temp->subKey = subKey->isChecked();
	temp->authKey = authKey->isChecked();
	temp->keyUse = lb2int(keyUsage);
	temp->eKeyUse = lb2QString(ekeyUsage);
	temp->validN = validNumber->text().toInt();
	temp->validM = validRange->currentIndex();
	temp->pathLen = QString::number(basicPath->text().toInt());
	temp->validMidn = midnightCB->isChecked();
	if (nconf_data->isReadOnly()) {
		temp->adv_ext = v3ext_backup;
	} else {
		temp->adv_ext = nconf_data->toPlainText();
	}
	temp->noWellDefined = noWellDefinedExpDate->isChecked();
}

void NewX509::on_fromReqCB_clicked()
{
	bool request = fromReqCB->isChecked();
	bool subj_tab_present = tabWidget->widget(1) == tab_1;
	bool subChange = reqSubChange->isChecked();

	if (request && subj_tab_present && !subChange)
		tabWidget->removeTab(1);
	else if ((!request || subChange) && !subj_tab_present)
		tabWidget->insertTab(1, tab_1, tr("Subject"));

	reqList->setEnabled(request);
	copyReqExtCB->setEnabled(request);
	showReqBut->setEnabled(request);
	reqSubChange->setEnabled(request);
	switchHashAlgo();
}

void NewX509::on_reqSubChange_clicked()
{
	if (reqSubChange->isChecked()) {
		pki_x509req *req = getSelectedReq();
		description->setText(req->getIntName());
		setX509name(req->getSubject());
		usedKeysToo->setEnabled(false);
		keyList->setEnabled(false);
		genKeyBut->setEnabled(false);
	}
	on_fromReqCB_clicked();
}

void NewX509::on_keyList_currentIndexChanged(const QString &)
{
	switchHashAlgo();
}

void NewX509::on_reqList_currentIndexChanged(const QString &)
{
	switchHashAlgo();
}

void NewX509::switchHashAlgo()
{
	pki_key *key;
	pki_x509super *sig;

	if (foreignSignRB->isChecked())
		sig = getSelectedSigner();
	else if (fromReqCB->isChecked())
		sig = getSelectedReq();
	else
		sig = NULL;

	key = sig ? sig->getRefKey() : getSelectedKey();

	if (key) {
		hashAlgo->setKeyType(key->getKeyType());
		hashAlgo->setupHashes(key->possibleHashNids());
	} else {
		hashAlgo->setKeyType(EVP_PKEY_RSA);
		hashAlgo->setupAllHashes();
	}
}

void NewX509::on_showReqBut_clicked()
{
	QString req = reqList->currentText();
	emit showReq(req);
}

void NewX509::on_genKeyBut_clicked()
{
	QString name = description->text();
	if (name.isEmpty())
		name = commonName->text();
	if (name.isEmpty())
		name = emailAddress->text();
	emit genKey(name);
}

void NewX509::on_certList_currentIndexChanged(int)
{
	a1time snb, sna;
	pki_x509 *cert = getSelectedSigner();

	switchHashAlgo();

	if (!cert)
		return;

	QString templ = cert->getTemplate();
	snb = cert->getNotBefore();
	sna = cert->getNotAfter();
	if (snb > notBefore->getDate())
		notBefore->setDate(snb);
	if (sna < notAfter->getDate())
		notAfter->setDate(sna);

	if (templ.isEmpty())
		return;

	templateChanged(templ);
}


void NewX509::templateChanged(QString tempname)
{
	int index;
	if (!tempList->isEnabled())
		return;
	if ((index = tempList->findText(tempname)) <0)
		return;

	tempList->setCurrentIndex(index);
}


void NewX509::templateChanged(pki_temp *templ)
{
	QString tempname = templ->getIntName();
	templateChanged(tempname);
}

pki_temp *NewX509::currentTemplate()
{
	if (!tempList->isEnabled())
		return NULL;
	QString name = tempList->currentText();
	if (name.isEmpty())
		return NULL;
	return (pki_temp *)MainWindow::temps->getByName(name);
}

void NewX509::on_applyTemplate_clicked()
{
	fromTemplate(currentTemplate());
}

void NewX509::on_applySubject_clicked()
{
	subjectFromTemplate(currentTemplate());
}

void NewX509::on_applyExtensions_clicked()
{
	extensionsFromTemplate(currentTemplate());
}

void NewX509::on_foreignSignRB_toggled(bool)
{
	switchHashAlgo();
}

void NewX509::newKeyDone(QString name)
{
	QStringList keys;
	private_keys = MainWindow::keys->get0KeyDesc(true);
	private_keys0 = MainWindow::keys->get0KeyDesc(false);
	keyList->clear();
	if (usedKeysToo->isChecked())
		keys = private_keys;
	else
		keys = private_keys0;

	keyList->insertItems(0, keys);
	if (name.isEmpty() && keys.count() >0)
		name = keys[0];
	keyList->setCurrentIndex(keys.indexOf(name));
}

void NewX509::on_usedKeysToo_toggled(bool)
{
	QString cur = keyList->currentText();
	QStringList keys;
	keyList->clear();
	if (usedKeysToo->isChecked())
		keys = private_keys;
	else
		keys = private_keys0;

	keyList->insertItems(0, keys);
	keyList->setCurrentIndex(keys.indexOf(cur));
}

pki_key *NewX509::getSelectedKey()
{
	QString name = pki_key::removeTypeFromIntName(keyList->currentText());
	return (pki_key *)MainWindow::keys->getByName(name);
}

pki_x509 *NewX509::getSelectedSigner()
{
	return (pki_x509 *)MainWindow::certs->getByName(certList->currentText());
}

pki_x509req *NewX509::getSelectedReq()
{
	return (pki_x509req *)MainWindow::reqs->getByName(reqList->currentText());
}

x509name NewX509::getX509name(int _throw)
{
	x509name x;
	int j, row, nid;

	try {
		for (j = 0; j<EXPLICIT_NAME_CNT; j++) {
			nid = name_nid[j];
			x.addEntryByNid(nid, name_ptr[j]->text());
		}
		row = extDNlist->rowCount();
		for (j=0; j<row; j++) {
			QStringList l = extDNlist->getRow(j);
			nid = OBJ_txt2nid(CCHAR(l[0]));
			x.addEntryByNid(nid, l[1]);
		}
	} catch (errorEx &err) {
		if (!err.isEmpty()) {
			if (_throw)
				throw err;
			else
				QMessageBox::warning(this, XCA_TITLE, err.getString());
		}
	}
	return x;
}

void NewX509::setX509name(const x509name &n)
{
	int i,j;

	extDNlist->deleteAllRows();
	for (j=0; j<EXPLICIT_NAME_CNT; j++) {
		name_ptr[j]->setText("");
	}
	for (i=0, j=0; i< n.entryCount(); i++) {
		int nid = n.nid(i);
		QStringList sl = n.entryList(i);
		for ( ; j<EXPLICIT_NAME_CNT; j++) {
			if (nid == name_nid[j] && name_ptr[j]->text().isEmpty()) {
				name_ptr[j]->setText(sl[2]);
				break;
			}
		}
		if (j == EXPLICIT_NAME_CNT) {
			extDNlist->addRow(sl.mid(1, 2));
		}
	}
}

void NewX509::on_applyTime_clicked()
{
	notAfter->setDiff(notBefore, validNumber->text().toInt(),
				     validRange->currentIndex());
}

void NewX509::setupTmpCtx()
{
	pki_x509 *signcert;
	pki_x509req *req = NULL;
	pki_key *key = NULL;
	a1int serial;
	QString errtxt;

	// initially create temporary ctx cert
	if (ctx_cert)
		delete(ctx_cert);
	ctx_cert = new pki_x509();
	if (fromReqCB->isChecked()) {
		req = getSelectedReq();
		ctx_cert->setSubject(req->getSubject());
		if (req)
			key = req->getRefKey();
	} else {
		ctx_cert->setSubject(getX509name());
		key = getSelectedKey();
	}
	if (key)
		ctx_cert->setPubKey(key);
	// Step 2 - select Signing
	if (foreignSignRB->isChecked()) {
		signcert = getSelectedSigner();
		serial = signcert->getCaSerial();
	} else {
		signcert = ctx_cert;
		ctx_cert->setIssuer(ctx_cert->getSubject());
		serial.setHex(serialNr->text());
	}
	ctx_cert->setSerial(serial);
	initCtx(ctx_cert, signcert, req);
}

void NewX509::editV3ext(QLineEdit *le, QString types, int n)
{
	v3ext *dlg;

	dlg = new v3ext(this);
	setupTmpCtx();
	dlg->addInfo(le, types.split(',' ), n, &ext_ctx);
	dlg->exec();
	delete(dlg);
}

void NewX509::on_adv_validate_clicked()
{
	if (!nconf_data->isReadOnly()) {
		QString errtxt;
		extList el;
		ign_openssl_error();
		QString result;
		setupTmpCtx();
		v3ext_backup = nconf_data->toPlainText();
		if (fromReqCB->isChecked() && copyReqExtCB->isChecked()) {
			el = getSelectedReq()->getV3ext();
		}
		if (el.size() > 0) {
			result = "<h2><center>";
			result += tr("From PKCS#10 request") +
				"</center></h2><p>\n";
			result += el.getHtml("<br>");
		}
		try {
			el = getGuiExt();
			el += getNetscapeExt();
			el.delInvalid();
		} catch (errorEx &err) {
			errtxt = err.getString();
			el.clear();
		}
		if (el.size() > 0) {
			if (!result.isEmpty())
				result += "\n<hr>\n";
			result += "<h2><center>";
			result += tr("Other Tabs") + "</center></h2><p>\n";
			result += el.getHtml("<br>");
		}
		try {
			el = getAdvanced();
		} catch (errorEx &err) {
			errtxt += err.getString();
			el.clear();
		}
		if (el.size() > 0) {
			if (!result.isEmpty())
				result += "\n<hr>\n";
			result += "<h2><center>";
			result += tr("Advanced Tab") + "</center></h2><p>\n";
			result += el.getHtml("<br>");
		}
		if (!errtxt.isEmpty()) {
			if (!result.isEmpty())
				result += "\n<hr>\n";
			result += "<h2><center>";
			result += tr("Errors") + "</center></h2><p>\n";
			result += errtxt;
		}
		nconf_data->document()->setHtml(result);
		nconf_data->setReadOnly(true);

		adv_validate->setText(tr("Edit"));
		valid_htmltext = result;
		checkExtDuplicates();
	} else {
		nconf_data->document()->setPlainText(v3ext_backup);
		nconf_data->setReadOnly(false);
		adv_validate->setText(tr("Validate"));
		valid_htmltext = "";
	}
	ign_openssl_error();
}

void NewX509::on_editSubAlt_clicked()
{
	QString s = "email,RID,URI,DNS,IP,otherName";
	editV3ext(subAltName, s, NID_subject_alt_name);
}

void NewX509::on_editIssAlt_clicked()
{
	QString s = "email,RID,URI,DNS,IP,otherName,issuer";
	editV3ext(issAltName, s, NID_issuer_alt_name);
}

void NewX509::on_editCrlDist_clicked()
{
	editV3ext(crlDist, "URI", NID_crl_distribution_points);
}

void NewX509::on_editAuthInfAcc_clicked()
{
	editV3ext(authInfAcc, "email,RID,URI,DNS,IP", NID_info_access);
}

void NewX509::on_tabWidget_currentChanged(int)
{
	/* reset advanced tab to editable text */
	if (nconf_data->isReadOnly())
		on_adv_validate_clicked();
}

QString NewX509::mandatoryDnRemain()
{
	QStringList dnl = MainWindow::mandatory_dn.split(",");
	x509name n;
	int i;

	if (fromReqCB->isChecked() && !reqSubChange->isChecked())
		n = getSelectedReq()->getSubject();
	else
		n = getX509name();

	for (i=0; i< n.entryCount(); i++) {
		int j = dnl.indexOf(QString(OBJ_nid2sn(n.nid(i))));
		if (j>=0)
			dnl.removeAt(j);
	}
	return dnl.join(",");
}

void NewX509::gotoTab(int tab)
{
	for (int i=0; i<tabWidget->count(); i++) {
		if (tabWidget->tabText(i) == tabnames[tab]) {
			tabWidget->setCurrentIndex(i);
			break;
		}
	}
}

void NewX509::accept()
{
	x509name xn;
	on_tabWidget_currentChanged(0);
	try {
		xn = getX509name(1);
	} catch (errorEx &err) {
		gotoTab(1);
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE, err.getString(),
			QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok);
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		if (msg.exec() == QMessageBox::Close) {
			reject();
		}
		return;
	}
	QString lenErr = xn.checkLength();
	if (!lenErr.isEmpty()) {
		gotoTab(1);
		lenErr = tr("The following length restrictions of RFC3280 are violated:") +
			"\n" + lenErr;
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
					lenErr, QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Edit subject"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		msg.addButton(QMessageBox::Apply)->setText(tr("Continue rollout"));
		switch (msg.exec())
		{
			case QMessageBox::Ok:
			case QMessageBox::Cancel:
				return;
			case QMessageBox::Close:
				reject();
				return;
			case QMessageBox::Apply:
				break;
		}
	}
	if (fromReqCB->isChecked() && !getSelectedReq()->verify()) {
		gotoTab(0);
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
			tr("The verification of the Certificate request failed.\nThe rollout should be aborted."),
			QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Continue anyway"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		if (msg.exec() == QMessageBox::Close) {
			reject();
		}
	}
	if (description->text().isEmpty() && !fromReqCB->isChecked()) {
		if (commonName->text().isEmpty()) {
			gotoTab(1);
			QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
				tr("The internal name and the common name are empty.\nPlease set at least the internal name."), QMessageBox::NoButton, this);
			msg.addButton(QMessageBox::Ok)->setText(tr("Edit name"));
			msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
			if (msg.exec() == QMessageBox::Close) {
				reject();
			}
			return;
		} else {
			description->setText(commonName->text());
		}
	}
	if (keyList->count() == 0 && keyList->isEnabled() &&
				!fromReqCB->isChecked())
	{
		gotoTab(1);
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
			tr("There is no Key selected for signing."),
			QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Select key"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		if (msg.exec() == QMessageBox::Close) {
			reject();
		}
		return;
	}
	QString unsetDN;
	if (pt != tmpl)
		unsetDN = mandatoryDnRemain();
	if (!unsetDN.isEmpty()) {
		gotoTab(1);
		QString text = tr("The following distinguished name entries are empty:\n%1\nthough you have declared them as mandatory in the options menu.").arg(unsetDN);
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
					text, QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Edit subject"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		msg.addButton(QMessageBox::Apply)->setText(tr("Continue rollout"));
		switch (msg.exec())
		{
			case QMessageBox::Ok:
			case QMessageBox::Cancel:
				return;
			case QMessageBox::Close:
				reject();
				return;
			case QMessageBox::Apply:
				break;
		}
	}
	pki_x509 *signer = NULL;
	if (!selfSignRB->isChecked())
		signer = getSelectedSigner();

	pki_key *signkey = getSelectedKey();
	if (signer)
		signkey = signer->getRefKey();

	if ((!signkey || signkey->isPubKey()) && pt != tmpl) {
		QString txt;
		gotoTab(signer ? 0 : 1);
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
			tr("The key you selected for signing is not a private one."),
			QMessageBox::NoButton, this);
		txt = signer ? tr("Select other signer"):tr("Select other key");
		msg.addButton(QMessageBox::Ok)->setText(txt);
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		if (msg.exec() == QMessageBox::Close) {
			reject();
		}
		return;
        }
	if (signer && notBefore->getDate() < signer->getNotBefore()) {
		gotoTab(2);
		QString text = tr("The certificate will be earlier valid than the signer. This is probably not what you want.");
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
					text, QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Edit dates"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		msg.addButton(QMessageBox::Apply)->setText(tr("Continue rollout"));
		msg.addButton(QMessageBox::Yes)->setText(tr("Adjust date and continue"));
		switch (msg.exec())
		{
			case QMessageBox::Ok:
			case QMessageBox::Cancel:
				return;
			case QMessageBox::Close:
				reject();
				return;
			case QMessageBox::Apply:
				break;
			case QMessageBox::Yes:
				notBefore->setDate(signer->getNotBefore());
		}
	}
	if (signer && notAfter->getDate() > signer->getNotAfter() &&
				!noWellDefinedExpDate->isChecked()) {
		gotoTab(2);
		QString text = tr("The certificate will be longer valid than the signer. This is probably not what you want.");
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
					text, QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Edit dates"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		msg.addButton(QMessageBox::Apply)->setText(tr("Continue rollout"));
		msg.addButton(QMessageBox::Yes)->setText(tr("Adjust date and continue"));
		switch (msg.exec())
		{
			case QMessageBox::Ok:
			case QMessageBox::Cancel:
				return;
			case QMessageBox::Close:
				reject();
				return;
			case QMessageBox::Apply:
				break;
			case QMessageBox::Yes:
				notAfter->setDate(signer->getNotAfter());
		}
	}
	if (notBefore->getDate() > notAfter->getDate()) {
		gotoTab(2);
		QString text = tr("The certificate will be out of date before it becomes valid. You most probably mixed up both dates.");
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
					text, QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Edit dates"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		msg.addButton(QMessageBox::Apply)->setText("Continue rollout");
		switch (msg.exec())
		{
			case QMessageBox::Ok:
			case QMessageBox::Cancel:
				return;
			case QMessageBox::Close:
				reject();
				return;
			case QMessageBox::Apply:
				break;
		}
	}
	on_adv_validate_clicked();
	if (checkExtDuplicates()) {
		gotoTab(5);
		QString text = tr("The certificate contains duplicated extensions. Check the validation on the advanced tab.");
		QMessageBox msg(QMessageBox::Warning, XCA_TITLE,
					text, QMessageBox::NoButton, this);
		msg.addButton(QMessageBox::Ok)->setText(tr("Edit extensions"));
		msg.addButton(QMessageBox::Close)->setText(tr("Abort rollout"));
		msg.addButton(QMessageBox::Apply)->setText(tr("Continue rollout"));
		switch (msg.exec())
		{
			case QMessageBox::Ok:
			case QMessageBox::Cancel:
				return;
			case QMessageBox::Close:
				reject();
				return;
			case QMessageBox::Apply:
				break;
		}
	}
	QDialog::accept();
}
