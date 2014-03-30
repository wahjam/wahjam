/*
    Copyright (C) 2014 Stefan Hajnoczi <stefanha@gmail.com>

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

#include "EffectPlugin.h"

EffectPlugin::EffectPlugin()
  : wetDryMix(1)
{
}

float EffectPlugin::getWetDryMix() const
{
  return wetDryMix;
}

void EffectPlugin::setWetDryMix(float mix)
{
  wetDryMix = qMax(0.0f, qMin(1.0f, mix));
}
