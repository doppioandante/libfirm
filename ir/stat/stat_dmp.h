/*
 * Copyright (C) 1995-2008 University of Karlsruhe.  All right reserved.
 *
 * This file is part of libFirm.
 *
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 *
 * Licensees holding valid libFirm Professional Edition licenses may use
 * this file in accordance with the libFirm Commercial License.
 * Agreement provided with the Software.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.
 */

/**
 * @file
 * @brief   Statistics for Firm. Dumping.
 * @author  Michael Beck
 */
#ifndef FIRM_STAT_STAT_DMP_H
#define FIRM_STAT_STAT_DMP_H

#include "firmstat_t.h"

/**
 * The simple human readable dumper.
 */
extern const dumper_t simple_dumper;

/**
 * the comma separated list dumper
 *
 * @note Limited capabilities, mostly for the Firm paper
 */
extern const dumper_t csv_dumper;

#endif /* FIRM_STAT_STAT_DMP_H */