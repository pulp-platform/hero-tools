/*
 * This file is part of the Snitch device driver.
 *
 * Copyright (C) 2022 ETH Zurich, University of Bologna
 *
 * Author: Cyril Koenig <cykoenig@iis.ee.ethz.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SNITCH_CREATE_H
#define _SNITCH_CREATE_H

#define DRIVER_NAME "eth_snitch_cluster"

#define HBM_0_OFFSET 0x80000000UL

#define SPM_OFFSET   0x70000000UL

#define DTB_OFFSET   SPM_OFFSET + 0
#define DTB_MAGIC    0xd00dfeed
// Arbitrary allow 10KiB to map were the DTB is written
#define DTB_MAP_SIZE 1024 * 10

#endif
