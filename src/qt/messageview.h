// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MessageVIEW_H
#define BITCOIN_QT_MessageVIEW_H

#include "guiutil.h"

#include <QWidget>
#include <QKeyEvent>

class PlatformStyle;
class MessageFilterProxy;
class WalletModel;

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QLineEdit;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QTableView;
class QTextEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

/** Widget showing the Message list for a wallet, including a filter row.
    Using the filter row, the user can view or export a subset of the Messages.
  */
class MessageView : public QWidget
{
    Q_OBJECT

public:
    explicit MessageView(const PlatformStyle *platformStyle, QWidget *parent = 0);

    void setModel(WalletModel *model);

    // Date ranges for filter
    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

    enum ColumnWidths {
        STATUS_COLUMN_WIDTH = 23,
        ENCRYPTED_COLUMN_WIDTH = 23,
        DATE_COLUMN_WIDTH = 100,
        FROMADDRESS_COLUMN_WIDTH = 300,
        SUBJ_COLUMN_WIDTH = 300,
        TOADDRESS_COLUMN_WIDTH = 300,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 120,
        MINIMUM_COLUMN_WIDTH = 23
    };

private:
    WalletModel *model;
    MessageFilterProxy *messageProxyModel;
    QTableView *messageView;

    QComboBox *dateWidget;
    QComboBox *typeWidget;
    QComboBox *watchOnlyWidget;
    QLineEdit *addressWidget;
    QLineEdit *amountWidget;

    QMenu *contextMenu;
    QSignalMapper *mapperThirdPartyTxUrls;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;
    QAction *abandonAction;
    QLabel *from;
    QLabel *sent;
    QLabel *subj;
    QTextEdit* messageEdit;
    QPushButton *replyButton;

    PlatformStyle *platformStyle;

    QWidget *createDateRangeWidget();

    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;

    virtual void resizeEvent(QResizeEvent* event);

    bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void showDetails();
    void copyAddress();
    void editLabel();
    void copyTxID();
    void copyTxHex();
    void copyTxPlainText();
    void openThirdPartyTxUrl(QString url);
    void abandonTx();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);

public Q_SLOTS:
    void chooseDate(int idx);
    void chooseType(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void exportClicked();
    void newMessage();
    void replyMessage();
    void focusMessage(const QModelIndex&);
    void messageSelected(const QModelIndex&, const QModelIndex&);
    void messageClicked(const QModelIndex&);

};

#endif // BITCOIN_QT_MessageVIEW_H
