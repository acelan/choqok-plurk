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

#include "plurkeditaccount.h"
#include "plurkmicroblog.h"
#include "plurkaccount.h"
#include <KDebug>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <KMessageBox>
#include <QDomDocument>
#include <QProgressBar>
#include <accountmanager.h>
#include <choqoktools.h>
#include <kio/accessmanager.h>
#include <QCheckBox>
#include <KInputDialog>

PlurkEditAccountWidget::PlurkEditAccountWidget(PlurkMicroBlog *microblog,
                                                    PlurkAccount* account, QWidget* parent)
    : ChoqokEditAccountWidget(account, parent), mAccount(account)
{
    setupUi(this);
    connect(kcfg_authorize, SIGNAL(clicked(bool)), SLOT(authorizeUser()));
    if(mAccount) {
        kcfg_alias->setText( mAccount->alias() );
        if( PlurkApiOAuth::self()->oauthToken().isEmpty() || PlurkApiOAuth::self()->oauthTokenSecret().isEmpty()) {
            setAuthenticated(false);
        } else {
            setAuthenticated(true);
        }
    } else {
        setAuthenticated(false);
        QString newAccountAlias = microblog->serviceName();
        QString servName = newAccountAlias;
        int counter = 1;
        while(Choqok::AccountManager::self()->findAccount(newAccountAlias)){
            newAccountAlias = QString("%1%2").arg(servName).arg(counter);
            counter++;
        }
        setAccount( mAccount = new PlurkAccount(microblog, newAccountAlias) );
        kcfg_alias->setText( newAccountAlias );
    }
//    loadTimelinesTableState();
    kcfg_alias->setFocus(Qt::OtherFocusReason);
}

PlurkEditAccountWidget::~PlurkEditAccountWidget()
{

}

bool PlurkEditAccountWidget::validateData()
{
    if(kcfg_alias->text().isEmpty() || !isAuthenticated )
        return false;
    else
        return true;
}

Choqok::Account* PlurkEditAccountWidget::apply()
{
    kDebug();
    mAccount->setAlias(kcfg_alias->text());
    mAccount->writeConfig();
    return mAccount;
}

void PlurkEditAccountWidget::authorizeUser()
{
    kDebug();
    bool ok= PlurkApiOAuth::self()->authorizeUser();
    if( ok)
        setAuthenticated(ok);
}

void PlurkEditAccountWidget::setAuthenticated(bool authenticated)
{
    isAuthenticated = authenticated;
    if(authenticated){
        kcfg_authorize->setIcon(KIcon("object-unlocked"));
        kcfg_authenticateLed->on();
        kcfg_authenticateStatus->setText(i18n("Authenticated"));
    } else {
        kcfg_authorize->setIcon(KIcon("object-locked"));
        kcfg_authenticateLed->off();
        kcfg_authenticateStatus->setText(i18n("Not Authenticated"));
    }
}

#include "plurkeditaccount.moc"
