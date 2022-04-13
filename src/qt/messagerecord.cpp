// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "consensus/consensus.h"
#include "main.h"
#include "timedata.h"
#include "wallet/wallet.h"
#include "guiutil.h"
#include "utilstrencodings.h"
#include "transactiontablemodel.h"
#include "messagetablemodel.h"

#include <stdint.h>

#include <boost/foreach.hpp>
#include <QDebug>

#include "messagerecord.h"

/* Return positive answer if Message should be shown in list.
 */
bool MessageRecord::showMessage(const CWalletTx &wtx)
{
    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;

    if (nNet > 0)
    {
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if (isMessage(txout.blob)) return true;
        }
    }
    else
    {
        for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
        {
            const CTxOut& txout = wtx.vout[nOut];
            if (isMessage(txout.blob)) return true;
        }
    }

    return false;
}

/*
 * is message?
 */
bool MessageRecord::isMessage(const CBlob& blob)
{
    return
            blob.length() &&
            (
                blob.find("\"class\":\"mail-msg\"", 0) != CBlob::npos || // new style
                blob.find("\"class\": \"mail-msg\"", 0) != CBlob::npos || // new style

                blob.find("\"type\":\"message\"", 0) != CBlob::npos ||  // old-style
                blob.find("\"type\": \"message\"", 0) != CBlob::npos
            );
}

/*
 * Unpack message
 */
bool MessageRecord::unpackBlob(const CBlob& blob)
{
    if (isMessage(blob))
    {
        UniValue lBlob;
        if (lBlob.read(blob))
        {
            if (lBlob["class"].isNull()) // old style
            {
                type = MessageRecord::RecvFromAddress;
                from = lBlob["from"].getValStr();
                message = lBlob["message"].getValStr().c_str();
                encrypted = false;
                plainText = true;
            }
            else // new style { "class": "mail-msg", "instance": {...} }
            {
                type = MessageRecord::RecvFromAddress;
                encrypted = lBlob["instance"]["encrypted"].getBool();
                plainText = lBlob["instance"]["plaintext"].getBool();;
                from = lBlob["instance"]["from"].getValStr();
                subj = lBlob["instance"]["subj"].getValStr().c_str();
                message = lBlob["instance"]["message"].getValStr().c_str();

                std::string lPubKey = lBlob["instance"]["pubkey"].getValStr();
                std::vector<unsigned char> lPubKeyRaw = DecodeBase64(lPubKey.c_str());
                fromPubKey.Set(lPubKeyRaw.begin(), lPubKeyRaw.end());
                haveFromPubKey = fromPubKey.IsValid();

                keyExchange = haveFromPubKey && !encrypted;

            }
            return true;
        }
    }
    return false;
}

/*
 * Decompose CWallet Message to model Message records.
 */
QList<MessageRecord> MessageRecord::decomposeMessage(const CWallet *wallet, const CWalletTx &wtx, const WalletModel *model)
{
    QList<MessageRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (nNet > 0)
    {
        //
        // Credit
        //
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            isminetype mine = wallet->IsMine(txout);
            if(mine)
            {
                MessageRecord sub(hash, nTime, const_cast<WalletModel*>(model));
                sub.idx = parts.size(); // sequence number

                if (sub.unpackBlob(txout.blob))
                {
                    CTxDestination address;
                    if (ExtractDestination(txout.scriptPubKey, address))
                    {
                        sub.to = CBitcoinAddress(address).ToString();
                    }
                    else
                    {
                        sub.to = mapValue["to"];
                    }

                    parts.append(sub);
                }
            }
        }
    }
    else
    {
        CAmount nTxFee = nDebit - wtx.GetValueOut();

        for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
        {
            const CTxOut& txout = wtx.vout[nOut];
            MessageRecord sub(hash, nTime, const_cast<WalletModel*>(model));
            sub.idx = parts.size();

            if (sub.unpackBlob(txout.blob))
            {
                sub.type = MessageRecord::SendToAddress;

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    sub.to = CBitcoinAddress(address).ToString();
                }
                else
                {
                    sub.to = mapValue["to"];
                }

                CAmount nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }

                parts.append(sub);
            }
        }
    }

    return parts;
}

void MessageRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine Message status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    BlockMap::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded Messages sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();

    if (!CheckFinalTx(wtx))
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = MessageStatus::OpenUntilBlock;
            status.open_for = wtx.nLockTime - chainActive.Height();
        }
        else
        {
            status.status = MessageStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = MessageStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = MessageStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = MessageStatus::Unconfirmed;
            if (wtx.isAbandoned())
                status.status = MessageStatus::Abandoned;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = MessageStatus::Confirming;
        }
        else
        {
            status.status = MessageStatus::Confirmed;
        }
    }

}

bool MessageRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height();
}

QString MessageRecord::getTxID() const
{
    return QString::fromStdString(hash.ToString());
}

int MessageRecord::getOutputIndex() const
{
    return idx;
}

MessageRecord* MessageRecord::clone()
{
    MessageRecord* lClone = new MessageRecord(model);

    lClone->hash = hash;
    lClone->time = time;
    lClone->type = type;
    lClone->from = from;
    lClone->to = to;
    lClone->message = message;
    lClone->subj = subj;
    lClone->encrypted = encrypted;
    lClone->plainText = plainText;
    lClone->idx = idx;
    lClone->fromPubKey = fromPubKey;
    lClone->haveFromPubKey = haveFromPubKey;

    return lClone;
}

#include <QTextCodec>
#include <QTextEdit>

/** Prepare reply message
 */
QString MessageRecord::ReplyMessage()
{
    QString lSource = DecodeMessage();

    if (plainText)
    {
        QString lReply =
                QString("\r\n") +
                QString("------ original ------\r\n") +
                QString("from: ") + QString(from.c_str()) + QString("\r\n") +
                QString("sent: ") + GUIUtil::dateTimeStr(time) + QString("\r\n") +
                QString("subj: ") + DecodeSubj() + QString("\r\n\r\n") +
                lSource;

        return lReply;
    }

    // richtext
    QString lReply =
            QString("<br>") +
            QString("<hr>") +
            QString("<b>from:</b> ") + QString(from.c_str()) + QString("<br>") +
            QString("<b>sent:</b> ") + GUIUtil::dateTimeStr(time) + QString("<br>") +
            QString("<b>subj:</b> ") + DecodeSubj() + QString("<br>") +
            lSource;

    return lReply;
}

/** Return decoded message
 */
QString MessageRecord::DecodeMessage()
{
    QString lMessage;

    if (encrypted)
    {
        if (model->isOwnAddress(from)) // sender
        {
            decrypt(from, to, message, lMessage);
        }
        else // recipient
        {
            decrypt(to, from, message, lMessage);
        }
    }
    else lMessage = message;

    return lMessage;
}

/** Return decoded subj
 */
QString MessageRecord::DecodeSubj()
{
    QString lSubj;

    if (encrypted)
    {
        if (model->isOwnAddress(from)) // sender
        {
            decrypt(from, to, subj, lSubj);
        }
        else // recipient
        {
            decrypt(to, from, subj, lSubj);
        }
    }
    else lSubj = subj;

    return lSubj;
}

/** serialize message
 */
typedef std::vector<unsigned char> valtype;
void MessageRecord::SerializeMessage(UniValue& blob)
{
    blob.push_back(Pair("class", "mail-msg"));

    UniValue lInstance(UniValue::VOBJ);
    lInstance.push_back(Pair("plaintext", plainText));
    lInstance.push_back(Pair("encrypted", encrypted));
    lInstance.push_back(Pair("from", from));

    if (encrypted)
    {
        // own private and public key
        CBitcoinAddress lFromAddress(from);
        CKeyID lFromID; lFromAddress.GetKeyID(lFromID);
        CKey lFromKey; model->getKey(lFromID, lFromKey);
        CPubKey lFromPubKey; model->getPubKey(lFromID, lFromPubKey);

        //qDebug() << "From = " << lFromAddress.ToString().c_str();
        //qDebug() << "FromPubKey = " << HexStr(lFromPubKey.begin(), lFromPubKey.end()).c_str();

        // TODO: check in messageedit with allowance to encrypt
        // other public key
        CBitcoinAddress lToAddress(to);
        CPubKey lToPubKey;
        if (!model->getMessageTableModel()->getPubKey(to, lToPubKey) && !model->getTransactionTableModel()->getPubKey(to, lToPubKey)) return;

        //qDebug() << "To = " << lToAddress.ToString().c_str();
        //qDebug() << "ToPubKey = " << HexStr(lToPubKey.begin(), lToPubKey.end()).c_str();

        // encrypt subj
        std::vector<unsigned char> lSubjCypher;
        std::string lSrcSubj = subj.toStdString();
        encrypt(lFromKey, lFromPubKey, lToPubKey, lSrcSubj, lSubjCypher);
        std::string lSubjEnc = EncodeBase64(&lSubjCypher[0], lSubjCypher.size());
        lInstance.push_back(Pair("subj", lSubjEnc));

        // encrypt message
        std::vector<unsigned char> lMessageCypher;
        std::string lSrcMessage = message.toStdString();
        encrypt(lFromKey, lFromPubKey, lToPubKey, lSrcMessage, lMessageCypher);
        std::string lMessageEnc = EncodeBase64(&lMessageCypher[0], lMessageCypher.size());
        lInstance.push_back(Pair("message", lMessageEnc));
    }
    else
    {
        lInstance.push_back(Pair("subj", subj.toStdString()));
        lInstance.push_back(Pair("message", message.toStdString()));
    }

    if (keyExchange)
    {
        // own private and public key
        CBitcoinAddress lFromAddress(from);
        CKeyID lFromID; lFromAddress.GetKeyID(lFromID);
        CKey lFromKey; model->getKey(lFromID, lFromKey);
        CPubKey lFromPubKey; model->getPubKey(lFromID, lFromPubKey);

        std::string lPubKey = EncodeBase64(lFromPubKey.begin(), lFromPubKey.size());
        lInstance.push_back(Pair("pubkey", lPubKey));
    }

    blob.push_back(Pair("instance", lInstance));
}

/** encrypt
 */
bool MessageRecord::encrypt(CKey& own, CPubKey& own_pub, CPubKey& other, std::string& src, std::vector<unsigned char>& cypher)
{
    std::vector<unsigned char> lResult;
    if (!own.GetECDHSecret(other, lResult))
    {
        qDebug() << "GetECDHSecret failed!";
        return false;
    }

    unsigned char chIV[16];
    unsigned char chKey[32];

    uint256 hash_a = own_pub.GetHash();
    memcpy(chIV, &hash_a, 16);
    memcpy(chKey, &lResult[0], 32);

    qDebug() << "encrypt: result = " << HexStr(lResult.begin(), lResult.end()).c_str();
    qDebug() << "encrypt: pubkey = " << HexStr(&chIV[0], ((unsigned char*)chIV)+16).c_str();

    AES256CBCEncrypt lEnc(chKey, chIV, true);

    std::vector<unsigned char> lSrc; lSrc.resize(src.length()+1, 0);
    memcpy(&lSrc[0], src.c_str(), src.length());
    cypher.resize(lSrc.size() + AES_BLOCKSIZE, 0);

    unsigned int lLen = lEnc.Encrypt(&lSrc[0], lSrc.size(), &cypher[0]);
    cypher.resize(lLen);

    qDebug() << "encrypt: src = " << src.c_str();
    qDebug() << "encrypt: cypher = " << HexStr(cypher.begin(), cypher.end()).c_str();
    qDebug() << "encrypt: len = " << lLen << lSrc.size();

    return lLen >= lSrc.size();
}

/** decrypt
 */
bool MessageRecord::decrypt(CKey& own, CPubKey&own_pub, CPubKey& other, std::vector<unsigned char>& cypher, std::string& dst)
{
    std::vector<unsigned char> lResult;
    own.GetECDHSecret(other, lResult);

    unsigned char chIV[16];
    unsigned char chKey[32];

    uint256 hash_a;

    if(model->isOwnAddress(from)) hash_a = own_pub.GetHash();
    else hash_a = other.GetHash();

    memcpy(chIV, &hash_a, 16);
    memcpy(chKey, &lResult[0], 32);

    qDebug() << "decrypt: result = " << HexStr(lResult.begin(), lResult.end()).c_str();
    qDebug() << "decrypt: pubkey = " << HexStr(&chIV[0], ((unsigned char*)chIV)+16).c_str();

    AES256CBCDecrypt lDec(chKey, chIV, true);

    std::vector<unsigned char> lDst; lDst.resize(cypher.size()+1, 0);
    int lLen = lDec.Decrypt(&cypher[0], cypher.size(), &lDst[0]);
    lDst.resize(lLen);

    qDebug() << "decrypt: cypher = " << HexStr(cypher.begin(), cypher.end()).c_str();
    qDebug() << "decrypt: len = " << lLen;

    if (lLen >= 0)
    {
        dst.insert(0, (const char*)&lDst[0], lDst.size());
        qDebug() << "decrypt: dst = " << dst.c_str();

        return true;
    }

    return false;
}

bool MessageRecord::decrypt(const std::string& from, const std::string& to, const QString& src, QString& dst)
{
    // own private and public key
    CBitcoinAddress lFromAddress(from);
    CKeyID lFromID; lFromAddress.GetKeyID(lFromID);
    CKey lFromKey; model->getKey(lFromID, lFromKey);
    CPubKey lFromPubKey; model->getPubKey(lFromID, lFromPubKey);

    // other public key
    CPubKey lToPubKey;
    if (!model->getMessageTableModel()->getPubKey(to, lToPubKey) && !model->getTransactionTableModel()->getPubKey(to, lToPubKey)) return false;

    // decrypt
    std::string lSrc = src.toStdString();
    std::vector<unsigned char> lCypher = DecodeBase64(lSrc.c_str());

    std::string lDst;
    if (!decrypt(lFromKey, lFromPubKey, lToPubKey, lCypher, lDst)) return false;

    QString lResult = QString::fromStdString(lDst);
    dst.push_back(lResult);

    return true;
}

bool MessageRecord::allowEncrypt(const std::string& address)
{
    CPubKey lToPubKey;
    return (model->getMessageTableModel()->getPubKey(address, lToPubKey) || model->getTransactionTableModel()->getPubKey(address, lToPubKey));
}
