/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#ifndef __AIMTRAK_H__
#define __AIMTRAK_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_AXIS 2
#define MAX_BTN 3

#define IDX_X       0
#define IDX_Y       1

#define IDX_LEFT    0
#define IDX_MIDDLE  1
#define IDX_RIGHT   2


struct aimtrak_axis {
    int min;
    int max;
    int value;
};

struct aimtrak_button {
    int pressed;
};

struct aimtrak_state {
    struct aimtrak_button   btns[MAX_BTN];
    struct aimtrak_axis     axis[MAX_AXIS];
};

struct aimtrak_context {
    int                     fd;
    struct aimtrak_state    state;
};


int aimtrak_locate(int product, struct aimtrak_context* dev_p);
int aimtrak_poll(struct aimtrak_context* dev_p);
int aimtrak_open(const char* file, unsigned char* evtype_bitmask, unsigned evtype_size);
void aimtrak_close(struct aimtrak_context* dev_p);


#ifdef __cplusplus
}
#endif

#endif /* __EVENT_AIMTRAK_H__ */
