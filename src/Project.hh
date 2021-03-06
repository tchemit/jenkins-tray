/*
** Copyright (C) 2011 Fargier Sylvain <fargier.sylvain@free.fr>
**
** This software is provided 'as-is', without any express or implied
** warranty.  In no event will the authors be held liable for any damages
** arising from the use of this software.
**
** Permission is granted to anyone to use this software for any purpose,
** including commercial applications, and to alter it and redistribute it
** freely, subject to the following restrictions:
**
** 1. The origin of this software must not be misrepresented; you must not
**    claim that you wrote the original software. If you use this software
**    in a product, an acknowledgment in the product documentation would be
**    appreciated but is not required.
** 2. Altered source versions must be plainly marked as such, and must not be
**    misrepresented as being the original software.
** 3. This notice may not be removed or altered from any source distribution.
**
** jenkinsProject.hh
**
**        Created on: Nov 19, 2011
**   Original Author: fargie_s
**
*/

#ifndef __JENKINS_PROJECT_HH__
#define __JENKINS_PROJECT_HH__

#include <QObject>
#include <QString>
#include <QUrl>

namespace jenkins {

class BuildParser;

class Project : public QObject
{
    Q_OBJECT;

public:
    enum State
    {
        SUCCESS = 3,
        UNSTABLE = 2,
        FAILURE = 1,
        UNKNOWN = 0,
    };

    Project(QObject *parent, const QString &name, const QUrl &uri);
    Project(QObject *parent, const QString &name, const QUrl &uri, int m_num);
    ~Project();

    inline int getNum() const
    {
        return m_num;
    }

    inline const QString &getName() const
    {
        return m_name;
    }

    inline State getState() const
    {
        return m_state;
    }

    inline const QUrl &getUrl() const
    {
        return m_uri;
    }

    bool isUpdating() const;

    void update(const QUrl &url);

    inline void update()
    { update(m_uri); }

signals:
    void updated(const Project &);

public slots:
    void buildEvent(int build);

protected slots:
    void stateEvent(Project::State state);
    void networkReplyFinished();

protected:
    BuildParser *m_parser;
    QString m_name;
    QUrl m_uri;
    int m_num;
    State m_state;
};

}

#endif

