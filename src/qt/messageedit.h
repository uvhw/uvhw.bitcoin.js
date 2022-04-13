// Copyright (c) 2017 The Graviocoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MESSAGEEDIT_H
#define MESSAGEEDIT_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class QMenu;
class QPrinter;
class QDialog;
class QMenuBar;
class QToolBar;
class QWidget;
class QValidatedLineEdit;
class QToolButton;
QT_END_NAMESPACE

#include "messagerecord.h"
#include "walletmodel.h"
#include "platformstyle.h"

class MessageEdit : public QDialog
{
    Q_OBJECT

public:
    enum Action
    {
        New,
        Reply
    };

public:
    MessageEdit(const PlatformStyle *platformStyle, WalletModel *walletModel, MessageRecord *rec, MessageEdit::Action action, QWidget *parent = 0);
    ~MessageEdit() { if (rec) delete rec; }

    bool load(const QString &f);

public Q_SLOTS:
    void messageSend();
    void messageEncrypt();
    void messageRichtext();

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

protected:
    void closeEvent(QCloseEvent *e);
    QMenuBar *menuBar();
    QToolBar *toolBar();
    QToolBar *toolBarFont();
    void makeTitle();
    bool trySendMessage();

private Q_SLOTS:
    void filePrint();
    void filePrintPreview();
    void filePrintPdf();

    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void textStyle(int styleIndex);
    void textColor();
    void textAlign(QAction *a);

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

    void clipboardDataChanged();
    void printPreview(QPrinter *);

    bool maybeSave();
    void closeClicked();

    void addressBookButtonClicked();
    void pasteButtonClicked();
    void addressToTextChanged(const QString &address);
    void deleteButtonClicked();

private:
    void setupFileActions(const PlatformStyle *platformStyle);
    void setupEditActions(const PlatformStyle *platformStyle);
    void setupTextActions(const PlatformStyle *platformStyle);
    void setCurrentFileName(const QString &fileName);

    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);
    bool updateLabel(const QString &address);

    void processSendMessageReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

    void init();

    QAction *actionSend;
    QAction *actionEncrypt;
    QAction *actionRichtext;
    QAction *actionTextBold;
    QAction *actionTextUnderline;
    QAction *actionTextItalic;
    QAction *actionTextColor;
    QAction *actionAlignLeft;
    QAction *actionAlignCenter;
    QAction *actionAlignRight;
    QAction *actionAlignJustify;
    QAction *actionUndo;
    QAction *actionRedo;
#ifndef QT_NO_CLIPBOARD
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
#endif

    QValidatedLineEdit *addressTo;
    QValidatedLineEdit *subj;
    QToolButton *addressBookButton;
    QToolButton *pasteButton;
    QToolButton *deleteButton;

    QComboBox *comboStyle;
    QFontComboBox *comboFont;
    QComboBox *comboSize;

    QToolBar *tb;
    QString fileName;
    QTextEdit *textEdit;
    QMenuBar *menuBarWidget;
    QToolBar *toolBarWidget;
    QToolBar *toolBarFontWidget;

    PlatformStyle *platformStyle;
    WalletModel *walletModel;
    MessageRecord *rec;
    MessageEdit::Action action;
};

#endif // MESSAGEEDIT_H
