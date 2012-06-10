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

#ifndef _SIGNALHANDLER_H_
#define _SIGNALHANDLER_H_

#include <QSocketNotifier>

/*
 * Adapter for POSIX signal handlers to Qt event loop
 *
 * Note there can only be a single instance of this class at any time because
 * the signal disposition for the entire process is affected.
 */
class SignalHandler : public QObject
{
  Q_OBJECT

public:
  SignalHandler(int argc_, char **argv_);
  ~SignalHandler();

  void setup();
  void cleanup();

private slots:
  void onSigHup();
  void onSigInt();

private:
  static void handleSigHup(int);
  static void handleSigInt(int);

  int argc;
  char **argv;
  QSocketNotifier *sigHupNotifier;
  QSocketNotifier *sigIntNotifier;
};

#endif /* _SIGNALHANDLER_H_ */
