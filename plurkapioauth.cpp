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

#include <QtOAuth/QtOAuth>
#include <QtOAuth/interface.h>
#include <QtOAuth/qoauth_namespace.h>

#include <KToolInvocation>
#include <KDebug>
#include <KMessageBox>
#include <KInputDialog>
#include <kio/accessmanager.h>
#include <choqoktools.h>

#include "plurkapioauth.h"

const char* plurkOAuthRequestTokenURL = "http://www.plurk.com/OAuth/request_token";
const char* plurkOAuthAuthorizeURL = "http://www.plurk.com/m/authorize";
const char* plurkOAuthAccessToken = "http://www.plurk.com/OAuth/access_token";
const char* plurkConsumerKey = "4TH0YShIOi3g";
const char* plurkConsumerSecret = "aAdoTJzJQAGcJauJ3dHLYV7LkSEo7tUS";

class PlurkApiOAuth::Private {
public:
    Private();

    QByteArray oauthToken;
    QByteArray oauthTokenSecret;
    QByteArray oauthConsumerKey;
    QByteArray oauthConsumerSecret;
    QSharedPointer< QOAuth::Interface > qoauth;
};

PlurkApiOAuth::Private::Private():
    oauthToken(),
    oauthTokenSecret(),
    oauthConsumerKey(),
    oauthConsumerSecret(),
    qoauth( new QOAuth::Interface )
{
    kDebug();
    qoauth->setConsumerKey( plurkConsumerKey);
    qoauth->setConsumerSecret( plurkConsumerSecret);
    qoauth->setRequestTimeout(20000);
    qoauth->setIgnoreSslErrors(true);
}
 
PlurkApiOAuth* PlurkApiOAuth::mSelf = 0L;

PlurkApiOAuth::~PlurkApiOAuth()
{
}

PlurkApiOAuth* PlurkApiOAuth::self()
{
    if ( !mSelf )
        mSelf = new PlurkApiOAuth;
    return mSelf;
}

PlurkApiOAuth::PlurkApiOAuth():
    d( new Private )
{
}

const QByteArray & PlurkApiOAuth::oauthToken() const
{
    return d->oauthToken;
}

void PlurkApiOAuth::setOAuthToken(const QByteArray& token)
{
    d->oauthToken = token;
}

const QByteArray & PlurkApiOAuth::oauthTokenSecret() const
{
    return d->oauthTokenSecret;
}

void PlurkApiOAuth::setOAuthTokenSecret(const QByteArray& tokenSecret)
{
    d->oauthTokenSecret = tokenSecret;
}

const QByteArray & PlurkApiOAuth::oauthConsumerKey() const
{
    return d->oauthConsumerKey;
}

void PlurkApiOAuth::setOAuthConsumerKey(const QByteArray& consumerKey)
{
    d->oauthConsumerKey = consumerKey;
}

const QByteArray & PlurkApiOAuth::oauthConsumerSecret() const
{
    return d->oauthConsumerSecret;
}

void PlurkApiOAuth::setOAuthConsumerSecret(const QByteArray& consumerSecret)
{
    d->oauthConsumerSecret = consumerSecret;
}

bool PlurkApiOAuth::authorizeUser()
{
    kDebug();
    // set the consumer key and secret
    d->qoauth->setConsumerKey( plurkConsumerKey );
    d->qoauth->setConsumerSecret( plurkConsumerSecret );
    // set a timeout for requests (in msecs)
    d->qoauth->setRequestTimeout( 20000 );
    d->qoauth->setIgnoreSslErrors(true);

    QOAuth::ParamMap otherArgs;

    // send a request for an unauthorized token
    QOAuth::ParamMap reply =
        d->qoauth->requestToken( plurkOAuthRequestTokenURL, QOAuth::GET, QOAuth::HMAC_SHA1 );

    // if no error occurred, read the received token and token secret
    if ( d->qoauth->error() == QOAuth::NoError ) {
        d->oauthToken = reply.value( QOAuth::tokenParameterName() );
        d->oauthTokenSecret = reply.value( QOAuth::tokenSecretParameterName() );
        kDebug() << "token: " << d->oauthToken;
        QUrl url(plurkOAuthAuthorizeURL);
        url.addQueryItem("oauth_token", d->oauthToken);
        url.addQueryItem( "oauth_callback", "oob" );
        KToolInvocation::invokeBrowser(url.toString());
        return getPinCode();
    } else {
        kDebug() << "ERROR: " << d->qoauth->error() << ' ' << Choqok::qoauthErrorText(d->qoauth->error());
        // TODO use a parent widget for this message box
        KMessageBox::detailedError(0, "Authorization Error",
                                   Choqok::qoauthErrorText(d->qoauth->error()));
    }

    return false;
}

bool PlurkApiOAuth::getPinCode()
{
    QString verifier = KInputDialog::getText( "Security code",
                                              "Security code recieved from Plurk",
                                                    "Enter security code:");
    if(verifier.isEmpty())
        return false;
    QOAuth::ParamMap otherArgs;
    otherArgs.insert( "oauth_verifier", verifier.toUtf8() );

    // send a request to exchange Request Token for an Access Token
    QOAuth::ParamMap reply =
    d->qoauth->accessToken( QString(plurkOAuthAccessToken),
                         QOAuth::GET, d->oauthToken, d->oauthTokenSecret, QOAuth::HMAC_SHA1, otherArgs );
    // if no error occurred, read the Access Token (and other arguments, if applicable)
    if ( d->qoauth->error() == QOAuth::NoError ) {
        d->oauthToken = reply.value( QOAuth::tokenParameterName() );
        d->oauthTokenSecret = reply.value( QOAuth::tokenSecretParameterName() );
        // TODO use a parent widget for this message box
        KMessageBox::information(0, "Choqok is authorized successfully.",
                                 "Authorized");
        return true;
    } else {
        kDebug() << "ERROR: " << d->qoauth->error() << ' ' << Choqok::qoauthErrorText(d->qoauth->error());
        KMessageBox::detailedError(0, "Authorization Error",
                                Choqok::qoauthErrorText(d->qoauth->error()));
    }

    return false;
}

QByteArray PlurkApiOAuth::makeHeader( const QString & url, const QMap< QString, QString > & params ) const {
    QOAuth::ParamMap oaParams;
    for( QMap< QString, QString >::const_iterator it = params.begin(); it != params.end(); ++it ) {
        QByteArray key( QUrl::toPercentEncoding( it.key() ) ), value( QUrl::toPercentEncoding( it.value() ) );
        oaParams.insert( key, value );
    }
    // NOTE Always use POST and SHA1 here. This seems always work with Plurk.
    QByteArray header( d->qoauth->createParametersString( url, QOAuth::POST, d->oauthToken, d->oauthTokenSecret, QOAuth::HMAC_SHA1, oaParams, QOAuth::ParseForHeaderArguments ) );
    header.prepend( "Authorization: " );
    return header;
}
