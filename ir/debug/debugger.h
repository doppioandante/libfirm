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
 * @brief     Helper function for integrated debug support
 * @author    Michael Beck
 * @date      2005
 */
#ifndef FIRM_DEBUG_DEBUGGER_H
#define FIRM_DEBUG_DEBUGGER_H

#include "firm_types.h"

/** Break into the debugger. */
void firm_debug_break(void);

/**
 * High level function to use from debugger interface
 *
 * Supported commands:
 *  .create nr    break if node nr was created
 *  .help         list all commands
 */
void firm_break(const char *cmd);

/** Creates the debugger tables. */
void firm_init_debugger(void);

void firm_finish_debugger(void);

/**
 * @defgroup external_debug    helper functions for debuggers
 *
 * @{
 */

/**
 * Returns non-zero, if the debug extension is active
 */
int firm_debug_active(void);

/**
 * Return the content of the debug text buffer.
 *
 * To be called from the debugger.
 */
const char *firm_debug_text(void);

/**
 * A gdb helper function to print tarvals.
 */
const char *gdb_tarval_helper(void *tv_object);

/**
 * A gdb helper to print all (new-style-) out edges of a node
 */
const char *gdb_out_edge_helper(const ir_node *node);

/**
 * High level function to use from debugger interface
 *
 * See show_commands() for supported commands.
 */
void firm_debug(const char *cmd);

/**
 * @}
 */

#endif