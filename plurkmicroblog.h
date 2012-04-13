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
#ifndef PLURKMICROBLOGPLUGIN_H
#define PLURKMICROBLOGPLUGIN_H

#include <KUrl>
#include <kio/jobclasses.h>
#include <QtOAuth/QtOAuth>
#include <kio/job.h>
#include <choqok/microblog.h>
#include <QPointer>
#include <qjson/parser.h>

class PlurkAccount;
class ChoqokEditAccountWidget;
class KJob;

/*
   Plurk types: all/new responses
   All
   My Plurks
   Private
   Responded
   Liked & Replurked
*/


enum ListMode {PublicPlurk, PrivatePlurk};

class UserInfo : public Choqok::User
{
public:
    ChoqokId listId;
    QString name;
    QString fullname;
    QString slug;
    QString description;
    int subscriberCount;
    int memberCount;
    QString uri;
    bool isFollowing;
    ListMode mode;
    Choqok::User author;
};

class PostInfo : public Choqok::Post
{
public:
    PostInfo() {}
    QString qualifier;
    QString qualifierTranslated;
    /*
       plurk_type=0 //Public plurk
       plurk_type=1 //Private plurk
       plurk_type=2 //Public plurk (responded by the logged in user)
       plurk_type=3 //Private plurk (responded by the logged in user)
    */
    int plurkType;
    /*
       If set to 1, then responses are disabled for this plurk.
       If set to 2, then only friends can respond to this plurk.
    */
    int noComments;
    QString contentRaw;
    int responseCount;
    int responseSeen;
    /*
       If the Plurk is public limited_to is null. If the Plurk is posted to a
       user's friends then limited_to is [0]. If limited_to is [1,2,6,3] then
       it's posted only to these user ids.
    */
    QString limitedTo;
};

class PlurkMicroBlog : public Choqok::MicroBlog
{
Q_OBJECT
public:
    PlurkMicroBlog( QObject* parent, const QVariantList& args  );
    ~PlurkMicroBlog();

    virtual Choqok::Account *createNewAccount( const QString &alias );
    virtual ChoqokEditAccountWidget * createEditAccountWidget( Choqok::Account *account, QWidget *parent );
    virtual Choqok::UI::TimelineWidget * createTimelineWidget( Choqok::Account* account,
                                                           const QString& timelineName, QWidget* parent );
    virtual Choqok::UI::PostWidget* createPostWidget(Choqok::Account* account,
                                                  const Choqok::Post& post, QWidget* parent);

    virtual void createPost(Choqok::Account*, Choqok::Post*);
    virtual void abortCreatePost(Choqok::Account*, Choqok::Post*);
    virtual void fetchPost(Choqok::Account*, Choqok::Post*);
    virtual void removePost(Choqok::Account*, Choqok::Post*);
    virtual void createFavorite( Choqok::Account* theAccount, const QString& postId );
    virtual void removeFavorite( Choqok::Account* theAccount, const QString& postId );
    virtual QString profileUrl( Choqok::Account* account, const QString& username ) const;


    virtual Choqok::TimelineInfo* timelineInfo(const QString& timelineName);
    virtual QList< Choqok::Post* > loadTimeline(Choqok::Account* accountAlias, const QString& timelineName);
    virtual void updateTimelines(Choqok::Account *theAccount);
    virtual void saveTimeline(Choqok::Account *account, const QString& timelineName,
                              const QList< Choqok::UI::PostWidget* > &timeline);

    virtual void listFriendsUsername( PlurkAccount *theAccount );
    virtual QString postUrl( Choqok::Account *account, const QString &username, const QString &postId) const;

    void getProfile(PlurkAccount* theAccount);
    QDateTime dateFromString( const QString &date );
    QString timelineApiPath;

Q_SIGNALS:
    void favoriteCreated( Choqok::Account* account, Choqok::Post* post );
    void favoriteRemoved( Choqok::Account* account, Choqok::Post* post );
    void friendshipCreated( Choqok::Account* theAccount, Choqok::User* user );
    void friendshipRemoved( Choqok::Account* theAccount, Choqok::User* user );
    void userLists(Choqok::Account* theAccount, const QString& username, QList<UserInfo> lists);
    void friendsUsernameListed( PlurkAccount * theAccount, const QMap< QString, QString > & friendsList );
    void responseReceived( Choqok::UI::PostWidget*, QList<Choqok::Post*>);

protected Q_SLOTS:
    virtual void requestFriendsScreenName( PlurkAccount* theAccount );
    virtual void slotRequestTimeline( KJob *job );
    virtual void slotRequestFriendsScreenName( KJob *job );
    void slotGetProfile(KJob* job);
    void slotGetResponse(Choqok::UI::PostWidget*, Choqok::Account* theAccount, const QString& id);
    void slotResponseReceived(KJob* job);
    void slotCreatePost ( KJob* );

protected:
    virtual void requestTimeLine(Choqok::Account *theAccount, QString timelineName,
                                 QString sincePostId );
    QHash<QString, Choqok::TimelineInfo*> mTimelineInfos;//timelineName, Info
    QHash<KJob*, Choqok::Post*> mCreatePostMap;//Job, post
    virtual Choqok::Post * readPostFromJsonMap( Choqok::Account* theAccount,
                                                   const QVariantMap& var, PostInfo* post,
						   const QVariantMap& userMap );
    virtual Choqok::Post * readPostFromJson( Choqok::Account* theAccount,
                                            const QByteArray& buffer, Choqok::Post* post );
    virtual QList<Choqok::Post*> readTimelineFromJson( Choqok::Account* theAccount, const QByteArray& buffer );
    virtual QList<Choqok::Post*> readResponseFromJson( Choqok::Account* theAccount, const QByteArray& buffer );
    virtual QMap< QString, QString > readUsersScreenNameFromJson( Choqok::Account *theAccount, const QByteArray & buffer );
    virtual Choqok::User readUserFromJsonMap( Choqok::Account* theAccount, const QVariantMap& map );
    virtual QString checkJsonForError(const QByteArray &buffer);
    QVariantMap readProfileFromJson( const QByteArray & buffer );
    QMap<KJob*, QString> mRequestTimelineMap;//Job, TimelineType
    QHash< Choqok::Account*, QMap<QString, QString> > mTimelineLatestId;//TimelineType, LatestId
    QMap<KJob*, Choqok::Account*> mJobsAccount;
    QMap<KJob*, Choqok::UI::PostWidget*> mJobsPostWidget;

private:
    KIO::StoredTransferJob * makeRequest( PlurkAccount * account, const QString & apiPath, const QMap< QString, QString > & params = QMap< QString, QString >() );
    QMap<QString, Choqok::TimelineInfo*> mListsInfo;
    int countOfTimelinesToSave;
    QMap<QString, int> monthes;
    QJson::Parser parser;
    QMap< QString, QString > friendsMap;
    QMap<QString, QString> mBaseUrlMap;//remoteUrl, BaseUrl
};

#endif
