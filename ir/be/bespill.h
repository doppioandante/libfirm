/**
 * Author:      Daniel Grund, Sebastian Hack
 * Date:		29.09.2005
 * Copyright:   (c) Universitaet Karlsruhe
 * Licence:     This file protected by GPL -  GNU GENERAL PUBLIC LICENSE.
 */

#ifndef BESPILL_H_
#define BESPILL_H_

#include "firm_types.h"
#include "set.h"
#include "pset.h"
#include "debug.h"

#include "bechordal.h"
#include "be_t.h"

#include "bearch.h"

typedef struct _spill_env_t spill_env_t;

/**
 * Creates a new spill environment.
 *
 * @param chordal
 */
spill_env_t *be_new_spill_env(const be_chordal_env_t *chordal);

/**
 * Deletes a spill environment.
 */
void be_delete_spill_env(spill_env_t *senv);

void be_add_reload(spill_env_t *senv, ir_node *to_spill, ir_node *before);

void be_add_reload_on_edge(spill_env_t *senv, ir_node *to_spill, ir_node *bl, int pos);

void be_insert_spills_reloads(spill_env_t *senv);

/**
 * Marks a phi-node for spilling. So when reloading from this phi-node, not
 * only its value but the whole phi will be spilled.
 * This might place be_Copy nodes in predecessor blocks.
 */
void be_spill_phi(spill_env_t *env, ir_node *node);

/**
 * Places the necessary copies for the spilled phis in the graph
 * This has to be done once before be_insert_spill_reloads, after
 * all phis to spill have been marked with be_spill_phi.
 */
void be_place_copies(spill_env_t *env);

/**
 * Computes the spill offsets for all spill nodes in the irg
 */
void be_compute_spill_offsets(be_chordal_env_t *cenv);

/**
 * Sets the debug module of a spill environment.
 */
DEBUG_ONLY(void be_set_spill_env_dbg_module(spill_env_t *env, firm_dbg_module_t *dbg));

#endif /* BESPILL_H_ */
