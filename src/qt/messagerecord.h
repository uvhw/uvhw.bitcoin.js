// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MessageRECORD_H
#define BITCOIN_QT_MessageRECORD_H

#include "amount.h"
#include "uint256.h"
#include "../primitives/transaction.h"
#include "univalue/include/univalue.h"
#include "key.h"
#include "pubkey.h"
#include "crypto/aes.h"

#include <QList>
#include <QString>
#include <QTextCodec>

class CWallet;
class CWalletTx;
class CKey;
class CPubKey;

#include "walletmodel.h"

/** UI model for Message status. The Message status is the part of a Message that will change over time.
 */
class MessageStatus
{
public:
    MessageStatus():
        countsForBalance(false), sortKey(""),
        matures_in(0), status(Offline), depth(0), open_for(0), cur_num_blocks(-1)
    { }

    enum Status {
        Confirmed,          /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) Messages
        OpenUntilDate,      /**< Message not yet final, waiting for date */
        OpenUntilBlock,     /**< Message not yet final, waiting for block */
        Offline,            /**< Not sent to any other nodes **/
        Unconfirmed,        /**< Not yet mined into a block **/
        Confirming,         /**< Confirmed, but waiting for the recommended number of confirmations **/
        Conflicted,         /**< Conflicts with other Message or mempool **/
        Abandoned,          /**< Abandoned from the wallet **/
        /// Generated (mined) Messages
        Immature,           /**< Mined but waiting for maturity */
        MaturesWarning,     /**< Message will likely not mature because no nodes have confirmed */
        NotAccepted         /**< Mined but not accepted */
    };

    /// Message counts towards available balance
    bool countsForBalance;
    /// Sorting key based on status
    std::string sortKey;

    /** @name Generated (mined) Messages
       @{*/
    int matures_in;
    /**@}*/

    /** @name Reported status
       @{*/
    Status status;
    qint64 depth;
    qint64 open_for; /**< Timestamp if status==OpenUntilDate, otherwise number
                      of additional blocks that need to be mined before
                      finalization */
    /**@}*/

    /** Current number of blocks (to know whether cached status is still valid) */
    int cur_num_blocks;
};

/** UI model for a Message. A core Message can be represented by multiple UI Messages if it has
    multiple outputs.
 */
class MessageRecord
{
public:
    enum Type
    {
        Other,
        SendToAddress,
        RecvFromAddress,
    };

    /** Number of confirmation recommended for accepting a Message */
    static const int RecommendedNumConfirmations = 1;

    MessageRecord(WalletModel *walletModel):
            hash(), time(0), type(Other), from(""), to(""), encrypted(false), plainText(false), idx(0), model(walletModel)
    {
        haveFromPubKey = false;
        keyExchange = true;
    }

    MessageRecord(uint256 hash, qint64 time, WalletModel *walletModel):
            hash(hash), time(time), type(Other), from(""), to(""), encrypted(false), plainText(false), idx(0), model(walletModel)
    {
        haveFromPubKey = false;
        keyExchange = true;
    }

    /** Decompose CWallet Message to model Message records.
     */
    static bool showMessage(const CWalletTx &wtx);
    static QList<MessageRecord> decomposeMessage(const CWallet *wallet, const CWalletTx &wtx, const WalletModel *model);

    /** @name Immutable Message attributes
      @{*/
    uint256 hash;
    qint64 time;
    Type type;
    std::string from;
    std::string to;
    QString message;
    QString subj;
    bool encrypted;
    bool plainText;
    int idx; /** SubMessage index, for sort key */
    CPubKey fromPubKey;
    bool haveFromPubKey;
    bool keyExchange;
    WalletModel *model;
    /**@}*/

    /** make clone */
    MessageRecord* clone();

    /** Must be encrypted? */
    bool allowEncrypt(const std::string&);

    /** Unpack message */
    bool unpackBlob(const CBlob& blob);

    /** is message? */
    static bool isMessage(const CBlob& blob);

    /** Status: can change with block chain update */
    MessageStatus status;

    /** Whether the Message was sent/received with a watch-only address */
    bool involvesWatchAddress;

    /** Return the unique identifier for this Message (part) */
    QString getTxID() const;

    /** Return the output index of the subMessage  */
    int getOutputIndex() const;

    /** Update status from core wallet tx.
     */
    void updateStatus(const CWalletTx &wtx);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded();

    /** Return decoded subj
     */
    QString DecodeSubj();

    /** Return decoded message
     */
    QString DecodeMessage();

    /** Prepare reply message
     */
    QString ReplyMessage();

    /** serialize message
     */
    void SerializeMessage(UniValue&);

private:
    /** encrypt
     */
    bool encrypt(CKey& own, CPubKey& own_pub, CPubKey& other, std::string& src, std::vector<unsigned char>& cypher);

    /** decrypt
     */
    bool decrypt(CKey& own, CPubKey&own_pub, CPubKey& other, std::vector<unsigned char>& cypher, std::string& dst);
    bool decrypt(const std::string& from, const std::string& to, const QString& src, QString& dst);
};

#endif // BITCOIN_QT_MessageRECORD_H
