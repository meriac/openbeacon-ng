/***************************************************************
 *
 * OpenBeacon.org - OnAir protocol position estimator
 * accepts a text file with tag sightings per reader.
 *
 * See the following website for already decoded Sputnik data:
 * http://people.openpcd.org/meri/openbeacon/sputnik/data/24c3/
 *
 * Copyright 2009 Milosch Meriac <meriac@bitmanufaktur.de>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __BMMAPHANDLETOITEM_H__
#define __BMMAPHANDLETOITEM_H__

#define HASH_MAP_INDEX_SIZE 0x100000

typedef uint64_t bmHandle;
typedef void (*bmIterationCallback) (void *Item, double timestamp,
									 bool realtime);

typedef struct
{
	bmHandle handle;
	pthread_mutex_t mutex;
	void *data;
} bmHandleMap;

class bmMapHandleToItem
{
  private:
	int m_ItemSize;
	int m_MapIteratePos;
	bmHandleMap m_Map[HASH_MAP_INDEX_SIZE];
	bmHandleMap *m_MapIterate[HASH_MAP_INDEX_SIZE];
	bmHandleMap *HashMap (bmHandle handle);
  public:
	  bmMapHandleToItem (void);
	 ~bmMapHandleToItem (void);
	bool SetItemSize (int ItemSize);
	int GetItemSize (void);
	int GetItemCount (void);
	void *Find (bmHandle handle, pthread_mutex_t ** mutex);
	void *Add (bmHandle handle, pthread_mutex_t ** mutex);
	int IterateLocked (bmIterationCallback Callback, double timestamp,
					   bool realtime);
};

#endif/*__BMMAPHANDLETOITEM_H__*/
