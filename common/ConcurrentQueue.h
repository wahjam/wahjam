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

#ifndef _CONCURRENTQUEUE_H_
#define _CONCURRENTQUEUE_H_

#include <stdlib.h>
#include <QMutex>
#include <QQueue>

/* Thread-safe single-reader, multiple-write queue */
template <class T>
class ConcurrentQueue
{
public:
  ConcurrentQueue(size_t reserve)
    : discardWrites(false)
  {
    writeQueue.reserve(reserve);
    readQueue.reserve(reserve);
  }

  void setDiscardWrites(bool discard)
  {
    lock.lock();
    discardWrites = discard;
    lock.unlock();
  }

  void write(const T *elems, size_t nelems)
  {
    lock.lock();
    if (!discardWrites) {
      for (size_t i = 0; i < nelems; i++) {
        writeQueue.enqueue(elems[i]);
      }
    }
    lock.unlock();
  }

  /* Instead of reading inidivual elements, the caller grabs the read queue and
   * dequeues elements themselves. */
  QQueue<T> &getReadQueue()
  {
    if (lock.tryLock()) {
      writeQueue.swap(readQueue);
      lock.unlock();
    }
    return readQueue;
  }

private:
  QMutex lock;
  bool discardWrites;
  QQueue<T> writeQueue;
  QQueue<T> readQueue;
};

#endif /* _CONCURRENTQUEUE_H_ */
