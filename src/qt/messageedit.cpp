// Copyright (c) 2017 The Graviocoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
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
#include <QToolButton>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "qvalidatedlineedit.h"
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

#include "ui_interface.h"
#include "bitcoinunits.h"
#include "messageedit.h"
#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "univalue/include/univalue.h"
#include "main.h" // mempool and minRelayTxFee
#include "optionsmodel.h"

const QString rsrcPath = ":/icons";

MessageEdit::MessageEdit(const PlatformStyle *platformStyle, WalletModel *walletModel, MessageRecord *rec, MessageEdit::Action action, QWidget *parent)
    : QDialog(parent)
{
    this->walletModel = walletModel;
    this->rec = rec;
    this->action = action;
    this->platformStyle = const_cast<PlatformStyle *>(platformStyle);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);

    setMinimumSize(QSize(640, 480));

    textEdit = new QTextEdit(this);
    textEdit->setFontPointSize(10);

    connect(textEdit, &QTextEdit::currentCharFormatChanged, this, &MessageEdit::currentCharFormatChanged);
    connect(textEdit, &QTextEdit::cursorPositionChanged, this, &MessageEdit::cursorPositionChanged);

    menuBarWidget = new QMenuBar(this);
    hlayout->addWidget(menuBarWidget);

    toolBarWidget = new QToolBar(this);
    toolBarFontWidget = new QToolBar(this);

    QHBoxLayout *hToLayout = new QHBoxLayout();
    hToLayout->setContentsMargins(2,0,2,0);

    addressTo = new QValidatedLineEdit(this);
    GUIUtil::setupAddressWidget(addressTo, this);

    QHBoxLayout *hSubjLayout = new QHBoxLayout();
    hSubjLayout->setContentsMargins(2,0,2,0);

    subj = new QValidatedLineEdit(this);
    subj->setPlaceholderText(QObject::tr("Enter subject"));

    addressBookButton = new QToolButton(this);
    addressBookButton->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/address-book"));
    addressBookButton->setIconSize(QSize(18, 18));
    addressBookButton->setFixedHeight(27);

    pasteButton = new QToolButton(this);
    pasteButton->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/editpaste"));
    pasteButton->setIconSize(QSize(18, 18));
    pasteButton->setFixedHeight(27);

    deleteButton = new QToolButton(this);
    deleteButton->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/remove"));
    deleteButton->setIconSize(QSize(18, 18));
    deleteButton->setFixedHeight(27);

    hToLayout->addWidget(addressTo);
    hToLayout->setSpacing(0);
    hToLayout->addWidget(addressBookButton);
    hToLayout->addWidget(pasteButton);
    hToLayout->addWidget(deleteButton);

    hSubjLayout->addWidget(subj);

    vlayout->addLayout(hlayout);
    vlayout->addWidget(toolBarWidget);
    vlayout->addWidget(toolBarFontWidget);

    QVBoxLayout *vTextEditLayout = new QVBoxLayout(this);
    vTextEditLayout->setContentsMargins(2,0,2,2);
    vTextEditLayout->addWidget(textEdit);

    vlayout->addLayout(hToLayout);
    vlayout->addLayout(hSubjLayout);
    vlayout->addLayout(vTextEditLayout);

    QWidget::setTabOrder(subj, textEdit);

    //setCentralWidget(textEdit);
    //setToolButtonStyle(Qt::ToolButtonFollowStyle);

    setupFileActions(platformStyle);
    setupEditActions(platformStyle);
    setupTextActions(platformStyle);

    QFont textFont("Helvetica");
    textFont.setStyleHint(QFont::SansSerif);
    textFont.setPointSize(10);
    textEdit->setFont(textFont);
    fontChanged(textEdit->font());
    colorChanged(textEdit->textColor());
    alignmentChanged(textEdit->alignment());

    connect(textEdit->document(), &QTextDocument::modificationChanged, actionSend, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::modificationChanged, this, &QWidget::setWindowModified);
    connect(textEdit->document(), &QTextDocument::undoAvailable, actionUndo, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::redoAvailable, actionRedo, &QAction::setEnabled);

    setWindowModified(textEdit->document()->isModified());
    actionSend->setEnabled(textEdit->document()->isModified());
    actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(textEdit->document()->isRedoAvailable());

#ifndef QT_NO_CLIPBOARD
    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MessageEdit::clipboardDataChanged);
#endif

    textEdit->setFocus();

    connect(addressBookButton, SIGNAL(clicked()), this, SLOT(addressBookButtonClicked()));
    connect(pasteButton, SIGNAL(clicked()), this, SLOT(pasteButtonClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteButtonClicked()));
    connect(addressTo, SIGNAL(textChanged(const QString &)), this, SLOT(addressToTextChanged(const QString &)));

    init();
}

void MessageEdit::init()
{
    makeTitle();

    if (action == MessageEdit::Action::Reply && rec)
    {
        QString str = rec->ReplyMessage();
        if (Qt::mightBeRichText(str))
        {
            textEdit->setHtml(str);
        }
        else
        {
            textEdit->setPlainText(str);
        }

        // set "to"
        addressTo->setText(QString(rec->from.c_str()));

        QString lSubj = rec->DecodeSubj();
        if (lSubj.length()) subj->setText(QString("re: ") + rec->DecodeSubj());

        actionEncrypt->setChecked(rec->allowEncrypt(addressTo->text().toStdString()));
    }
    else if (action == MessageEdit::Action::New)
    {
        rec->plainText = false;
        rec->encrypted = false;
    }
}

void MessageEdit::addressBookButtonClicked()
{
    if(!walletModel)
        return;

    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(walletModel->getAddressTableModel());
    if(dlg.exec())
    {
        addressTo->setText(dlg.getReturnValue());
        makeTitle();
        textEdit->setFocus();
    }
}

void MessageEdit::pasteButtonClicked()
{
    addressTo->setText(QApplication::clipboard()->text());
}

void MessageEdit::deleteButtonClicked()
{
    addressTo->setText(tr(""));
}

void MessageEdit::addressToTextChanged(const QString &address)
{
    updateLabel(address);
    actionEncrypt->setChecked(rec->allowEncrypt(addressTo->text().toStdString()));
}

bool MessageEdit::updateLabel(const QString &address)
{
    if(!walletModel)
        return false;

    QString associatedLabel = walletModel->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        makeTitle();
        return true;
    }

    return false;
}

void MessageEdit::makeTitle()
{
    if (action == MessageEdit::Action::New)
    {
        if (!addressTo->text().size()) setWindowTitle("New message");
        else setWindowTitle(QString("Message to: ") + addressTo->text());
    }
    else // Reply
    {
        if (rec != 0)
        {
            setWindowTitle(QString("Reply to: ") + QString(rec->from.c_str()));
        }
    }
}

QMenuBar *MessageEdit::menuBar()
{
    return menuBarWidget;
}

QToolBar *MessageEdit::toolBar()
{
    return toolBarWidget;
}

QToolBar *MessageEdit::toolBarFont()
{
    return toolBarFontWidget;
}

void MessageEdit::closeEvent(QCloseEvent *e)
{
    if (maybeSave())
        e->accept();
    else
        e->ignore();
}

void MessageEdit::setupFileActions(const PlatformStyle *platformStyle)
{

    QToolBar *tb = toolBar();
    QMenu *menu = menuBar()->addMenu(tr("&Mail"));

    actionSend = new QAction(tr("&Send"), this); menu->addAction(actionSend);
    actionSend->setIcon(platformStyle->SingleColorIcon(":/icons/send"));
    actionSend->setShortcut(Qt::CTRL + Qt::Key_Return);
    tb->addAction(actionSend);
    actionSend->setPriority(QAction::LowPriority);
    connect(actionSend, SIGNAL(triggered()), this, SLOT(messageSend()));

    actionEncrypt = new QAction(tr("Encr&ypt"), this); menu->addAction(actionEncrypt);
    actionEncrypt->setIcon(platformStyle->SingleColorIcon(":/icons/key"));
    actionEncrypt->setShortcut(Qt::CTRL + Qt::Key_Y);
    tb->addAction(actionEncrypt);
    actionEncrypt->setPriority(QAction::LowPriority);
    connect(actionEncrypt, SIGNAL(triggered()), this, SLOT(messageEncrypt()));
    actionEncrypt->setCheckable(true);
    actionEncrypt->setChecked(rec->encrypted);

    actionRichtext = new QAction(tr("Ric&h text"), this); menu->addAction(actionRichtext);
    actionRichtext->setIcon(platformStyle->SingleColorIcon(":/icons/richtext"));
    actionRichtext->setShortcut(Qt::CTRL + Qt::Key_Y);
    tb->addAction(actionRichtext);
    actionRichtext->setPriority(QAction::LowPriority);
    connect(actionRichtext, SIGNAL(triggered()), this, SLOT(messageRichtext()));
    actionRichtext->setCheckable(true);
    actionRichtext->setChecked(!rec->plainText);

    menu->addSeparator();

    QAction *a;

#if defined(QT_PRINTSUPPORT_LIB)
#ifndef QT_NO_PRINTER
    a = new QAction(tr("&Print..."), this); menu->addAction(a);
    a->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/fileprint"));
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Print);
    tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));

    a = new QAction(tr("Print Preview..."), this); menu->addAction(a);
    a->setIcon(platformStyle->SingleColorIcon(platformStyle->SingleColorIcon(rsrcPath + "/fileprint"));
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));

    a = new QAction(tr("&Export PDF..."), this); menu->addAction(a);
    a->setIcon(platformStyle->SingleColorIcon(platformStyle->SingleColorIcon(rsrcPath + "/exportpdf"));
    a->setPriority(QAction::LowPriority);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));

    menu->addSeparator();
#endif
#endif

    a = new QAction(tr("&Exit"), this); menu->addAction(a);
    a->setIcon(platformStyle->SingleColorIcon(":/icons/quit"));
    a->setShortcut(QKeySequence::Quit);
    //tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(closeClicked()));

    tb->addSeparator();
}

void MessageEdit::setupEditActions(const PlatformStyle *platformStyle)
{
    QToolBar *tb = toolBar();
    QMenu *menu = menuBar()->addMenu(tr("&Edit"));

    actionUndo = new QAction(tr("&Undo"), textEdit); menu->addAction(actionUndo);
    actionUndo->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/editundo"));
    actionUndo->setShortcut(QKeySequence::Undo);
    tb->addAction(actionUndo);
    connect(actionUndo, SIGNAL(triggered()), textEdit, SLOT(undo()));

    actionRedo = new QAction(tr("&Redo"), textEdit); menu->addAction(actionRedo);
    actionRedo->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/editredo"));
    actionRedo->setShortcut(QKeySequence::Redo);
    tb->addAction(actionRedo);
    connect(actionRedo, SIGNAL(triggered()), textEdit, SLOT(redo()));

    menu->addSeparator();
    tb->addSeparator();

#ifndef QT_NO_CLIPBOARD
    actionCut = new QAction(tr("Cu&t"), textEdit); menu->addAction(actionCut);
    actionCut->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/editcut"));
    actionCut->setPriority(QAction::LowPriority);
    actionCut->setShortcut(QKeySequence::Cut);
    tb->addAction(actionCut);
    connect(actionCut, SIGNAL(triggered()), textEdit, SLOT(cut()));

    actionCopy = new QAction(tr("&Copy"), textEdit); menu->addAction(actionCopy);
    actionCopy->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/editcopy"));
    actionCopy->setPriority(QAction::LowPriority);
    actionCopy->setShortcut(QKeySequence::Copy);
    tb->addAction(actionCopy);
    connect(actionCopy, SIGNAL(triggered()), textEdit, SLOT(copy()));

    actionPaste = new QAction(tr("&Paste"), textEdit); menu->addAction(actionPaste);
    actionPaste->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/editpaste"));
    actionPaste->setPriority(QAction::LowPriority);
    actionPaste->setShortcut(QKeySequence::Paste);
    tb->addAction(actionPaste);
    connect(actionPaste, SIGNAL(triggered()), textEdit, SLOT(paste()));

    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif

    tb->addSeparator();
}

void MessageEdit::setupTextActions(const PlatformStyle *platformStyle)
{
    QToolBar *tb = toolBar();
    QMenu *menu = menuBar()->addMenu(tr("F&ormat"));

    actionTextBold = new QAction(tr("&Bold"), this); menu->addAction(actionTextBold);
    actionTextBold->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textbold"));
    connect(actionTextBold, SIGNAL(triggered()), this, SLOT(textBold()));
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    tb->addAction(actionTextBold);
    actionTextBold->setCheckable(true);

    actionTextItalic = new QAction(tr("&Italic"), this); menu->addAction(actionTextItalic);
    actionTextItalic->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textitalic"));
    connect(actionTextItalic, SIGNAL(triggered()), this, SLOT(textItalic()));
    actionTextItalic->setPriority(QAction::LowPriority);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    tb->addAction(actionTextItalic);
    actionTextItalic->setCheckable(true);

    actionTextUnderline = new QAction(tr("&Underline"), this); menu->addAction(actionTextUnderline);
    actionTextUnderline->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textunder"));
    connect(actionTextUnderline, SIGNAL(triggered()), this, SLOT(textUnderline()));
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    tb->addAction(actionTextUnderline);
    actionTextUnderline->setCheckable(true);

    menu->addSeparator();
    tb->addSeparator();

    actionAlignLeft = new QAction(tr("&Left"), this);
    actionAlignLeft->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textleft"));
    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    actionAlignLeft->setPriority(QAction::LowPriority);

    actionAlignCenter = new QAction(tr("C&enter"), this);
    actionAlignCenter->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textcenter"));
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    actionAlignCenter->setPriority(QAction::LowPriority);

    actionAlignRight = new QAction(tr("&Right"), this);
    actionAlignRight->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textright"));
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    actionAlignRight->setPriority(QAction::LowPriority);

    actionAlignJustify = new QAction(tr("&Justify"), this);
    actionAlignJustify->setIcon(platformStyle->SingleColorIcon(rsrcPath + "/textjustify"));
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    actionAlignJustify->setPriority(QAction::LowPriority);

    // Make sure the alignLeft  is always left of the alignRight
    QActionGroup *alignGroup = new QActionGroup(this);
    connect(alignGroup, &QActionGroup::triggered, this, &MessageEdit::textAlign);

    if (QApplication::isLeftToRight()) {
        alignGroup->addAction(actionAlignLeft);
        alignGroup->addAction(actionAlignCenter);
        alignGroup->addAction(actionAlignRight);
    } else {
        alignGroup->addAction(actionAlignRight);
        alignGroup->addAction(actionAlignCenter);
        alignGroup->addAction(actionAlignLeft);
    }
    alignGroup->addAction(actionAlignJustify);

    tb->addActions(alignGroup->actions());
    menu->addActions(alignGroup->actions());

    menu->addSeparator();
    tb->addSeparator();

    QPixmap pix(16, 16); pix.fill(Qt::black);
    actionTextColor = new QAction(tr("&Color..."), this); menu->addAction(actionTextColor);
    actionTextColor->setIcon(pix);
    tb->addAction(actionTextColor);
    connect(actionTextColor, SIGNAL(triggered()), this, SLOT(textColor()));

    tb = toolBarFont();

    comboStyle = new QComboBox(tb); tb->addWidget(comboStyle);
    comboStyle->addItem("Standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    comboStyle->addItem("Ordered List (Roman lower)");
    comboStyle->addItem("Ordered List (Roman upper)");
    comboStyle->setStyleSheet("QComboBox { padding-left: 3px; padding-top: 2px; padding-bottom: 2px; padding-right: 1px; }");

    connect(comboStyle, SIGNAL(activated(int)), this, SLOT(textStyle(int)));

    comboFont = new QFontComboBox(tb); tb->addWidget(comboFont);
    connect(comboFont, SIGNAL(activated(const QString&)), this, SLOT(textFamily(const QString&)));
    comboFont->setStyleSheet("QComboBox { padding-left: 3px; padding-top: 2px; padding-bottom: 2px; padding-right: 3px; }");

    comboSize = new QComboBox(tb);
    comboSize->setObjectName("comboSize");
    tb->addWidget(comboSize);
    comboSize->setEditable(true);

    const QList<int> standardSizes = QFontDatabase::standardSizes();
    for(int size = 0; size < standardSizes.size(); size++)
        comboSize->addItem(QString::number(standardSizes.at(size)));

    comboSize->setCurrentIndex(standardSizes.indexOf(QApplication::font().pointSize()));
    comboSize->setStyleSheet("QComboBox { width: 40px; padding-left: 3px; padding-top: 2px; padding-bottom: 2px; padding-right: 3px; }");

    connect(comboSize, SIGNAL(activated(const QString&)), this, SLOT(textSize(const QString&)));
}

bool MessageEdit::load(const QString &f)
{
    return true;
}

void MessageEdit::closeClicked()
{
   if (maybeSave())
   {
       this->close();
   }
}

bool MessageEdit::maybeSave()
{
    if (!textEdit->document()->isModified())
        return true;

    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("Your message has been modified.\n"
                                "Do you want save and send to recipient?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
    {
        return trySendMessage();
    }
    else if (ret == QMessageBox::Cancel)
    {
        return false;
    }

    return true;
}

void MessageEdit::processSendMessageReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid. Please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AbsurdFee:
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), maxTxFee));
        break;
    case WalletModel::PaymentRequestExpired:
        msgParams.first = tr("Payment request expired.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    Q_EMIT message(tr("Send message"), msgParams.first, msgParams.second);
}

void MessageEdit::messageSend()
{
    if (trySendMessage()) close();
}

bool MessageEdit::trySendMessage()
{
    // update rec
    rec->subj = subj->text();
    if (!rec->plainText)
        rec->message = textEdit->toHtml();
    else
        rec->message = textEdit->toPlainText();

    // need to be encrypted
    rec->encrypted = rec->allowEncrypt(addressTo->text().toStdString());
    // locate our address
    rec->from = walletModel->getFirstOwnAddress();
    // set recipient
    rec->to = addressTo->text().toStdString();
    // set subj
    rec->subj = subj->text();

    // serialize message
    UniValue lBlob(UniValue::VOBJ);
    rec->SerializeMessage(lBlob);

    // send to recipient
    QList<SendCoinsRecipient> lRecipients;
    SendCoinsRecipient lRecipient;
    lRecipient.address = QString(rec->to.c_str());
    lRecipient.calcAndAddFee = true;

    std::string lRawBlob = lBlob.write();
    lRecipient.amount = ((lRawBlob.length() / 1024) + 2) * DEFAULT_MIN_RELAY_TX_FEE * 3;

    //qDebug() << "lRawBlob = " << lRawBlob.c_str();

    lRecipient.blob = QString(lRawBlob.c_str());
    lRecipients.append(lRecipient);

    //
    WalletModelTransaction lTransaction(lRecipients);
    WalletModel::SendCoinsReturn lPrepareStatus = walletModel->prepareTransaction(lTransaction);

    // process prepareStatus and on error generate message shown to user
    processSendMessageReturn(lPrepareStatus,
        BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), lTransaction.getTransactionFee()));

    if(lPrepareStatus.status != WalletModel::OK)
    {
        qDebug() << "ERROR: PrepareStatus = " << lPrepareStatus.status;
        return false;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn lSendStatus = walletModel->sendCoins(lTransaction);
    // process sendStatus and on error generate message shown to user
    processSendMessageReturn(lSendStatus);

    if(lSendStatus.status != WalletModel::OK)
    {
        qDebug() << "ERROR: SendStatus = " << lSendStatus.status;
        return false;
    }

    qDebug() << "SUCCESS: Message was sent";

    textEdit->document()->setModified(false);

    return true;
}

void MessageEdit::messageEncrypt()
{
    if (rec->allowEncrypt(addressTo->text().toStdString()))
    {
        if (actionEncrypt->isChecked()) rec->encrypted = true;
        else actionEncrypt->setChecked(true);
    }
    else actionEncrypt->setChecked(false);
}

void MessageEdit::messageRichtext()
{
    rec->plainText = !actionRichtext->isChecked();
}

void MessageEdit::setCurrentFileName(const QString &fileName)
{
}

void MessageEdit::filePrint()
{
#if defined(QT_PRINTSUPPORT_LIB)
#if QT_CONFIG(printdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (textEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        textEdit->print(&printer);
    delete dlg;
#endif
#endif
}

void MessageEdit::filePrintPreview()
{
#if defined(QT_PRINTSUPPORT_LIB)
#if QT_CONFIG(printpreviewdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &MessageEdit::printPreview);
    preview.exec();
#endif
#endif
}

void MessageEdit::printPreview(QPrinter *printer)
{
#if defined(QT_PRINTSUPPORT_LIB)
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    textEdit->print(printer);
#endif
#endif
}


void MessageEdit::filePrintPdf()
{
#if defined(QT_PRINTSUPPORT_LIB)
#ifndef QT_NO_PRINTER
//! [0]
    QFileDialog fileDialog(this, tr("Export PDF"));
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setMimeTypeFilters(QStringList("application/pdf"));
    fileDialog.setDefaultSuffix("pdf");
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    QString fileName = fileDialog.selectedFiles().first();
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    textEdit->document()->print(&printer);
//! [0]
#endif
#endif
}

void MessageEdit::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void MessageEdit::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MessageEdit::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MessageEdit::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void MessageEdit::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MessageEdit::textStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
            case 7:
                style = QTextListFormat::ListLowerRoman;
                break;
            case 8:
                style = QTextListFormat::ListUpperRoman;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    } else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void MessageEdit::textColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void MessageEdit::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == actionAlignCenter)
        textEdit->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == actionAlignJustify)
        textEdit->setAlignment(Qt::AlignJustify);
}

void MessageEdit::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void MessageEdit::cursorPositionChanged()
{
    alignmentChanged(textEdit->alignment());
}

void MessageEdit::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}

void MessageEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}

void MessageEdit::fontChanged(const QFont &f)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}

void MessageEdit::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    actionTextColor->setIcon(pix);
}

void MessageEdit::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        actionAlignLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        actionAlignCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        actionAlignRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        actionAlignJustify->setChecked(true);
}

