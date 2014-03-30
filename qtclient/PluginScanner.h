/*
    Copyright (C) 2013 Stefan Hajnoczi <stefanha@gmail.com>

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

#ifndef _PLUGINSCANNER_H_
#define _PLUGINSCANNER_H_

#include <QString>
#include <QStringList>

/* Effects plugin scanner interface */
class PluginScanner
{
public:
  virtual ~PluginScanner() {}

  /* Return list of found plugin names */
  virtual QStringList scan(const QStringList &searchPaths) const = 0;

  /* Return human-readable plugin name */
  virtual QString displayName(const QString &fullName) const = 0;

  /* Return name of plugin type */
  virtual QString tag() const = 0;
};

#endif /* _PLUGINSCANNER_H_ */
