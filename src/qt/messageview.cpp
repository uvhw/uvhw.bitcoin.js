// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "messageview.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "messagefilterproxy.h"
#include "messagerecord.h"
#include "messagetablemodel.h"
#include "walletmodel.h"

#include "ui_interface.h"

#include "messageedit.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSignalMapper>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

#include <QClipboard>
#include <QColorDialog>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printer)
#if QT_CONFIG(printdialog)
#include <QPrintDialog>
#endif
#include <QPrinter>
#if QT_CONFIG(printpreviewdialog)
#include <QPrintPreviewDialog>
#endif
#endif
#endif

MessageView::MessageView(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent), model(0), messageProxyModel(0),
    messageView(0), abandonAction(0), columnResizingFixer(0)
{
    this->platformStyle = const_cast<PlatformStyle *>(platformStyle);

    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);

    /*
    if (platformStyle->getUseExtraSpacing()) {
        hlayout->setSpacing(5);
        hlayout->addSpacing(26);
    } else {
        hlayout->setSpacing(3); //SPC: 0
        hlayout->addSpacing(30); //SPC: 23
    }
    */

    QPushButton *newButton = new QPushButton(tr("&New"), this);
    newButton->setToolTip(tr("Compose a new message"));
    if (platformStyle->getImagesOnButtons()) {
        newButton->setIcon(platformStyle->SingleColorIcon(":/icons/edit"));
    }
    hlayout->addWidget(newButton);

    dateWidget = new QComboBox(this);
    if (platformStyle->getUseExtraSpacing()) {
        dateWidget->setFixedWidth(121);
    } else {
        dateWidget->setFixedWidth(120);
    }
    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateWidget);

    typeWidget = new QComboBox(this);
    if (platformStyle->getUseExtraSpacing()) {
        typeWidget->setFixedWidth(121);
    } else {
        typeWidget->setFixedWidth(120);
    }

    typeWidget->addItem(tr("All"), MessageFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received from"), MessageFilterProxy::TYPE(MessageRecord::RecvFromAddress));
    typeWidget->addItem(tr("Sent to"), MessageFilterProxy::TYPE(MessageRecord::SendToAddress));

    hlayout->addWidget(typeWidget);

    addressWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    addressWidget->setPlaceholderText(tr("Enter address or label to search"));
#endif
    hlayout->addWidget(addressWidget);

    /*
    amountWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    amountWidget->setPlaceholderText(tr("Min amount"));
#endif
    if (platformStyle->getUseExtraSpacing()) {
        amountWidget->setFixedWidth(97);
    } else {
        amountWidget->setFixedWidth(100);
    }
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
    hlayout->addWidget(amountWidget);
    */

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateRangeWidget());
    vlayout->addWidget(view);
    vlayout->setSpacing(3); //SPC: 0
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
    if (platformStyle->getUseExtraSpacing()) {
        hlayout->addSpacing(width+2);
    } else {
        hlayout->addSpacing(width);
    }

    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    view->installEventFilter(this);

    messageView = view;

    // Message pane
    QFrame *messageFrame = new QFrame(this);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHeightForWidth(messageFrame->sizePolicy().hasHeightForWidth());
    messageFrame->setSizePolicy(sizePolicy);
    messageFrame->setMaximumSize(QSize(16777215, 16777215));
    vlayout->addWidget(messageFrame);
    messageFrame->installEventFilter(this);

    // Reply button
    replyButton = new QPushButton(tr("&Reply"), this);
    replyButton->setToolTip(tr("Reply to sender"));
    if (platformStyle->getImagesOnButtons()) {
        replyButton->setIcon(platformStyle->SingleColorIcon(":/icons/send"));
    }
    if (platformStyle->getUseExtraSpacing()) {
        replyButton->setFixedWidth(85);
    } else {
        replyButton->setFixedWidth(84);
    }

    replyButton->setFixedHeight(62);

    // H Layout
    QHBoxLayout *hMessageLayout = new QHBoxLayout();
    hMessageLayout->setContentsMargins(0,0,0,0);
    hMessageLayout->setAlignment(Qt::AlignLeft);

    /*
    if (platformStyle->getUseExtraSpacing()) {
        hMessageLayout->setSpacing(5);
        hMessageLayout->addSpacing(26);
    } else {
        hMessageLayout->setSpacing(3); //SPC: 0
        hMessageLayout->addSpacing(30); //SPC: 23
    }
    */

    hMessageLayout->addWidget(replyButton);

    // V Layout
    QVBoxLayout *vMessageLayout = new QVBoxLayout(messageFrame);
    vMessageLayout->setContentsMargins(0,0,0,0);
    vMessageLayout->addLayout(hMessageLayout);

    // Message view
    messageEdit = new QTextEdit(this);
    messageEdit->setReadOnly(true);
    messageEdit->setFontPointSize(10);
    vMessageLayout->addWidget(messageEdit);

    // Labels
    QLabel *lFrom = new QLabel(this);
    lFrom->setText("From: ");
    lFrom->setAlignment(Qt::AlignRight);

    QLabel *lSent = new QLabel(this);
    lSent->setText("Sent: ");
    lSent->setAlignment(Qt::AlignRight);

    QLabel *lSubj = new QLabel(this);
    lSubj->setText("Subj: ");
    lSubj->setAlignment(Qt::AlignRight);

    // Mail info
    from = new QLabel(this);
    from->setText("...");
    from->setAlignment(Qt::AlignLeft);

    sent = new QLabel(this);
    sent->setText("...");
    sent->setAlignment(Qt::AlignLeft);

    subj = new QLabel(this);
    subj->setText("...");
    subj->setAlignment(Qt::AlignLeft);

    // VH Layout
    QVBoxLayout *vLabelsLayout = new QVBoxLayout(messageFrame);
    vLabelsLayout->setContentsMargins(0,0,0,0);

    vLabelsLayout->addWidget(lFrom);
    vLabelsLayout->addWidget(lSent);
    vLabelsLayout->addWidget(lSubj);

    // VH Layout
    QVBoxLayout *vInfoLayout = new QVBoxLayout(messageFrame);
    vInfoLayout->setContentsMargins(0,0,0,0);

    vInfoLayout->addWidget(from);
    vInfoLayout->addWidget(sent);
    vInfoLayout->addWidget(subj);

    hMessageLayout->addLayout(vLabelsLayout);
    hMessageLayout->addLayout(vInfoLayout);

    // Actions
    abandonAction = new QAction(tr("Abandon Message"), this);
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy Message ID"), this);
    QAction *copyTxHexAction = new QAction(tr("Copy raw Message"), this);
    QAction *copyTxPlainText = new QAction(tr("Copy full Message details"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show Message details"), this);

    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(copyTxHexAction);
    contextMenu->addAction(copyTxPlainText);
    //contextMenu->addAction(showDetailsAction);
    contextMenu->addSeparator();
    contextMenu->addAction(abandonAction);
    contextMenu->addAction(editLabelAction);

    mapperThirdPartyTxUrls = new QSignalMapper(this);

    // Connect actions
    connect(mapperThirdPartyTxUrls, SIGNAL(mapped(QString)), this, SLOT(openThirdPartyTxUrl(QString)));

    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(newButton, SIGNAL(clicked()), this, SLOT(newMessage()));
    connect(replyButton, SIGNAL(clicked()), this, SLOT(replyMessage()));
    replyButton->setEnabled(false);

    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(view, SIGNAL(clicked(QModelIndex)), this, SLOT(messageClicked(QModelIndex)));

    connect(abandonAction, SIGNAL(triggered()), this, SLOT(abandonTx()));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(copyTxHexAction, SIGNAL(triggered()), this, SLOT(copyTxHex()));
    connect(copyTxPlainText, SIGNAL(triggered()), this, SLOT(copyTxPlainText()));
    connect(editLabelAction, SIGNAL(triggered()), this, SLOT(editLabel()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
}

void MessageView::setModel(WalletModel *model)
{
    this->model = model;
    if(model)
    {
        messageProxyModel = new MessageFilterProxy(this);
        messageProxyModel->setSourceModel(model->getMessageTableModel());
        messageProxyModel->setDynamicSortFilter(true);
        messageProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        messageProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        messageProxyModel->setSortRole(Qt::EditRole);

        messageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        messageView->setModel(messageProxyModel);
        messageView->setAlternatingRowColors(true);
        messageView->setSelectionBehavior(QAbstractItemView::SelectRows);
        messageView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        messageView->setSortingEnabled(true);
        messageView->sortByColumn(MessageTableModel::Date, Qt::DescendingOrder);
        messageView->verticalHeader()->hide();

        connect(messageView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), this, SLOT(messageSelected(QModelIndex, QModelIndex)));

        messageView->setColumnWidth(MessageTableModel::Status, STATUS_COLUMN_WIDTH);
        messageView->setColumnWidth(MessageTableModel::Encrypted, ENCRYPTED_COLUMN_WIDTH);
        messageView->setColumnWidth(MessageTableModel::Date, DATE_COLUMN_WIDTH);
        messageView->setColumnWidth(MessageTableModel::FromAddress, FROMADDRESS_COLUMN_WIDTH);
        messageView->setColumnWidth(MessageTableModel::Subj, SUBJ_COLUMN_WIDTH);
        messageView->setColumnWidth(MessageTableModel::ToAddress, TOADDRESS_COLUMN_WIDTH);

        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(messageView, TOADDRESS_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this);

        if (model->getOptionsModel())
        {
            // Add third party Message URLs to context menu
            QStringList listUrls = model->getOptionsModel()->getThirdPartyTxUrls().split("|", QString::SkipEmptyParts);
            for (int i = 0; i < listUrls.size(); ++i)
            {
                QString host = QUrl(listUrls[i].trimmed(), QUrl::StrictMode).host();
                if (!host.isEmpty())
                {
                    QAction *thirdPartyTxUrlAction = new QAction(host, this); // use host as menu item label
                    if (i == 0)
                        contextMenu->addSeparator();
                    contextMenu->addAction(thirdPartyTxUrlAction);
                    connect(thirdPartyTxUrlAction, SIGNAL(triggered()), mapperThirdPartyTxUrls, SLOT(map()));
                    mapperThirdPartyTxUrls->setMapping(thirdPartyTxUrlAction, listUrls[i].trimmed());
                }
            }
        }
    }
}

void MessageView::newMessage()
{
    MessageEdit *messageEdit = new MessageEdit(platformStyle, model, new MessageRecord(model), MessageEdit::Action::New, 0);
    connect(messageEdit, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

    messageEdit->show();
}

void MessageView::replyMessage()
{
    QModelIndexList selection = messageView->selectionModel()->selectedRows(0);
    if (selection.empty())
    {
        replyButton->setEnabled(false);
        return;
    }

    QVariant lVarRec = selection.at(0).data(MessageTableModel::RecordRole);
    MessageRecord *lRec = (MessageRecord *)lVarRec.value<void *>();

    MessageEdit *messageEdit = new MessageEdit(platformStyle, model, lRec->clone(), MessageEdit::Action::Reply, 0);
    connect(messageEdit, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

    messageEdit->show();
}

void MessageView::chooseDate(int idx)
{
    if(!messageProxyModel)
        return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        messageProxyModel->setDateRange(
                MessageFilterProxy::MIN_DATE,
                MessageFilterProxy::MAX_DATE);
        break;
    case Today:
        messageProxyModel->setDateRange(
                QDateTime(current),
                MessageFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        messageProxyModel->setDateRange(
                QDateTime(startOfWeek),
                MessageFilterProxy::MAX_DATE);

        } break;
    case ThisMonth:
        messageProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                MessageFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        messageProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        messageProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                MessageFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}

void MessageView::chooseType(int idx)
{
    if(!messageProxyModel)
        return;
    messageProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
}

void MessageView::changedPrefix(const QString &prefix)
{
    if(!messageProxyModel)
        return;
    messageProxyModel->setAddressPrefix(prefix);
}

void MessageView::changedAmount(const QString &amount)
{
    if(!messageProxyModel)
        return;
    CAmount amount_parsed = 0;
    if(BitcoinUnits::parse(model->getOptionsModel()->getDisplayUnit(), amount, &amount_parsed))
    {
        messageProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        messageProxyModel->setMinAmount(0);
    }
}

void MessageView::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Message History"), QString(),
        tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(messageProxyModel);
    writer.addColumn(tr("Status"), 0, MessageTableModel::StatusRole);
    writer.addColumn(tr("Encrypted"), 0, MessageTableModel::EncryptedRole);
    writer.addColumn(tr("Date"), 0, MessageTableModel::DateRole);
    writer.addColumn(tr("From"), 0, MessageTableModel::FromAddressRole);
    writer.addColumn(tr("Subj"), 0, MessageTableModel::SubjRole);
    writer.addColumn(tr("To"), 0, MessageTableModel::ToAddressRole);
    writer.addColumn(tr("ID"), 0, MessageTableModel::TxIDRole);

    if(!writer.write()) {
        Q_EMIT message(tr("Exporting Failed"), tr("There was an error trying to save the Message history to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Exporting Successful"), tr("The Message history was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void MessageView::messageClicked(const QModelIndex& index)
{
    QModelIndex lIndex;
    messageSelected(index, lIndex);
}

void MessageView::messageSelected(const QModelIndex&, const QModelIndex&)
{
    qDebug() << "MessageView::messageClicked";

    QModelIndexList selection = messageView->selectionModel()->selectedRows(0);
    if (selection.empty())
    {
        replyButton->setEnabled(false);
        return;
    }

    replyButton->setEnabled(true);

    QVariant lVarRec = selection.at(0).data(MessageTableModel::RecordRole);
    MessageRecord *lRec = (MessageRecord *)lVarRec.value<void *>();

    QString str = lRec->DecodeMessage();
    if (lRec->plainText && !Qt::mightBeRichText(str))
    {
        messageEdit->setPlainText(str);
    }
    else
    {
        messageEdit->setHtml(str);
    }

    from->setText(model->getMessageTableModel()->formatFromAddress(lRec, true));
    sent->setText(model->getMessageTableModel()->formatTxDate(lRec));
    subj->setText(lRec->DecodeSubj());
}

void MessageView::contextualMenu(const QPoint &point)
{
    QModelIndex index = messageView->indexAt(point);
    QModelIndexList selection = messageView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;

    // check if Message can be abandoned, disable context menu action in case it doesn't
    uint256 hash;
    hash.SetHex(selection.at(0).data(MessageTableModel::TxHashRole).toString().toStdString());
    abandonAction->setEnabled(model->transactionCanBeAbandoned(hash));

    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void MessageView::abandonTx()
{
    if(!messageView || !messageView->selectionModel())
        return;
    QModelIndexList selection = messageView->selectionModel()->selectedRows(0);

    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(MessageTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());

    // Abandon the wallet Message over the walletModel
    model->abandonTransaction(hash);

    // Update the table
    model->getMessageTableModel()->updateMessage(hashQStr, CT_UPDATED, false);
}

void MessageView::copyAddress()
{
    GUIUtil::copyEntryData(messageView, 0, MessageTableModel::FromAddressRole);
}

void MessageView::copyTxID()
{
    GUIUtil::copyEntryData(messageView, 0, MessageTableModel::TxIDRole);
}

void MessageView::copyTxHex()
{
    GUIUtil::copyEntryData(messageView, 0, MessageTableModel::TxHexRole);
}

void MessageView::copyTxPlainText()
{
    GUIUtil::copyEntryData(messageView, 0, MessageTableModel::TxPlainTextRole);
}

void MessageView::editLabel()
{
    if(!messageView->selectionModel() ||!model)
        return;
    QModelIndexList selection = messageView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(MessageTableModel::FromAddressRole).toString();
        if(address.isEmpty())
        {
            // If this Message has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a Message is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(AddressTableModel::TypeRole).toString();

            EditAddressDialog dlg(
                type == AddressTableModel::Receive
                ? EditAddressDialog::EditReceivingAddress
                : EditAddressDialog::EditSendingAddress, this);
            dlg.setModel(addressBook);
            dlg.loadRow(idx);
            dlg.exec();
        }
        else
        {
            // Add sending address
            EditAddressDialog dlg(EditAddressDialog::NewSendingAddress,
                this);
            dlg.setModel(addressBook);
            dlg.setAddress(address);
            dlg.exec();
        }
    }
}

void MessageView::showDetails()
{
    if(!messageView->selectionModel())
        return;
    QModelIndexList selection = messageView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        // TODO: inline widget needed
        //MessageDescDialog *dlg = new MessageDescDialog(selection.at(0));
        //dlg->setAttribute(Qt::WA_DeleteOnClose);
        //dlg->show();
    }
}

void MessageView::openThirdPartyTxUrl(QString url)
{
    if(!messageView || !messageView->selectionModel())
        return;
    QModelIndexList selection = messageView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(MessageTableModel::TxHashRole).toString())));
}

QWidget *MessageView::createDateRangeWidget()
{
    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new QDateTimeEdit(this);
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    connect(dateTo, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));

    return dateRangeWidget;
}

void MessageView::dateRangeChanged()
{
    if(!messageProxyModel)
        return;
    messageProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()).addDays(1));
}

void MessageView::focusMessage(const QModelIndex &idx)
{
    if(!messageProxyModel)
        return;
    QModelIndex targetIdx = messageProxyModel->mapFromSource(idx);
    messageView->scrollTo(targetIdx);
    messageView->setCurrentIndex(targetIdx);
    messageView->setFocus();
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void MessageView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(MessageTableModel::ToAddress);
}

// Need to override default Ctrl+C action for amount as default behaviour is just to copy DisplayRole text
bool MessageView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
             GUIUtil::copyEntryData(messageView, 0, MessageTableModel::TxPlainTextRole);
             return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

