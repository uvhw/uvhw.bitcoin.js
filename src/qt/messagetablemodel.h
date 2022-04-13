// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MessageTABLEMODEL_H
#define BITCOIN_QT_MessageTABLEMODEL_H

#include "bitcoinunits.h"
#include "pubkey.h"

#include <QAbstractTableModel>
#include <QStringList>

class PlatformStyle;
class MessageRecord;
class MessageTablePriv;
class WalletModel;
class CPubKey;
class CWallet;

/** UI model for the Message table of a wallet.
 */
class MessageTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MessageTableModel(const PlatformStyle *platformStyle, CWallet* wallet, WalletModel *parent = 0);
    ~MessageTableModel();

    enum ColumnIndex {
        Status = 0,
        Encrypted = 1,
        Date = 2,
        FromAddress = 3,
        Subj = 4,
        ToAddress = 5
    };

    /** Roles to get specific information from a Message row.
        These are independent of column.
    */
    enum RoleIndex {
        /** Type of Message */
        TypeRole = Qt::UserRole,
        /** Date and time this Message was created */
        DateRole,
        /** Long description (HTML format) */
        LongDescriptionRole,
        /** FromAddress of Message */
        FromAddressRole,
        /** ToAddress of Message */
        ToAddressRole,
        /** Unique identifier */
        TxIDRole,
        /** Message hash */
        TxHashRole,
        /** Message data, hex-encoded */
        TxHexRole,
        /** Whole Message as plain text */
        TxPlainTextRole,
        /** Is Message confirmed? */
        ConfirmedRole,
        /** Formatted amount, without brackets when unconfirmed */
        FormattedAmountRole,
        /** Message status (MessageRecord::Status) */
        StatusRole,
        /** Unprocessed icon */
        RawDecorationRole,
        /** Decoded message */
        DecodedMessageRole,
        /** Record */
        RecordRole,
        /** Encrypted message (MessageRecord::Encrypted) */
        EncryptedRole,
        /** Subj (MessageRecord::Subj) */
        SubjRole
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    bool processingQueuedMessages() { return fProcessingQueuedMessages; }
    void addPubKey(const std::string&, const CPubKey&);
    bool getPubKey(const std::string&, CPubKey&);

private:
    CWallet* wallet;
    WalletModel *walletModel;
    QStringList columns;
    MessageTablePriv *priv;
    bool fProcessingQueuedMessages;
    const PlatformStyle *platformStyle;
    std::map<std::string, CPubKey> pubKeyCache;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

public:
    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant addressColor(const MessageRecord *wtx) const;
    QString formatTxStatus(const MessageRecord *wtx) const;
    QString formatTxDate(const MessageRecord *wtx) const;
    QString formatTxType(const MessageRecord *wtx) const;
    QString formatFromAddress(const MessageRecord *wtx, bool tooltip) const;
    QString formatToAddress(const MessageRecord *wtx, bool tooltip) const;
    QString formatTooltip(const MessageRecord *rec) const;
    QVariant txStatusDecoration(const MessageRecord *wtx) const;
    QVariant txEncryptedDecoration(const MessageRecord *wtx) const;
    QVariant txWatchonlyDecoration(const MessageRecord *wtx) const;
    QVariant txAddressDecoration(const MessageRecord *wtx) const;

public Q_SLOTS:
    /* New Message, or Message changed status */
    void updateMessage(const QString &hash, int status, bool showMessage);
    /* Needed to update fProcessingQueuedMessages through a QueuedConnection */
    void setProcessingQueuedMessages(bool value) { fProcessingQueuedMessages = value; }

    friend class MessageTablePriv;
};

#endif // BITCOIN_QT_MessageTABLEMODEL_H
