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

#ifndef PLURKEDITACCOUNT_H
#define PLURKEDITACCOUNT_H

#include "editaccountwidget.h"
#include <QWidget>
#include "ui_plurkeditaccount_base.h"

#include "plurkapioauth.h"

class KJob;
class QProgressBar;
class PlurkAccount;
class PlurkMicroBlog;
/**

@author AceLan Kao \<acelan@acelan.idv.tw\>
*/
class PlurkEditAccountWidget : public ChoqokEditAccountWidget, public Ui::PlurkEditAccountBase
{
    Q_OBJECT
public:
    PlurkEditAccountWidget(PlurkMicroBlog *microblog, PlurkAccount* account, QWidget *parent);

    /**
    * Destructor
    */
    virtual ~PlurkEditAccountWidget();

    virtual bool validateData();

    /**
    * Create a new account if we are in the 'add account wizard',
    * otherwise update the existing account.
    * @Return new or modified account. OR 0L on failure.
    */
    virtual Choqok::Account *apply();

protected slots:
    virtual void authorizeUser();

protected:
    void loadTimelinesTableState();
    void saveTimelinesTableState();
    void setAuthenticated(bool authenticated);
    bool isAuthenticated;
    PlurkMicroBlog *mBlog;
    PlurkAccount *mAccount;
    QProgressBar *progress;
    QString username;
};

#endif
