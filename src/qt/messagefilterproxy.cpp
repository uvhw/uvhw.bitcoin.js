// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "messagefilterproxy.h"

#include "messagetablemodel.h"
#include "messagerecord.h"

#include <cstdlib>

#include <QDateTime>

// Earliest date that can be represented (far in the past)
const QDateTime MessageFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime MessageFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

MessageFilterProxy::MessageFilterProxy(QObject *parent) :
    QSortFilterProxyModel(parent),
    dateFrom(MIN_DATE),
    dateTo(MAX_DATE),
    addrPrefix(),
    typeFilter(ALL_TYPES),
    minAmount(0),
    limitRows(-1),
    showInactive(true)
{
}

bool MessageFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int type = index.data(MessageTableModel::TypeRole).toInt();
    QDateTime datetime = index.data(MessageTableModel::DateRole).toDateTime();
    QString fromAddress = index.data(MessageTableModel::FromAddressRole).toString();
    QString toAddress = index.data(MessageTableModel::ToAddressRole).toString();
    int status = index.data(MessageTableModel::StatusRole).toInt();

    if(!showInactive && status == MessageStatus::Conflicted)
        return false;
    if(!(TYPE(type) & typeFilter))
        return false;
    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if(!fromAddress.contains(addrPrefix, Qt::CaseInsensitive) || !toAddress.contains(addrPrefix, Qt::CaseInsensitive))
        return false;

    return true;
}

void MessageFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
}

void MessageFilterProxy::setAddressPrefix(const QString &addrPrefix)
{
    this->addrPrefix = addrPrefix;
    invalidateFilter();
}

void MessageFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
}

void MessageFilterProxy::setMinAmount(const CAmount& minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
}

void MessageFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

void MessageFilterProxy::setShowInactive(bool showInactive)
{
    this->showInactive = showInactive;
    invalidateFilter();
}

int MessageFilterProxy::rowCount(const QModelIndex &parent) const
{
    if(limitRows != -1)
    {
        return std::min(QSortFilterProxyModel::rowCount(parent), limitRows);
    }
    else
    {
        return QSortFilterProxyModel::rowCount(parent);
    }
}
