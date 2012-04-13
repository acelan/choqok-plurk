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

#include "plurkmicroblog.h"

#include <KLocale>
#include <KDebug>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <QDomElement>
#include <KAboutData>
#include <KGenericFactory>
#include <qjson/parser.h>
#include "account.h"
#include "accountmanager.h"
#include "timelinewidget.h"
#include "editaccountwidget.h"
#include "plurkeditaccount.h"
#include "postwidget.h"
#include "plurkaccount.h"
#include "composerwidget.h"
#include "plurkpostwidget.h"
#include "choqokbehaviorsettings.h"
#include "choqokappearancesettings.h"
#include <KMenu>
#include <KAction>
#include <KMessageBox>
#include "choqoktypes.h"
#include "plurktimelinewidget.h"
#include "application.h"
#include "mediamanager.h"

K_PLUGIN_FACTORY( PlurkMicroBlogFactory, registerPlugin < PlurkMicroBlog > (); )
K_EXPORT_PLUGIN( PlurkMicroBlogFactory( "choqok_plurk" ) )

KIO::StoredTransferJob * PlurkMicroBlog::makeRequest( PlurkAccount * account, const QString & apiPath, const QMap< QString, QString > & params ) {
    QByteArray data;
    for( QMap< QString, QString >::const_iterator it = params.begin(); it != params.end(); ++it ) {
        QByteArray key( QUrl::toPercentEncoding( it.key() ) ), value( QUrl::toPercentEncoding( it.value() ) );
        data += key + "=" + value + "&";
    }
    data.chop( 1 );

    KUrl url( "http://www.plurk.com/APP" );
    url.addPath( apiPath );

    // NOTE Plurk API 2.0 always use OAuth
    QByteArray header( PlurkApiOAuth::self()->makeHeader( url.url(), params ) );
//    kDebug() << header;

    // FIXME when will this object be deleted?
    KIO::StoredTransferJob * job = KIO::storedHttpPost( data, url, KIO::HideProgressInfo );
    // for POST method
    job->addMetaData( "content-type", "Content-Type: application/x-www-form-urlencoded" );
    job->addMetaData( "customHTTPHeader", header );

    Q_UNUSED(account);
    return job;
}

PlurkMicroBlog::PlurkMicroBlog ( QObject* parent, const QVariantList&  )
: MicroBlog(PlurkMicroBlogFactory::componentData(), parent)
{
    kDebug();
    setServiceName("Plurk");
    setServiceHomepageUrl("http://www.plurk.com/");
    setCharLimit(140);
    QStringList timelineTypes;
    timelineTypes<< "All" << "Mine" << "Private" << "Responded" << "Favorite";
    setTimelineNames(timelineTypes);
    timelineApiPath = "/Timeline/getPlurks";

    monthes["Jan"] = 1;
    monthes["Feb"] = 2;
    monthes["Mar"] = 3;
    monthes["Apr"] = 4;
    monthes["May"] = 5;
    monthes["Jun"] = 6;
    monthes["Jul"] = 7;
    monthes["Aug"] = 8;
    monthes["Sep"] = 9;
    monthes["Oct"] = 10;
    monthes["Nov"] = 11;
    monthes["Dec"] = 12;

    Choqok::TimelineInfo *t = new Choqok::TimelineInfo;
    t->name = i18nc("Timeline Name", "All");
    t->description = i18nc("Timeline description", "You and your friends");
    t->icon = "user-home";
    mTimelineInfos["All"] = t;

    t = new Choqok::TimelineInfo;
    t->name = i18nc("Timeline Name", "Mine");
    t->description = i18nc("Timeline description", "Your post");
    t->icon = "edit-undo";
    mTimelineInfos["Mine"] = t;

    t = new Choqok::TimelineInfo;
    t->name = i18nc("Timeline Name", "Private");
    t->description = i18nc("Timeline description", "Your incoming private messages");
    t->icon = "mail-folder-inbox";
    mTimelineInfos["Private"] = t;

    t = new Choqok::TimelineInfo;
    t->name = i18nc("Timeline Name", "Responded");
    t->description = i18nc("Timeline description", "The messages you have replied");
    t->icon = "mail-folder-outbox";
    mTimelineInfos["Responded"] = t;

    t = new Choqok::TimelineInfo;
    t->name = i18nc("Timeline Name", "Favorite");
    t->description = i18nc("Timeline description", "Your favorites");
    t->icon = "favorites";
    mTimelineInfos["Favorite"] = t;
}

PlurkMicroBlog::~PlurkMicroBlog()
{
    kDebug();
}

Choqok::Account * PlurkMicroBlog::createNewAccount( const QString &alias )
{
    PlurkAccount *acc = qobject_cast<PlurkAccount*>( Choqok::AccountManager::self()->findAccount(alias) );
    if(!acc) {
        return new PlurkAccount(this, alias);
    } else {
        return 0;
    }
}

ChoqokEditAccountWidget * PlurkMicroBlog::createEditAccountWidget( Choqok::Account *account, QWidget *parent )
{
    kDebug();
    PlurkAccount *acc = qobject_cast<PlurkAccount*>(account);
    if(acc || !account)
        return new PlurkEditAccountWidget(this, acc, parent);
    else{
        kDebug()<<"Account passed here is not a PlurkAccount!";
        return 0L;
    }
}

Choqok::UI::TimelineWidget * PlurkMicroBlog::createTimelineWidget( Choqok::Account *account,
                                                                 const QString &timelineName, QWidget *parent )
{
    return new PlurkTimelineWidget(account, timelineName, parent);
}

Choqok::UI::PostWidget* PlurkMicroBlog::createPostWidget(Choqok::Account* account,
                                                        const Choqok::Post &post, QWidget* parent)
{
    return new PlurkPostWidget(account, post, parent);
}

void PlurkMicroBlog::createPost( Choqok::Account* theAccount, Choqok::Post* post )
{
    kDebug();
    PlurkAccount* account = qobject_cast<PlurkAccount*>(theAccount);

    if ( !post || post->content.isEmpty() ) {
        kDebug() << "ERROR: Status text is empty!";
        emit errorPost ( theAccount, post, Choqok::MicroBlog::OtherError,
                         i18n ( "Creating the new post failed. Text is empty." ), MicroBlog::Critical );
        return;
    }

    QMap< QString, QString > params;
    params.insert( "content", post->content );
    // TODO let user select this in GUI
    params.insert( "qualifier", ":" );

    if( post->isPrivate ) {
        // TODO multiple friends support
        params.insert( "limited_to", "[" + post->replyToUserId + "]" );
    }

    KIO::StoredTransferJob * job = makeRequest( account, "/Timeline/plurkAdd", params );
    if ( !job ) {
        kDebug() << "Cannot create an http POST request!";
        return;
    }
    mCreatePostMap[ job ] = post;
    mJobsAccount[job] = theAccount;
    connect ( job, SIGNAL ( result ( KJob* ) ), this, SLOT ( slotCreatePost ( KJob* ) ) );
    job->start();
}

void PlurkMicroBlog::slotCreatePost ( KJob* )
{
}

void PlurkMicroBlog::abortCreatePost( Choqok::Account* theAccount, Choqok::Post* post )
{
    if ( mCreatePostMap.isEmpty() )
        return;

    if ( !post ) {
        QHash<KJob*, Choqok::Post*>::Iterator it = mCreatePostMap.begin();
        QHash<KJob*, Choqok::Post*>::Iterator end = mCreatePostMap.end();
        while ( it != end ) {
            KJob* job = it.key();
            if ( mJobsAccount.value( job ) == theAccount )
                job->kill( KJob::EmitResult );
            ++it;
        }
    }

    mCreatePostMap.key( post )->kill( KJob::EmitResult );
}

void PlurkMicroBlog::fetchPost( Choqok::Account* theAccount, Choqok::Post* post )
{
    // to notify plurkpostwidget to update the post
    emit postFetched ( theAccount, post );
}

void PlurkMicroBlog::removePost( Choqok::Account* theAccount, Choqok::Post* post )
{
    Q_UNUSED(theAccount);
    Q_UNUSED(post);
}

Choqok::TimelineInfo* PlurkMicroBlog::timelineInfo(const QString& timelineName)
{
    if ( isValidTimeline( timelineName ) )
        return mTimelineInfos.value( timelineName );
    else
        return 0;
}

QMap< QString, QString > PlurkMicroBlog::readUsersScreenNameFromJson(Choqok::Account* theAccount,
                                                             const QByteArray& buffer)
{
    QMap< QString, QString > data;
    bool ok;
    QVariantList jsonList = parser.parse(buffer, &ok).toList();

    if ( ok ) {
        QVariantList::const_iterator it = jsonList.constBegin();
        QVariantList::const_iterator endIt = jsonList.constEnd();
        for(; it!=endIt; ++it){
            QVariantMap tmp( it->toMap() );
            // NOTE some users has no display_name, in this case we need to fallback to nick_name
            QString screenName( tmp["display_name"].toString() );
            if( screenName.isEmpty() ) {
                screenName = tmp["nick_name"].toString();
            }
            data.insert( tmp["id"].toString(), screenName );
        }
    } else {
        QString err = i18n( "Retrieving the friends list failed. The data returned from the server is corrupted." );
        kDebug() << "JSON parse error: the buffer is: \n" << buffer;
        emit error(theAccount, ParsingError, err, Critical);
    }
    return data;
}

void PlurkMicroBlog::listFriendsUsername(PlurkAccount* theAccount)
{
    if ( theAccount ) {
        requestFriendsScreenName(theAccount);
    }
}

void PlurkMicroBlog::requestFriendsScreenName(PlurkAccount* theAccount)
{
    QMap< QString, QString > params;
    params.insert( "user_id", theAccount->userId() );
    params.insert( "offset", QString::number( friendsMap.size() ) );
    params.insert( "limit", QString::number( 100 ) );

    KIO::StoredTransferJob *job = makeRequest( theAccount, "/FriendsFans/getFriendsByOffset", params );
    if ( !job ) {
        kDebug() << "Cannot create an http POST request!";
        return;
    }
    mJobsAccount[job] = theAccount;
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotRequestFriendsScreenName(KJob*) ) );
    job->start();
    Choqok::UI::Global::mainWindow()->showStatusMessage( i18n("Updating friends list for account %1 ...", theAccount->username()) );
}

void PlurkMicroBlog::slotRequestFriendsScreenName(KJob* job)
{
    kDebug();
    PlurkAccount *theAccount = qobject_cast<PlurkAccount *>( mJobsAccount.take(job) );
    KIO::StoredTransferJob* stJob = qobject_cast<KIO::StoredTransferJob*>( job );
    if (stJob->error()) {
        emit error(theAccount, ServerError, i18n("Friends list for account %1 could not be updated:\n%2",
            theAccount->username(), stJob->errorString()), Critical);
        return;
    }
// Friends data:  "[{"verified_account": false, "uid": 3321424, "following_im": 1, "is_premium": false, "full_name": "Macpaul Lin", "name_color": null, "timezone": null, "id": 3321424, "display_name": "macpaul", "date_of_birth": "Mon, 21 Jan 1980 00:01:00 GMT", "location": "Taipei, Taiwan", "recruited": 17, "bday_privacy": 2, "karma": 100.83, "default_lang": "tr_ch", "relationship": "not_saying", "dateformat": 0, "has_profile_image": 1, "following_tl": 1, "email_confirmed": true, "settings": true, "nick_name": "macpaul", "gender": 1, "avatar": 34, "following": true}, {"verified_account": false, "uid": 3225113, "following_im": 0, "is_premium": false, "full_name": "Manto Lin", "timezone": null, "id": 3225113, "display_name": "\u5305\u5b50\u4e0d\u5305\u8089", "date_of_birth": "Sat, 25 Apr 1981 00:01:00 GMT", "location": "Hsinchu, Taiwan", "recruited": 2, "bday_privacy": 2, "karma": 0.0, "default_lang": "tr_ch", "relationship": "not_saying", "dateformat": 0, "has_profile_image": 1, "following_tl": 1, "email_confirmed": true, "settings": true, "nick_name": "manto_lin", "gender": 1, "avatar": 2, "following": true}, {"verified_account": false, "uid": 3179033, "following_im": 1, "is_premium": false, "full_name": "Yosimasu Hayashi", "name_color": null, "timezone": null, "id": 3179033, "display_name": "\u4e00\u652f\u7a0b\u5f0f", "date_of_birth": "Mon, 24 Nov 1975 00:01:00 GMT", "location": "Chungli, Taiwan", "recruited": 2, "bday_privacy": 2, "karma": 55.19, "default_lang": "tr_ch", "relationship": "married", "dateformat": 0, "has_profile_image": 1, "following_tl": 1, "email_confirmed": true, "settings": true, "nick_name": "masu", "gender": 1, "avatar": 2, "following": true},
kDebug() << "Friends data: " << stJob->data() << endl;
    QMap< QString, QString > newList;
    newList = readUsersScreenNameFromJson( theAccount, stJob->data() );
    friendsMap.unite( newList );
    if ( newList.count() == 100 ) {
        requestFriendsScreenName( theAccount );
    } else {
        // TODO how do we store a (id,display_name) pair?
//        theAccount->setFriendsList( friendsList );
        Choqok::UI::Global::mainWindow()->showStatusMessage(i18n("Friends list for account %1 has been updated.",
            theAccount->username()) );
        emit friendsUsernameListed( theAccount, friendsMap );
    }
    disconnect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotRequestFriendsScreenName(KJob*) ) );
}

void PlurkMicroBlog::createFavorite( Choqok::Account* theAccount, const QString& postId )
{
    Q_UNUSED(theAccount);
    Q_UNUSED(postId);
}

void PlurkMicroBlog::removeFavorite( Choqok::Account* theAccount, const QString& postId )
{
    Q_UNUSED(theAccount);
    Q_UNUSED(postId);
}

QDateTime PlurkMicroBlog::dateFromString ( const QString &date )
{
    // Franklin.20110826: Can not use QDateTime::fromString(), because
    // localized dayname/monthname will become Chinese under locale zh_TW.*
    // hence making the parsing misbehaved.
    char wday[10], mon[10], tz[10];
    int year, day, hours, minutes, seconds;

    //sscanf( qPrintable ( date ), "%*s %s %d %d:%d:%d %*s %d", s, &day, &hours, &minutes, &seconds, &year );
    // Franklin.20110826
    sscanf( qPrintable ( date ), "%s %d %s %d %d:%d:%d %s", wday, &day, mon, &year, &hours, &minutes, &seconds, tz);

    int month = monthes[mon];

    QDateTime recognized ( QDate ( year, month, day ), QTime ( hours, minutes, seconds ) );
    recognized.setTimeSpec( Qt::UTC );

    return recognized.toLocalTime();
}

QString PlurkMicroBlog::checkJsonForError(const QByteArray& buffer)
{
    bool ok;
    QVariantMap map = parser.parse(buffer, &ok).toMap();
    if(ok && map.contains("error")){
        kError()<<"Error at request "<<map.value("request").toString()<<" : "<<map.value("error").toString();
        return map.value("error").toString();
    }
    return QString();
}

QList< Choqok::Post* > PlurkMicroBlog::readTimelineFromJson(Choqok::Account* theAccount,
                                                                 const QByteArray& buffer)
{
    QList<Choqok::Post*> postList;
    bool ok;
    QVariantMap plurksMap= parser.parse(buffer, &ok).toMap();
    QVariantMap plurkUsersMap = plurksMap["plurk_users"].toMap();
    QVariantList plurksList = plurksMap["plurks"].toList();
//    kDebug() << "map: " << plurksMap << endl;
//    kDebug() << "plurk_users: " << plurkUsersMap << endl;
//    kDebug() << "plurks(" << plurksList.count() << "): " << plurksList << endl;

    kDebug() << "parse result: " << ok << endl;
    if ( ok ) {
        QVariantList::const_iterator it = plurksList.constBegin();
        QVariantList::const_iterator endIt = plurksList.constEnd();
        for(; it != endIt; ++it){
            QVariantMap plurkMap= it->toMap();
//            kDebug() << "owner_id: " << plurkMap["owner_id"].toString() << endl;
            postList.prepend(readPostFromJsonMap(theAccount, it->toMap(), new PostInfo, plurkUsersMap[plurkMap["owner_id"].toString()].toMap()));
        }
    } else {
        QString err = checkJsonForError(buffer);
        if(err.isEmpty()){
            kError() << "JSON parsing failed.\nBuffer was: \n" << buffer;
            emit error(theAccount, ParsingError, i18n("Could not parse the data that has been received from the server."));
        } else {
            Q_EMIT error(theAccount, ServerError, err);
        }
        return postList;
    }
    return postList;
}

Choqok::Post* PlurkMicroBlog::readPostFromJson(Choqok::Account* theAccount,
                                                    const QByteArray& buffer,
                                                    Choqok::Post* post)
{
    bool ok;
    qDebug() << buffer;
    QVariantMap plurksMap= parser.parse(buffer, &ok).toMap();
    QVariantMap plurkUserMap = plurksMap["plurk_users"].toMap();
    QVariantMap plurkMap = plurksMap["plurks"].toMap();

    if ( ok ) {
        return readPostFromJsonMap ( theAccount, plurkMap, reinterpret_cast< PostInfo* >( post ), plurkUserMap );
    } else {
        if(!post){
            kError()<<"PlurkMicroBlog::readPostFromXml: post is NULL!";
            post = new Choqok::Post;
        }
        emit errorPost(theAccount, post, ParsingError, i18n("Could not parse the data that has been received from the server."));
        kError()<<"JSon parsing failed. Buffer was:"<<buffer;
        post->isError = true;
        return post;
    }
}

Choqok::Post* PlurkMicroBlog::readPostFromJsonMap(Choqok::Account* theAccount,
                                                       const QVariantMap& var,
                                                       PostInfo* post, const QVariantMap& userMap)
{
    if(!post){
        kError()<<"PlurkMicroBlog::readPostFromJsonMap: post is NULL!";
        return 0;
    }
/*
userMap:  QMap(("avatar", QVariant(qulonglong, 12) ) ( "bday_privacy" ,  QVariant(qulonglong, 0) ) ( "date_of_birth" ,  QVariant(, ) ) ( "dateformat" ,  QVariant(qulonglong, 0) ) ( "default_lang" ,  QVariant(QString, "tr_ch") ) ( "display_name" ,  QVariant(QString, "阿斯~~蘭") ) ( "email_confirmed" ,  QVariant(bool, true) ) ( "full_name" ,  QVariant(QString, "AceLan Kao") ) ( "gender" ,  QVariant(qulonglong, 1) ) ( "has_profile_image" ,  QVariant(qulonglong, 1) ) ( "id" ,  QVariant(qulonglong, 3328986) ) ( "is_premium" ,  QVariant(bool, false) ) ( "karma" ,  QVariant(double, 124.63) ) ( "location" ,  QVariant(QString, "Taipei, Taiwan") ) ( "name_color" ,  QVariant(QString, "2264D6") ) ( "nick_name" ,  QVariant(QString, "acelan") ) ( "timezone" ,  QVariant(, ) ) ( "verified_account" ,  QVariant(bool, false) ) )

post:  QMap(("content", QVariant(QString, "<a href="http://images.plurk.com/5537df64b3643ad20c87ab51a940a4d9.jpg" class="ex_link pictureservices"><img src="http://images.plurk.com/tn_5537df64b3643ad20c87ab51a940a4d9.gif" alt="http://images.plurk.com/5537df64b3643ad20c87ab51a940a4d9.jpg" height="30" /></a> # 停了好久的 plurk client 這幾天又把 code 抓出來重弄 終於比較像樣了點 <img src="http://statics.plurk.com/2d5e21929e752498e36d74096b1965e1.gif" class="emoticon" alt=":-P" height="18" />") ) ( "content_raw" ,  QVariant(QString, "http://images.plurk.com/5537df64b3643ad20c87ab51a940a4d9.jpg # 停了好久的 plurk client 這幾天又把 code 抓出來重弄 終於比較像樣了點 :p") ) ( "favorers" ,  QVariant(QVariantList, (QVariant(qulonglong, 3156986) ,  QVariant(qulonglong, 3239874) )  ) ) ( "favorite" ,  QVariant(bool, false) ) ( "favorite_count" ,  QVariant(qulonglong, 2) ) ( "is_unread" ,  QVariant(qulonglong, 0) ) ( "lang" ,  QVariant(QString, "tr_ch") ) ( "limited_to" ,  QVariant(, ) ) ( "no_comments" ,  QVariant(qulonglong, 0) ) ( "owner_id" ,  QVariant(qulonglong, 3328986) ) ( "plurk_id" ,  QVariant(qulonglong, 963456602) ) ( "plurk_type" ,  QVariant(qulonglong, 2) ) ( "posted" ,  QVariant(QString, "Thu, 22 Mar 2012 09:11:13 GMT") ) ( "qualifier" ,  QVariant(QString, "shares") ) ( "qualifier_translated" ,  QVariant(QString, "分享") ) ( "replurkable" ,  QVariant(bool, false) ) ( "replurked" ,  QVariant(bool, false) ) ( "replurker_id" ,  QVariant(qulonglong, 0) ) ( "replurkers" ,  QVariant(QVariantList, () ) ) ( "replurkers_count" ,  QVariant(qulonglong, 0) ) ( "response_count" ,  QVariant(qulonglong, 19) ) ( "responses_seen" ,  QVariant(qulonglong, 0) ) ( "user_id" ,  QVariant(qulonglong, 3328986) ) )
*/
//    kDebug() << "userMap: " << userMap << endl;
//    kDebug() << "post: " << var << endl;

    // content:  "<a href="http://pum.hrw.mega88.com/i/p04.gif" class="ex_link pictureservices" rel="nofollow"><img src="http://pum.hrw.mega88.com/i/p04.gif" height="30" /></a> [社群互動噗] 吉普每六小時會發一篇社群互動噗，如果人在線上想找人對決、冒險、送餐點的，可以在這篇噗中回應唷～請在自己的堆疊中用『 <img class="emoticon_my" src="http://emos.plurk.com/aaa80e5cf1da9a8d75df4b0389723af3_w33_h22.png" width="33" height="22" /> 挑戰/冒險/贈送料理 目標帳號』的指令，與其他家的女兒進行互動喔～"
//    kDebug() << "content: " << var["content"].toString() << endl;

    // qualifier_translated: Only set if the language is not English, will be the translated qualifier.
    if( var["qualifier_translated"].toString() != "")
	    post->content = var["qualifier_translated"].toString() + " " + var["content"].toString();
    else
	    post->content = var["qualifier"].toString() + " " + var["content"].toString();

    post->contentRaw = var["content_raw"].toString();

    post->creationDateTime = dateFromString(var["posted"].toString());
// create time :  "2012-4-11T13:59:16"
// kDebug() << "create time : " << post->creationDateTime.toString("yyyy-M-dThh:mm:ss") << endl;

    post->isFavorited = var["favorite"].toBool();
    post->postId = var["plurk_id"].toString();
//    post->source = ""; // plurk don't have this field
    post->plurkType = var["plurk_type"].toInt();

    post->author = readUserFromJsonMap( theAccount, userMap);

    post->link = "http://www.plurk.com/p/" + QString::number( var["plurk_id"].toInt(), 36);
    post->isRead = !var["is_unread"].toBool();
    post->responseCount = var["response_count"].toInt();
    post->responseSeen = var["responses_seen"].toInt();
    if(post->responseSeen != 0)
	    post->isRead = false;

    return reinterpret_cast< Choqok::Post * >( post );
}

Choqok::User PlurkMicroBlog::readUserFromJsonMap(Choqok::Account* theAccount, const QVariantMap& map)
{
    Q_UNUSED(theAccount);
    Choqok::User u;
    u.realName = map["nick_name"].toString();
    u.userId = map["id"].toString();
    u.userName = map["display_name"].toString();

    u.profileImageUrl = "http://www.plurk.com/static/default_small.gif";
    if( map["has_profile_image"].toBool())
    {
	    u.profileImageUrl = "http://avatars.plurk.com/" + u.userId + "-medium";
	    if( map["avatar"].toInt())
		    u.profileImageUrl += map["avatar"].toString();
	    u.profileImageUrl += ".gif";
    }
    return u;
}

QVariantMap PlurkMicroBlog::readProfileFromJson( const QByteArray& buffer)
{
    bool ok = false;
    QVariantMap profile( parser.parse( buffer, &ok ).toMap() );
    if( !ok ) {
        QString err = i18n( "Retrieving the profile failed. The data returned from the server is corrupted." );
        kDebug() << "JSON parse error: the buffer is: \n" << buffer;
        return QVariantMap();
    }
    return profile["user_info"].toMap();
}

QList< Choqok::Post* > PlurkMicroBlog::loadTimeline( Choqok::Account *account,
                                                          const QString& timelineName)
{
    QList< Choqok::Post* > list;
    if(timelineName.compare("Favorite") == 0)
        return list;//NOTE Won't cache favorites, and this is for compatibility with older versions!
    kDebug()<<timelineName;
    QString fileName = Choqok::AccountManager::generatePostBackupFileName(account->alias(), timelineName);
    KConfig postsBackup( "choqok/" + fileName, KConfig::NoGlobals, "data" );
    QStringList tmpList = postsBackup.groupList();

    if( tmpList.isEmpty() || !(QDateTime::fromString(tmpList.first()).isValid()) )
        return list;

    QList<QDateTime> groupList;
    foreach(const QString &str, tmpList)
        groupList.append(QDateTime::fromString(str) );
    qSort(groupList);
    int count = groupList.count();
    if( count ) {
        PostInfo *st = 0;
        for ( int i = 0; i < count; ++i ) {
            st = new PostInfo;
            KConfigGroup grp( &postsBackup, groupList[i].toString() );
            st->postId = grp.readEntry( "plurk_id", QString() );
	    st->qualifier = grp.readEntry( "qualifier", QString() );
            st->qualifierTranslated = grp.readEntry( "qualifierTranslated", QString() );
            st->isRead = grp.readEntry( "is_unread", true);
            st->plurkType = grp.readEntry( "plurk_type", 0);
            st->author.userId = grp.readEntry( "user_id", QString() );
            st->creationDateTime = dateFromString(grp.readEntry( "posted", QDateTime::currentDateTime().toString()));
            st->noComments = grp.readEntry( "no_comments", 0 );
            st->content = grp.readEntry( "text", QString() );
            st->source = grp.readEntry( "content_raw", QString() );
            st->responseCount = grp.readEntry( "responseCount", 0 );
            st->responseSeen = grp.readEntry( "responseSeen", 0 );
            st->limitedTo= grp.readEntry( "limited_to", QString() );

            list.append( st );
        }
        mTimelineLatestId[account][timelineName] = st->creationDateTime.toString("yyyy-M-dThh:mm:ss");
    }
    return list;
}

void PlurkMicroBlog::saveTimeline(Choqok::Account *account,
                                       const QString& timelineName,
                                       const QList< Choqok::UI::PostWidget* > &timeline)
{
    if(timelineName.compare("Favorite") == 0)
        return;//NOTE Won't cache favorites, and this is for compatibility with older versions!
//    kDebug();
    QString fileName = Choqok::AccountManager::generatePostBackupFileName(account->alias(), timelineName);
    KConfig postsBackup( "choqok/" + fileName, KConfig::NoGlobals, "data" );

    ///Clear previous data:
    QStringList prevList = postsBackup.groupList();
    int c = prevList.count();
    if ( c > 0 ) {
        for ( int i = 0; i < c; ++i ) {
            postsBackup.deleteGroup( prevList[i] );
        }
    }
    QList< Choqok::UI::PostWidget *>::const_iterator it, endIt = timeline.constEnd();
    for ( it = timeline.constBegin(); it != endIt; ++it ) {
        const Choqok::Post *post = &((*it)->currentPost());
        KConfigGroup grp( &postsBackup, post->creationDateTime.toString() );
        grp.writeEntry( "creationDateTime", post->creationDateTime );
        grp.writeEntry( "postId", post->postId.toString() );
        grp.writeEntry( "text", post->content );
        grp.writeEntry( "source", post->source );
        grp.writeEntry( "inReplyToPostId", post->replyToPostId.toString() );
        grp.writeEntry( "inReplyToUserId", post->replyToUserId.toString() );
        grp.writeEntry( "favorited", post->isFavorited );
        grp.writeEntry( "inReplyToUserName", post->replyToUserName );
        grp.writeEntry( "authorId", post->author.userId.toString() );
        grp.writeEntry( "authorUserName", post->author.userName );
        grp.writeEntry( "authorRealName", post->author.realName );
        grp.writeEntry( "authorProfileImageUrl", post->author.profileImageUrl );
        grp.writeEntry( "authorDescription" , post->author.description );
        grp.writeEntry( "isPrivate" , post->isPrivate );
        grp.writeEntry( "authorLocation" , post->author.location );
        grp.writeEntry( "isProtected" , post->author.isProtected );
        grp.writeEntry( "authorUrl" , post->author.homePageUrl );
        grp.writeEntry( "isRead" , post->isRead );
        grp.writeEntry( "repeatedFrom", post->repeatedFromUsername);
        grp.writeEntry( "repeatedPostId", post->repeatedPostId.toString());
    }
    postsBackup.sync();
    if(Choqok::Application::isShuttingDown()) {
        --countOfTimelinesToSave;
        if(countOfTimelinesToSave < 1)
            emit readyForUnload();
    }
}

void PlurkMicroBlog::updateTimelines (Choqok::Account* theAccount)
{
    kDebug();
    foreach ( const QString &tm, theAccount->timelineNames() ) {
        requestTimeLine ( theAccount, tm, mTimelineLatestId[theAccount][tm] );
    }
}

void PlurkMicroBlog::requestTimeLine ( Choqok::Account* theAccount, QString type,
                                            QString latestStatusId )
{
    kDebug();
    PlurkAccount* account = qobject_cast<PlurkAccount*>(theAccount);

    QMap< QString, QString > params;
    params.insert( "limit", "50" );
    if( type == "Mine")
    {
        params.insert( "filter", "only_user");
    }
    else if( type == "Private")
    {
        params.insert( "filter", "only_private");
    }
    else if( type == "Responded")
    {
        params.insert( "filter", "only_responded");
    }
    else if( type == "Favorite")
    {
        params.insert( "filter", "only_favorite");
    }

    if( !latestStatusId.isEmpty())
	    params.insert("offset", latestStatusId);

    KIO::StoredTransferJob *job = makeRequest( account, "/Timeline/getPlurks", params );
    if ( !job ) {
        kDebug() << "Cannot create an http POST request!";
        return;
    }
    mRequestTimelineMap[job] = type;
    mJobsAccount[job] = theAccount;
    connect ( job, SIGNAL ( result ( KJob* ) ), this, SLOT ( slotRequestTimeline ( KJob* ) ) );
    job->start();
}

void PlurkMicroBlog::slotRequestTimeline ( KJob *job )
{
//    kDebug();//TODO Add error detection for XML "checkXmlForError()" and JSON
    if ( !job ) {
        kDebug() << "Job is null pointer";
        return;
    }
    Choqok::Account *theAccount = mJobsAccount.take(job);
    if ( job->error() ) {
        kDebug() << "Job Error: " << job->errorString();
        emit error( theAccount, CommunicationError,
                    i18n("Timeline update failed, %1", job->errorString()), Low );
        return;
    }
    QString type = mRequestTimelineMap.take(job);
    kDebug() << "type is: " << type; // Mine, Private, Responded, Favorite
    KIO::StoredTransferJob* j = qobject_cast<KIO::StoredTransferJob*>( job );
    QList<Choqok::Post*> list;
    list = readTimelineFromJson( theAccount, j->data() );
    if(!list.isEmpty()) {
        kDebug() << "Parse JSON ok";
        mTimelineLatestId[theAccount][type] = list.last()->creationDateTime.toString("yyyy-M-dThh:mm:ss");
        emit timelineDataReceived( theAccount, type, list );
    }
    disconnect ( job, SIGNAL ( result ( KJob* ) ), this, SLOT ( slotRequestTimeline ( KJob* ) ) );
}

QString PlurkMicroBlog::postUrl(Choqok::Account*, const QString&, const QString&) const
{
	// TODO: this function is not necessary, but in libchoqok/microblog.cpp
	// It ask microblog plugin have to implement it
	return "";
}


QString PlurkMicroBlog::profileUrl( Choqok::Account*, const QString& username ) const
{
    return QString( "http://www.plurk.com/%1" ).arg( username);
}

void PlurkMicroBlog::getProfile(PlurkAccount* theAccount)
{
    KIO::StoredTransferJob *job = makeRequest( theAccount, "/Profile/getOwnProfile", QMap< QString, QString >());
    mJobsAccount[job] = theAccount;
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotGetProfile( KJob* ) ) );
    job->start();
}

void PlurkMicroBlog::slotGetProfile(KJob* job)
{
    PlurkAccount* theAccount = qobject_cast<PlurkAccount *>( mJobsAccount.take(job) );
    KIO::StoredTransferJob* stj = qobject_cast< KIO::StoredTransferJob * >( job );
    if( stj->error() ) {
        emit error(theAccount, ServerError, i18n("Profile for account %1 could not be updated:\n%2",
            theAccount->username(), stj->errorString()), Critical);
        return;
    }
    QVariantMap userData( this->readProfileFromJson( stj->data() ) );
    theAccount->setUserId( userData["id"].toString() );
    theAccount->setUsername( userData["display_name"].toString() );
// choqok(5685) PlurkMicroBlog::slotGetProfile: user id:  "3328986" user name:  "阿斯~~蘭"
kDebug() << "user id: " << userData["id"].toString() << "user name: " << userData["display_name"].toString() << endl;
    disconnect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotGetProfile( KJob* ) ) );
}

void PlurkMicroBlog::slotGetResponse( Choqok::UI::PostWidget* thePostWidget, Choqok::Account* theAccount, const QString& id)
{
kDebug();
    PlurkAccount* account = qobject_cast<PlurkAccount*>(theAccount);

    QMap< QString, QString > params;
    params.insert("plurk_id", id);

kDebug() << "Requesting the responses" << endl;;
    KIO::StoredTransferJob *job = makeRequest( account, "/Responses/get", params );
    if ( !job ) {
        kDebug() << "Cannot create an http POST request!";
        return;
    }
    mJobsAccount[job] = theAccount;
    mJobsPostWidget[job] = thePostWidget;
    connect ( job, SIGNAL ( result ( KJob* ) ), this, SLOT ( slotResponseReceived( KJob* ) ) );
    job->start();
}

void PlurkMicroBlog::slotResponseReceived(KJob* job)
{
kDebug();
    if ( !job ) {
        kDebug() << "Job is null pointer";
        return;
    }
    Choqok::Account *theAccount = mJobsAccount.take(job);
    Choqok::UI::PostWidget *thePostWidget = mJobsPostWidget.take(job);
    if ( job->error() ) {
        kDebug() << "Job Error: " << job->errorString();
        emit error( theAccount, CommunicationError,
                    i18n("Failed to get response: %1", job->errorString()), Low );
        return;
    }
    KIO::StoredTransferJob* j = qobject_cast<KIO::StoredTransferJob*>( job );
    QList<Choqok::Post*> list;
// data:  "{"friends": {"3328986": {"verified_account": false, "default_lang": "tr_ch", "display_name": "\u963f\u65af~~\u862d\uf8ff", "uid": 3328986, "dateformat": 0, "nick_name": "acelan", "has_profile_image": 1, "avatar": 12, "is_premium": false, "date_of_birth": null, "email_confirmed": true, "gender": 1, "name_color": "2264D6", "recruited": 2, "id": 3328986, "karma": 125.08}, "555903": {"verified_account": false, "default_lang": "tr_ch", "display_name": "faifai", "uid": 555903, "dateformat": 0, "nick_name": "faifai", "has_profile_image": 1, "avatar": 2, "is_premium": false, "date_of_birth": "Sat, 11 Sep 1976 00:01:00 GMT", "email_confirmed": true, "gender": 0, "name_color": null, "recruited": 0, "id": 555903, "karma": 103.64}}, "response_count": 2, "responses_seen": 2, "responses": [{"lang": "en", "content_raw": "(blush)", "user_id": 555903, "qualifier": "says", "plurk_id": 974122896, "content": "<img src=\"http:\/\/statics.plurk.com\/9939dd585cf0e8d39e7912a98a9ce727.gif\" class=\"emoticon\" alt=\"(blush)\" height=\"19\" \/>", "id": 4668926594, "posted": "Thu, 12 Apr 2012 01:57:27 GMT"}, {"lang": "en", "content_raw": "(K)", "user_id": 3328986, "qualifier": "says", "plurk_id": 974122896, "content": "<img src=\"http:\/\/statics.plurk.com\/9454d15bcaf411b159dcc147ebc3f0eb.gif\" class=\"emoticon\" alt=\"(K)\" height=\"19\" \/>", "id": 4668943560, "posted": "Thu, 12 Apr 2012 02:06:15 GMT"}]}"
//kDebug() << "data: " << j->data() << endl;
    list = readResponseFromJson( theAccount, j->data() );
    if(!list.isEmpty()) {
        emit responseReceived( thePostWidget, list );
    }
    disconnect ( job, SIGNAL ( result ( KJob* ) ), this, SLOT ( slotResponseReceived( KJob* ) ) );
}

QList< Choqok::Post* > PlurkMicroBlog::readResponseFromJson(Choqok::Account* theAccount,
                                                                 const QByteArray& buffer)
{
    QList<Choqok::Post*> postList;
    bool ok;
    QVariantMap plurksMap= parser.parse(buffer, &ok).toMap();
    QVariantMap plurkUsersMap = plurksMap["friends"].toMap();
    QVariantList plurksList = plurksMap["responses"].toList();

    if ( ok ) {
        QVariantList::const_iterator it = plurksList.constBegin();
        QVariantList::const_iterator endIt = plurksList.constEnd();
        for(; it != endIt; ++it){
            QVariantMap plurkMap= it->toMap();
            postList.prepend(readPostFromJsonMap(theAccount, it->toMap(), new PostInfo, plurkUsersMap[plurkMap["user_id"].toString()].toMap()));
        }
    } else {
        QString err = checkJsonForError(buffer);
        if(err.isEmpty()){
            kError() << "JSON parsing failed.\nBuffer was: \n" << buffer;
            emit error(theAccount, ParsingError, i18n("Could not parse the data that has been received from the server."));
        } else {
            Q_EMIT error(theAccount, ServerError, err);
        }
        return postList;
    }
    return postList;
}

#include "plurkmicroblog.moc"
