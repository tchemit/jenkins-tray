/*
** Copyright (C) 2012 Fargier Sylvain <fargier.sylvain@free.fr>
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
** test_simple.cpp
**
**        Created on: Jan 07, 2012
**   Original Author: fargie_s
**
*/

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <QApplication>

#include "RSSParser.hh"

#include "test_helpers.hh"
#include "stub_Tray.hh"
#include "stub_Settings.hh"
#include "stub_Jenkins.hh"

using namespace jenkins;

class simpleTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(simpleTest);
    CPPUNIT_TEST(simple);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp()
    {
        m_app = new QApplication(0, NULL);
        m_jenkins = new JenkinsServerStub(m_app);

        Q_INIT_RESOURCE(Tray);
        m_tray = new TrayStub();
    }

    void tearDown()
    {
        delete m_jenkins;
        delete m_tray;
        delete m_app;
    }

protected:
    JenkinsServerStub *m_jenkins;
    QApplication      *m_app;
    TrayStub          *m_tray;

    bool wait(int timeout)
    {
        QTimer timer;

        if (timeout > 0)
        {
            QObject::connect(&timer, SIGNAL(timeout()), m_app, SLOT(quit()));
            timer.setInterval(timeout);
            timer.setSingleShot(true);
            timer.start();
        }
        m_app->exec();
        return !timer.isActive();
    }

    void simple()
    {
        m_jenkins->add(QString("/simple/rssLatest"),
                getExePath() + "/data/simple_rss.xml");

        SettingsStub &stub = m_tray->getSettingsStub();
        stub.setUrl(m_jenkins->baseUri() + "/simple");
        stub.setInterval(1);

        m_tray->start();

        QObject::connect(m_tray->getParser(), SIGNAL(finished()), m_app,
                SLOT(quit()));
        CPPUNIT_ASSERT(!wait(3000));
        QObject::disconnect(m_tray->getParser(), SIGNAL(finished()), m_app,
                SLOT(quit()));

        QMap<QString, Project *> &projs = m_tray->getProjects();
        CPPUNIT_ASSERT_EQUAL(4, projs.size());

        foreach(Project *p, projs)
        {
            CPPUNIT_ASSERT_EQUAL(Project::UNKNOWN, p->getState());
        }

        m_jenkins->add(QString("/job/jenkins-target/13/api/xml"),
                getExePath() + "/data/simple_target_13.xml");

        QMap<QString, Project *>::const_iterator it = projs.find("jenkins-target");
        CPPUNIT_ASSERT(it != projs.end());
        QObject::connect(it.value(), SIGNAL(updated(const Project &)),
                m_app, SLOT(quit()));
        CPPUNIT_ASSERT(!wait(3000));

        it = projs.find("jenkins-target");
        CPPUNIT_ASSERT(it != projs.end());
        CPPUNIT_ASSERT_EQUAL(Project::SUCCESS, it.value()->getState());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(simpleTest);
