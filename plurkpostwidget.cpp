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

#include <KAction>
#include <KLocale>
#include <KMenu>
#include <KDebug>
#include <KPushButton>
#include <KUrl>

#include <choqok/mediamanager.h>
#include <choqokappearancesettings.h>
#include "mediamanager.h"
#include <textbrowser.h>

#include "plurkmicroblog.h"
#include "plurkpostwidget.h"
#include "plurkaccount.h"

QRegExp mGeneralImagesRegExp("(<img.*src=\")(https?://[^> ]*.(jpeg|jpg|gif|png|bmp))", Qt::CaseInsensitive);

const KIcon PlurkPostWidget::unFavIcon(Choqok::MediaManager::convertToGrayScale(KIcon("rating").pixmap(16)) );

PlurkPostWidget::PlurkPostWidget(Choqok::Account* account, const Choqok::Post& post, QWidget* parent)
: Choqok::UI::PostWidget(account, post, parent)
{
    PlurkMicroBlog* microblog = qobject_cast<PlurkMicroBlog*>(account->microblog());
    connect( microblog, SIGNAL(favoriteRemoved(Choqok::Account*,Choqok::Post*)),
             this, SLOT(slotFavoriteRemoved(Choqok::Account*,Choqok::Post*)) );
    connect( microblog, SIGNAL(favoriteCreated(Choqok::Account*,Choqok::Post*)),
             this, SLOT(slotFavoriteCreated(Choqok::Account*,Choqok::Post*)) );
}

PlurkPostWidget::~PlurkPostWidget()
{
}

void PlurkPostWidget::initUi()
{
    Choqok::UI::PostWidget::initUi();

    // mute, push, replurk, favorite

    KPushButton *btnRe = addButton( "btnReply",i18nc( "@info:tooltip", "Reply" ), "edit-undo" );
    QMenu *menu = new QMenu(btnRe);

    KAction *actRep = new KAction(KIcon("edit-undo"), i18n("Reply to %1", currentPost().author.userName), menu);
    menu->addAction(actRep);
    connect( actRep, SIGNAL(triggered(bool)), SLOT(slotReply()) );
    connect( btnRe, SIGNAL(clicked(bool)), SLOT(slotReply()) );

    KAction *actWrite = new KAction( KIcon("document-edit"), i18n("Write to %1", currentPost().author.userName),
                                     menu );
    menu->addAction(actWrite);
    connect( actWrite, SIGNAL(triggered(bool)), SLOT(slotWriteTo()) );

    KAction *actReplytoAll = new KAction(i18n("Reply to all"), menu);
    menu->addAction(actReplytoAll);
    connect( actReplytoAll, SIGNAL(triggered(bool)), SLOT(slotReplyToAll()) );

    menu->setDefaultAction(actRep);
    btnRe->setDelayedMenu(menu);

    btnFav = addButton( "btnFavorite",i18nc( "@info:tooltip", "Favorite" ), "rating" );
    btnFav->setCheckable(true);
    connect( btnFav, SIGNAL(clicked(bool)), SLOT(setFavorite()) );
    updateFavStat();
  
    setContent( parseImageTag( content()));
    originHtml = _mainWidget->toHtml();
}

void PlurkPostWidget::slotReply()
{
}

void PlurkPostWidget::slotWriteTo()
{
    emit reply( QString("@%1").arg( currentPost().author.userName ), QString(), currentPost().author.userName );
}

void PlurkPostWidget::slotReplyToAll()
{
    QString txt = QString("@%1").arg( currentPost().author.userName );
    emit reply( txt, currentPost().postId, currentPost().author.userName );
}

void PlurkPostWidget::setFavorite()
{
    setReadWithSignal();
    PlurkMicroBlog* microblog = qobject_cast<PlurkMicroBlog*>(currentAccount()->microblog());
    if(currentPost().isFavorited){
        connect(microblog, SIGNAL(favoriteRemoved(Choqok::Account*,QString)),
                this, SLOT(slotSetFavorite(Choqok::Account*,QString)) );
        microblog->removeFavorite(currentAccount(), currentPost().postId);
    } else {
        connect(microblog, SIGNAL(favoriteCreated(Choqok::Account*,QString)),
                   this, SLOT(slotSetFavorite(Choqok::Account*,QString)) );
        microblog->createFavorite(currentAccount(), currentPost().postId);
    }
}

void PlurkPostWidget::slotSetFavorite(Choqok::Account *theAccount, const QString& postId)
{
    if(currentAccount() == theAccount && postId == currentPost().postId){
        Choqok::Post tmp = currentPost();
        tmp.isFavorited = !tmp.isFavorited;
        setCurrentPost(tmp);
        updateFavStat();
    }
}

void PlurkPostWidget::updateFavStat()
{
    if(currentPost().isFavorited){
        btnFav->setChecked(true);
        btnFav->setIcon(KIcon("rating"));
    } else {
        btnFav->setChecked(false);
        btnFav->setIcon(unFavIcon);
    }
}

QString PlurkPostWidget::prepareStatus(const QString& txt)
{
    return txt;
}

QString PlurkPostWidget::parseImageTag( QString content)
{
    int len, pos = 0;

    mGeneralImagesRegExp.setMinimal(true);
    while ((pos = mGeneralImagesRegExp.indexIn(content, pos)) != -1) {
        len=pos;
//kDebug()<<mGeneralImagesRegExp.capturedTexts();
        QString link(mGeneralImagesRegExp.cap(2));
        QString resLink(link);
	resLink.replace("http://","img://");

	// remove <img> tag is necessary, for the img parameters will destroy the post layout
        int idx = content.indexOf("<img", pos, Qt::CaseInsensitive);
        if ((idx != -1) && (idx <= pos)) { // The image url comes with a <img> tag
            while (pos > 0) {
                if (content.at(pos) == '<')
                    break;
                pos--;
            }
            while (len <= content.length()) {
                if ( content.at(len) == '>')
                    break;
                len++;
            }
            content.remove(pos, len - pos + 1);   // remove <img> tag
            content.insert(pos, "<img src='"+resLink+"'>");
        }
        QPixmap *pix = Choqok::MediaManager::self()->fetchImage(link, Choqok::MediaManager::Async);
        if(pix)
            slotImageFetched(link, *pix);
        else
            connect( Choqok::MediaManager::self(), SIGNAL(imageFetched(QString,QPixmap)),
                     this, SLOT(slotImageFetched(QString,QPixmap)) );

        pos += link.length();
    }
    return content;
}

void PlurkPostWidget::slotImageFetched(const QString& remoteUrl, const QPixmap& pixmap)
{
    QPixmap pix = pixmap;
    if(pixmap.width() > 200) {
        pix = pixmap.scaledToWidth(200);
    } else if(pixmap.height() > 200) {
        pix = pixmap.scaledToHeight(200);
    }
    _mainWidget->document()->addResource( QTextDocument::ImageResource,
                                          QString(remoteUrl).replace("http://", "img://"), pix);
    disconnect( Choqok::MediaManager::self(), SIGNAL( imageFetched(QString,QPixmap)),
        this, SLOT(slotImageFetched(QString, QPixmap) ) );
}

void PlurkPostWidget::checkAnchor(const QUrl & url)
{
    QString scheme = url.scheme();
    if( scheme == "replyto" ) {
        if(isBasePostShowed) {
            setContent( prepareStatus(currentPost().content).replace("<a href","<a style=\"text-decoration:none\" href",Qt::CaseInsensitive) );
            isBasePostShowed = false;
            return;
        } else {
            connect(currentAccount()->microblog(), SIGNAL(postFetched(Choqok::Account*,Choqok::Post*)),
                    this, SLOT(slotBasePostFetched(Choqok::Account*,Choqok::Post*)) );
            Choqok::Post *ps = new Choqok::Post;
            ps->postId = url.host();
            currentAccount()->microblog()->fetchPost(currentAccount(), ps);
        }
    } else if (scheme == "thread") {
        // TODO: thread
    } else {
        Choqok::UI::PostWidget::checkAnchor(url);
    }
}

void PlurkPostWidget::slotBasePostFetched(Choqok::Account* theAccount, Choqok::Post* post)
{
    if(theAccount == currentAccount() && post && post->postId == currentPost().replyToPostId){
        disconnect( currentAccount()->microblog(), SIGNAL(postFetched(Choqok::Account*,Choqok::Post*)),
                   this, SLOT(slotBasePostFetched(Choqok::Account*,Choqok::Post*)) );
        if(isBasePostShowed)
            return;
        isBasePostShowed = true;
        QString color;
        if( Choqok::AppearanceSettings::isCustomUi() ) {
            color = Choqok::AppearanceSettings::readForeColor().lighter().name();
        } else {
            color = this->palette().dark().color().name();
        }
        QString baseStatusText = "<p style=\"margin-top:10px; margin-bottom:10px; margin-left:20px;\
        margin-right:20px; text-indent:0px\"><span style=\" color:" + color + ";\">";
        baseStatusText += "<b><a href='user://"+ post->author.userName +"'>" +
        post->author.userName + "</a> :</b> ";

        baseStatusText += prepareStatus( post->content ) + "</p>";
        setContent( content().prepend( baseStatusText.replace("<a href","<a style=\"text-decoration:none\" href",Qt::CaseInsensitive) ) );
    }
}

void PlurkPostWidget::slotFavoriteCreated( Choqok::Account* account, Choqok::Post* post )
{
    if ( currentAccount() != account || post->postId != currentPost().postId )
        return;

    Choqok::Post tmp = currentPost();
    tmp.isFavorited = true;
    setCurrentPost( tmp );
    btnFav->setChecked( true );
    btnFav->setIcon( KIcon( "rating" ) );
}

void PlurkPostWidget::slotFavoriteRemoved( Choqok::Account* account, Choqok::Post* post )
{
    if ( currentAccount() != account || post->postId != currentPost().postId )
        return;

    Choqok::Post tmp = currentPost();
    tmp.isFavorited = false;
    setCurrentPost( tmp );
    btnFav->setChecked( false );
    btnFav->setIcon( unFavIcon );
}

void PlurkPostWidget::mousePressEvent(QMouseEvent* ev)
{
    if(!isRead()) {
        setReadWithSignal();
    }

    // The length() won't be the same, even if we call setHtml( originHTML )
    if( _mainWidget->toHtml().length() - originHtml.length() < 100)
    {
        PlurkMicroBlog* microblog = qobject_cast<PlurkMicroBlog*>(currentAccount()->microblog());
        Choqok::Post curPost= currentPost();

        connect( this, SIGNAL(getResponse(Choqok::UI::PostWidget*, Choqok::Account*, const QString&)),
                 microblog, SLOT(slotGetResponse(Choqok::UI::PostWidget*, Choqok::Account*, const QString&)) );
        connect( microblog, SIGNAL(responseReceived(Choqok::UI::PostWidget*, QList<Choqok::Post*>)),
                 this, SLOT(slotResponseReceived(Choqok::UI::PostWidget*, QList<Choqok::Post*>)) );
        emit getResponse( qobject_cast<Choqok::UI::PostWidget *>(this), currentAccount(), curPost.postId );
    }
    else
        _mainWidget->setHtml( originHtml);

    QWidget::mousePressEvent(ev);
}

void PlurkPostWidget::slotResponseReceived( Choqok::UI::PostWidget* thePostWidget, QList<Choqok::Post*> posts)
{
    if( thePostWidget != this)
        return;
    PlurkMicroBlog* microblog = qobject_cast<PlurkMicroBlog*>(currentAccount()->microblog());
    disconnect( this, SIGNAL(getResponse(Choqok::UI::PostWidget*, Choqok::Account*, const QString&)),
        microblog, SLOT(slotGetResponse(Choqok::UI::PostWidget*, Choqok::Account*, const QString&)) );
    disconnect( microblog, SIGNAL(responseReceived(Choqok::UI::PostWidget*, QList<Choqok::Post*>)),
        this, SLOT(slotResponseReceived(Choqok::UI::PostWidget*, QList<Choqok::Post*>)) );
    QString str("");
    const QString responseText ( "<table height=\"100%\" width=\"100%\"><tr><td rowspan=\"2\"\
        width=\"48\">%1</td><td width=\"5\"><!-- EMPTY HAHA --></td><td dir=\"%4\"><p>%2</p></td></tr><tr><td><!-- EMPTY HAHA --></td><td style=\"font-size:small;\" dir=\"ltr\" align=\"right\" width=\"100%\" valign=\"bottom\">%3</td></tr></table>");

    // remember to delete the posts
    foreach(const Choqok::Post* post, posts)
    {
        QString avatar = "<img src=\"" + QString(post->author.profileImageUrl).replace("http://","img://")
		       + "\" title=\""+ post->author.realName + "\" width=\"48\" height=\"48\" />";
        getAvatar(post->author.profileImageUrl);

        QString sign = "<b><a href='"+ currentAccount()->microblog()->profileUrl( currentAccount(), post->author.userName)
                     + "' title=\"" +post->author.description + "\">" + post->author.userName + "</a> - </b>"
                     + "<a href=\"" + post->link + "\" title=\"" + post->creationDateTime.toString(Qt::DefaultLocaleLongDate) + "\">"
                     + formatDateTime( post->creationDateTime) + "</a>";

        str= "<hr width=80% />" + responseText.arg( avatar, parseImageTag( post->content), sign, "") + str;
//kDebug() << "response: username=" << post->author.userName << " postID=" << post->postId << endl;
    }
    _mainWidget->setHtml( originHtml + str);
}

void PlurkPostWidget::getAvatar( QString url)
{
    QPixmap *pix = Choqok::MediaManager::self()->fetchImage( url, Choqok::MediaManager::Async );
    if(pix)
        slotImageFetched(url, *pix);
    else
        connect( Choqok::MediaManager::self(), SIGNAL(imageFetched(QString,QPixmap)),
                 this, SLOT(slotImageFetched(QString,QPixmap)) );
}

#include "plurkpostwidget.moc"
