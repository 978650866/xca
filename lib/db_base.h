/* vi: set sw=4 ts=4:
 *
 * Copyright (C) 2001 - 2010 Christian Hohnstaedt.
 *
 * All rights reserved.
 */

#ifndef __DB_BASE_H
#define __DB_BASE_H

#include "db.h"
#include "base.h"
#include "load_obj.h"
#include <QtGui/QListView>
#include <QtGui/QPixmap>
#include <QtGui/QContextMenuEvent>
#include <QtCore/QStringList>
#include <QtCore/QAbstractItemModel>
#include "pki_base.h"
//#include "widgets/XcaTreeView.h"

#define FOR_ALL_pki(pki, pki_type) \
	for(pki_type *pki=(pki_type*)rootItem->iterate(); pki; pki=(pki_type*)pki->iterate())

class MainWindow;
class QContextMenuEvent;
class XcaTreeView;
class NewX509;

class db_base: public QAbstractItemModel
{
	Q_OBJECT

	protected:
		QString dbName;
		QModelIndex currentIdx;
		void _writePKI(pki_base *pki, bool overwrite);
		void _removePKI(pki_base *pki );
		void removeItem(QString k);
		QList<enum pki_type> pkitype;
		MainWindow *mainwin;
		QString class_name;
		dbheaderList allHeaders;
		int colResizing;
	public:
		pki_base *rootItem;
		db_base(QString db, MainWindow *mw);
		virtual ~db_base();
		virtual pki_base *newPKI(db_header_t *head = NULL);
		virtual void insertPKI(pki_base *pki);
		virtual void updatePKI(pki_base *pki);
		pki_base *getByName(QString desc);
		pki_base *getByReference(pki_base *refpki);
		pki_base *getByPtr(void *);
		virtual void loadContainer();
		QStringList getDesc();
		virtual pki_base* insert(pki_base *item);
		virtual void inToCont(pki_base *pki);
		virtual void remFromCont(QModelIndex &idx);

		QPixmap *loadImg(const char *name);
		void writeAll(void);
		void dump(QString dirname);
		virtual void showContextMenu(QContextMenuEvent * ,
				const QModelIndex &) {};

		QModelIndex index(int row, int column, const QModelIndex &parent)const;
		QModelIndex index(pki_base *pki)const;
		QModelIndex parent(const QModelIndex &index) const;
		int rowCount(const QModelIndex &parent) const;
		int columnCount(const QModelIndex &parent) const;
		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation,
				int role) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;
		bool setData(const QModelIndex &index, const QVariant &value, int role);
		void deleteSelectedItems(XcaTreeView* view);
		void showSelectedItems(XcaTreeView *view);
		void storeSelectedItems(XcaTreeView *view);
		void load_default(load_base &load);
		void insertChild(pki_base *parent, pki_base *child);
		void createSuccess(pki_base *pki);
		void showHeaderMenu(QContextMenuEvent *e, int sect);
		bool columnHidden(int col) const;
		bool isNumericCol(int col) const;
		void saveHeaderState();
		void contextMenu(QContextMenuEvent *e,
			QMenu *parent = NULL, int sect = -1);
		void initHeaderView(QHeaderView *hv);
		void setVisualIndex(int i, int visualIndex);
		bool fixedHeaderSize(int sect);
		void colResizeStart()
		{
			colResizing++;
		}
		void colResizeEnd()
		{
			colResizing--;
		}

	public slots:
		void deletePKI();
		void delete_ask();
		void edit();
		void columnResetDefaults();
		virtual void showItem();
		virtual void store(){};
		virtual void showPki(pki_base *) {};
		virtual void showItem(const QModelIndex &index);
		virtual void showItem(const QString keyname);
		void sectionResized(int i, int, int newSize);
		void sortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

	signals:
		void connNewX509(NewX509 *dlg);
		void resetHeader();
		void updateHeader();
		void columnsContentChanged();
		void editItem(const QModelIndex &);
};

#endif
