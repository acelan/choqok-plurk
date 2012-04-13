
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

#ifndef PLURKAPIOAUTH_H
#define PLURKAPIOAUTH_H
#include "choqok_export.h"

#include <QtCore/QSharedPointer>

class CHOQOK_EXPORT PlurkApiOAuth
{
public:
    static PlurkApiOAuth* self();

    bool authorizeUser();
    bool getPinCode();

    const QByteArray & oauthToken() const;
    void setOAuthToken( const QByteArray &token );

    const QByteArray & oauthTokenSecret() const;
    void setOAuthTokenSecret( const QByteArray &tokenSecret );

    const QByteArray & oauthConsumerKey() const;
    void setOAuthConsumerKey( const QByteArray &consumerKey );

    const QByteArray & oauthConsumerSecret() const;
	void setOAuthConsumerSecret( const QByteArray &consumerSecret );

    QByteArray makeHeader( const QString & url, const QMap< QString, QString > & params ) const;

private:
    PlurkApiOAuth();
    PlurkApiOAuth( const PlurkApiOAuth & );
	~PlurkApiOAuth();

    static PlurkApiOAuth* mSelf;
	class Private;
    QSharedPointer< Private > d;
};

#endif
