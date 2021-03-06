/* tracetool - a framework for tracing the execution of C++ programs
 * Copyright 2010-2016 froglogic GmbH
 *
 * This file is part of tracetool.
 *
 * tracetool is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * tracetool is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with tracetool.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "settings.h"

#include "entryfilter.h"
#include "columnsinfo.h"
#include "../hooklib/tracelib_config.h"

#include <cassert>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStringList>

const char companyName[] = "froglogic";
const char productName[] = "appstalker";
const char sessionGroup[] = "Session";
const char databaseGroup[] = "Database";
const char serverGroup[] = "Server";
const char configGroup[] = "Configuration";
const char displayGroup[] = "Display";

const int defaultSoftLimit = 1500000;
const int defaultHardLimit = defaultSoftLimit + 500000;

static QString defaultFile()
{
    return QDir::temp().absoluteFilePath("demo.trace");
}

Settings::Settings()
{
    m_entryFilter = new EntryFilter();
    registerRestorable("Filter", m_entryFilter);
    m_columnsInfo = new ColumnsInfo();
    registerRestorable("ColumnsInfo", m_columnsInfo);

    if (!load()) {
        qWarning() << "Failed to load settings";
    }
}

Settings::~Settings()
{
    delete m_entryFilter;
    delete m_columnsInfo;
}

bool Settings::save() const
{
    QSettings qs(QSettings::IniFormat, QSettings::UserScope,
                 companyName, productName);

    // [Configuration]
    qs.beginGroup(configGroup);
    qs.setValue("Files", m_configFiles);
    qs.endGroup();

    // [Server]
    qs.beginGroup(serverGroup);
    qs.setValue("GUIPort", m_serverGUIPort);
    qs.setValue("TracePort", m_serverTracePort);
    qs.setValue("StartAutomatically", m_serverStartedAutomatically);
    qs.setValue("OutputFile", m_databaseFile);
    qs.endGroup();

    // [Display]
    qs.beginGroup(displayGroup);
    qs.setValue("Font", m_font.toString());
    qs.endGroup();

    qs.sync();
    return qs.status() == QSettings::NoError;
}

bool Settings::load()
{
    QSettings qs(QSettings::IniFormat, QSettings::UserScope,
                 companyName, productName);

    // [Configuration]
    qs.beginGroup(configGroup);
    m_configFiles = qs.value("Files", QStringList()).toStringList();
    qs.endGroup();

    // [Server]
    qs.beginGroup(serverGroup);
    m_serverTracePort = qs.value("TracePort", TRACELIB_DEFAULT_PORT).toInt();
    m_serverGUIPort = qs.value("GUIPort", m_serverTracePort + 1).toInt();
    m_serverStartedAutomatically = qs.value("StartAutomatically", true).toBool();
    m_databaseFile = qs.value("OutputFile", defaultFile()).toString();
    qs.endGroup();

    // [Display]
    qs.beginGroup(displayGroup);
    m_font.fromString(qs.value("Font", QApplication::font().toString()).toString());
    qs.endGroup();

    return qs.status() == QSettings::NoError;

}

void Settings::registerRestorable(const QString &name, RestorableObject *r)
{
    assert(!m_restorables.contains(name));
    m_restorables.insert(name, r);
}

bool Settings::saveSession()
{
    QSettings qs(QSettings::IniFormat, QSettings::UserScope,
                 companyName, productName);

    qs.beginGroup(sessionGroup);
    QMapIterator<QString, RestorableObject*> it(m_restorables);
    while (it.hasNext()) {
        it.next();
        QString n = it.key();
        RestorableObject *r = it.value();
        qs.setValue(n, r->sessionState());
    }
    qs.endGroup();

    qs.sync();
    return qs.status() == QSettings::NoError;

}

bool Settings::restoreSession() const
{
    QSettings qs(QSettings::IniFormat, QSettings::UserScope,
                 companyName, productName);

    qs.beginGroup(sessionGroup);
    QMapIterator<QString, RestorableObject*> it(m_restorables);
    while (it.hasNext()) {
        it.next();
        QString n = it.key();
        RestorableObject *r = it.value();
        QVariant state = qs.value(n);
        if (state.isValid()) {
            if (!r->restoreSessionState(state)) {
                qWarning() << "Failed to restore session state for object"
                           << n;
            }
        }
    }

    qs.endGroup();

    return qs.status() == QSettings::NoError;
}

void Settings::addConfigurationFile(const QString &fileName)
{
    if (m_configFiles.contains(fileName))
        return;
    m_configFiles.prepend(fileName);
    while (m_configFiles.size() > 10)
        m_configFiles.removeLast();
}
