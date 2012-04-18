/*
    Copyright (C) 2012 Stefan Hajnoczi <stefanha@gmail.com>

    Wahjam is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Wahjam is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Wahjam; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <QCoreApplication>

#include "ninjamsrv.h"
#include "SignalHandler.h"

/* These need to be global since the signal handler has no instance pointer */
static int sigHupFds[2];
static int sigIntFds[2];

SignalHandler::SignalHandler(int argc_, char **argv_)
  : argc(argc_), argv(argv_)
{
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigHupFds) != 0) {
    qFatal("socketpair failed %d", errno);
  }
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigIntFds) != 0) {
    qFatal("socketpair failed %d", errno);
  }

  sigHupNotifier = new QSocketNotifier(sigHupFds[1], QSocketNotifier::Read, this);
  connect(sigHupNotifier, SIGNAL(activated(int)), this, SLOT(onSigHup()));

  sigIntNotifier = new QSocketNotifier(sigIntFds[1], QSocketNotifier::Read, this);
  connect(sigIntNotifier, SIGNAL(activated(int)), this, SLOT(onSigInt()));

  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, handleSigHup);
  signal(SIGINT, handleSigInt);
}

SignalHandler::~SignalHandler()
{
  signal(SIGPIPE, SIG_DFL);
  signal(SIGHUP, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  close(sigHupFds[0]);
  close(sigHupFds[1]);
  close(sigIntFds[0]);
  close(sigIntFds[1]);
}

void SignalHandler::onSigHup()
{
  sigHupNotifier->setEnabled(false);
  char dummy;
  read(sigHupFds[1], &dummy, sizeof(dummy));

  reloadConfig(argc, argv, false);

  sigHupNotifier->setEnabled(true);
}

void SignalHandler::onSigInt()
{
  sigIntNotifier->setEnabled(false);
  char dummy;
  read(sigIntFds[1], &dummy, sizeof(dummy));

  QCoreApplication::quit();

  sigIntNotifier->setEnabled(true);
}

void SignalHandler::handleSigHup(int)
{
  char dummy = 0;
  write(sigHupFds[0], &dummy, sizeof(dummy));
}

void SignalHandler::handleSigInt(int)
{
  char dummy = 0;
  write(sigIntFds[0], &dummy, sizeof(dummy));
}
