/*
    This file is part of Choqok, the KDE micro-blogging client

    Copyright (C) 2008-2011 Mehrdad Momeny <mehrdad.momeny@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License or (at your option) version 3 or any later version
    accepted by the membership of KDE e.V. (or its successor approved
    by the membership of KDE e.V.), which shall act as a proxy
    defined in Section 14 of version 3 of the license.


    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see http://www.gnu.org/licenses/

*/

#include "plurkaccount.h"
#include "plurkmicroblog.h"
#include <passwordmanager.h>
#include <KDebug>

#include "plurkapioauth.h"

class PlurkAccount::Private
{
public:
    QString userId;
    int count;
    QStringList friendsList;
    QStringList timelineNames;

    PlurkApiOAuth* plurkOAuth;
};

PlurkAccount::PlurkAccount(PlurkMicroBlog* parent, const QString &alias)
    : Account(parent, alias), d(new Private)
{
    kDebug();
    d->userId = configGroup()->readEntry("UserId", QString());
    d->count = configGroup()->readEntry("CountOfPosts", 20);
    d->friendsList = configGroup()->readEntry("Friends", QStringList());
    d->timelineNames = configGroup()->readEntry("Timelines", QStringList());

    PlurkApiOAuth::self()->setOAuthToken( configGroup()->readEntry("OAuthToken", QByteArray()));
    PlurkApiOAuth::self()->setOAuthConsumerKey( configGroup()->readEntry("OAuthConsumerKey", QByteArray()));
    PlurkApiOAuth::self()->setOAuthConsumerSecret( Choqok::PasswordManager::self()->readPassword(
                                            QString("%1_consumerSecret").arg(alias) ).toUtf8());
    PlurkApiOAuth::self()->setOAuthTokenSecret( Choqok::PasswordManager::self()->readPassword(
                                            QString("%1_tokenSecret").arg(alias) ).toUtf8());

    if( d->userId.isEmpty() ) {
        // NOTE this is an asynchronized method
        parent->getProfile( this );
    }

    if( d->timelineNames.isEmpty() ){
        QStringList list = parent->timelineNames();
        list.removeOne("Public");
        list.removeOne("Favorite");
        list.removeOne("ReTweets");
        d->timelineNames = list;
    }

    if( d->friendsList.isEmpty() ){
        parent->listFriendsUsername(this);
        //Result will set on PlurkMicroBlog!
    }
}

PlurkAccount::~PlurkAccount()
{
    delete d;
}

void PlurkAccount::writeConfig()
{
    configGroup()->writeEntry("UserId", d->userId);
    configGroup()->writeEntry("CountOfPosts", d->count);
    configGroup()->writeEntry("Friends", d->friendsList);
    configGroup()->writeEntry("Timelines", d->timelineNames);
    configGroup()->writeEntry("OAuthToken", PlurkApiOAuth::self()->oauthToken() );
    configGroup()->writeEntry("OAuthConsumerKey", PlurkApiOAuth::self()->oauthConsumerKey() );
    Choqok::PasswordManager::self()->writePassword( QString("%1_consumerSecret").arg(alias()),
                                                    QString::fromUtf8(PlurkApiOAuth::self()->oauthConsumerSecret()) );
    Choqok::PasswordManager::self()->writePassword( QString("%1_tokenSecret").arg(alias()),
                                                    QString::fromUtf8( PlurkApiOAuth::self()->oauthTokenSecret()) );
    Choqok::Account::writeConfig();
}

QString PlurkAccount::userId() const
{
    return d->userId;
}

void PlurkAccount::setUserId( const QString &id )
{
    d->userId = id;
}

int PlurkAccount::countOfPosts() const
{
    return d->count;
}

void PlurkAccount::setCountOfPosts(int count)
{
    d->count = count;
}

QStringList PlurkAccount::friendsList() const
{
    return d->friendsList;
}

void PlurkAccount::setFriendsList(const QStringList& list)
{
    d->friendsList = list;
    writeConfig();
}

QStringList PlurkAccount::timelineNames() const
{
    return d->timelineNames;
}

void PlurkAccount::setTimelineNames(const QStringList& list)
{
    d->timelineNames.clear();
    foreach(const QString &name, list){
        if(microblog()->timelineNames().contains(name))
            d->timelineNames<<name;
    }
}

#include "plurkaccount.moc"
