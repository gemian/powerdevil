/***************************************************************************
 *   Copyright (C) 2010 by Dario Freddi <drf@kde.org>                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#include "powerdevilaction.h"
#include "powerdevilcore.h"

namespace PowerDevil
{

class Action::Private
{
public:
    Private() {}
    ~Private() {}

    PowerDevil::Core *core;

    QList< int > registeredIdleTimeouts;
};

Action::Action(QObject* parent)
        : QObject(parent)
        , d(new Private)
{
    d->core = qobject_cast<PowerDevil::Core*>(parent);
}

Action::~Action()
{
    unloadAction();
    delete d;
}

void Action::registerIdleTimeout(int msec)
{
    d->registeredIdleTimeouts.append(msec);
    d->core->registerActionTimeout(this, msec);
}

bool Action::unloadAction()
{
    // Remove all registered idle timeouts, if any
    d->core->unregisterActionTimeouts(this);
    d->registeredIdleTimeouts.clear();

    // Ok, let's see if the action has to do something for being unloaded
    return onUnloadAction();
}

bool Action::onUnloadAction()
{
    // Usually nothing has to be done, so let's just happily return true
    return true;
}

BackendInterface* Action::backend()
{
    return d->core->backend();
}

}

#include "powerdevilaction.moc"