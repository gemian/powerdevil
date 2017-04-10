/********************************************************************
Copyright 2016  Martin Gräßlin <mgraesslin@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "powerdevilpowermanagement.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

namespace PowerDevil {

static const QString s_fdoPowerService = QStringLiteral("org.freedesktop.PowerManagement");
static const QString s_fdoPowerPath = QStringLiteral("/org/freedesktop/PowerManagement");

class PowerManagementInstance : public PowerManagement
{
    Q_OBJECT
public:
    explicit PowerManagementInstance() : PowerManagement() {}
};
Q_GLOBAL_STATIC(PowerManagementInstance, s_instance)

class PowerManagement::Private
{
public:
    Private(PowerManagement *q);
    void update();
    void setCanSuspend(bool set);
    void setCanHibernate(bool set);
    void setCanHybridSuspend(bool set);

    bool serviceRegistered;
    bool canSuspend;
    bool canHibernate;
    bool canHybridSuspend;
    QScopedPointer<QDBusServiceWatcher> fdoPowerServiceWatcher;

private:
    void updateProperty(const QString &dbusName, void (Private::*setter)(bool));
    PowerManagement *q;
};

PowerManagement::Private::Private(PowerManagement *q)
    : serviceRegistered(false)
    , canSuspend(false)
    , canHibernate(false)
    , canHybridSuspend(false)
    , fdoPowerServiceWatcher(new QDBusServiceWatcher(s_fdoPowerService, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration))
    , q(q)
{
}

void PowerManagement::Private::update()
{
    serviceRegistered = true;
    updateProperty(QStringLiteral("CanSuspend"), &Private::setCanSuspend);
    updateProperty(QStringLiteral("CanHibernate"), &Private::setCanHibernate);
    updateProperty(QStringLiteral("CanHybridSuspend"), &Private::setCanHybridSuspend);
}

void PowerManagement::Private::updateProperty(const QString &dbusName, void (Private::*setter)(bool))
{
    QDBusMessage message = QDBusMessage::createMethodCall(s_fdoPowerService,
                                                          s_fdoPowerPath,
                                                          s_fdoPowerService,
                                                          dbusName);
    QDBusPendingReply<bool> reply = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(reply, q);
    QObject::connect(callWatcher, &QDBusPendingCallWatcher::finished, q,
        [this, setter](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<bool> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                return;
            }
            ((this)->*setter)(reply.value());
        }
    );

}

void PowerManagement::Private::setCanHibernate(bool set)
{
    if (canHibernate == set) {
        return;
    }
    canHibernate = set;
    q->canHibernateChanged();
}

void PowerManagement::Private::setCanSuspend(bool set)
{
    if (canSuspend == set) {
        return;
    }
    canSuspend = set;
    q->canSuspendChanged();
}

void PowerManagement::Private::setCanHybridSuspend(bool set)
{
    if (canHybridSuspend == set) {
        return;
    }
    canHybridSuspend = set;
    q->canHybridSuspendChanged();
}

PowerManagement *PowerManagement::instance()
{
    return s_instance;
}

PowerManagement::PowerManagement()
    : QObject()
    , d(new Private(this))
{
    connect(d->fdoPowerServiceWatcher.data(), &QDBusServiceWatcher::serviceRegistered, this, [this] { d->update(); });
    connect(d->fdoPowerServiceWatcher.data(), &QDBusServiceWatcher::serviceUnregistered, this,
        [this] {
            d->serviceRegistered = false;
            d->setCanSuspend(false);
            d->setCanHibernate(false);
            d->setCanHybridSuspend(false);
        }
    );

    // check whether the service is registered
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("/"),
                                                          QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("ListNames"));
    QDBusPendingReply<QStringList> async = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QStringList> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                return;
            }
            if (reply.value().contains(s_fdoPowerService)) {
                d->update();
            }
        }
    );
}

PowerManagement::~PowerManagement()
{
}

void PowerManagement::suspend()
{
    if (!d->serviceRegistered) {
        return;
    }
    if (!d->canSuspend) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(s_fdoPowerService,
                                                          s_fdoPowerPath,
                                                          s_fdoPowerService,
                                                          QStringLiteral("Suspend"));
    QDBusConnection::sessionBus().asyncCall(message);
}

void PowerManagement::hibernate()
{
    if (!d->serviceRegistered) {
        return;
    }
    if (!d->canHibernate) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(s_fdoPowerService,
                                                          s_fdoPowerPath,
                                                          s_fdoPowerService,
                                                          QStringLiteral("Hibernate"));
    QDBusConnection::sessionBus().asyncCall(message);
}

void PowerManagement::hybridSuspend()
{
    // FIXME: Whether there is a support of this mode?
}

bool PowerManagement::canSuspend() const
{
    return d->canSuspend;
}

bool PowerManagement::canHibernate() const
{
    return d->canHibernate;
}

bool PowerManagement::canHybridSuspend() const
{
    return d->canHybridSuspend;
}

} // namespace PowerDevil

#include "powerdevilpowermanagement.moc"