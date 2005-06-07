/**
 * @file   benode_t.h
 * @date   17.05.2005
 * @author Sebastian Hack
 *
 * Backend node support.
 *
 * Copyright (C) 2005 Universitaet Karlsruhe
 * Released under the GPL
 */

#ifndef _BENODE_T_H
#define _BENODE_T_H

#include "pmap.h"

#include "irmode.h"
#include "irnode.h"

#include "bearch.h"

struct _be_node_factory_t {
  const arch_isa_if_t *isa;

  struct obstack      obst;
  set                 *ops;
  pmap                *irn_op_map;
  pmap                *reg_req_map;

  arch_irn_handler_t  handler;
  arch_irn_ops_t      irn_ops;
};

typedef struct _be_node_factory_t 			be_node_factory_t;

be_node_factory_t *be_node_factory_init(be_node_factory_t *factory,
    const arch_isa_if_t *isa);

const arch_irn_handler_t *be_node_get_irn_handler(const be_node_factory_t *f);

ir_node *new_Spill(const be_node_factory_t *factory,
    const arch_register_class_t *cls,
    ir_graph *irg, ir_node *bl, ir_node *node_to_spill);

ir_node *new_Reload(const be_node_factory_t *factory,
    const arch_register_class_t *cls,
    ir_graph *irg, ir_node *bl, ir_node *spill_node);

ir_node *new_Perm(const be_node_factory_t *factory,
    const arch_register_class_t *cls,
    ir_graph *irg, ir_node *bl, int arity, ir_node **in);

ir_node *new_Copy(const be_node_factory_t *factory,
    const arch_register_class_t *cls,
    ir_graph *irg, ir_node *block, ir_node *in);

ir_node *be_spill(const be_node_factory_t *factory, const arch_env_t *env, ir_node *irn);
ir_node *be_reload(const be_node_factory_t *factory, const arch_env_t *env, ir_node *irn);

int is_Spill(const be_node_factory_t *f, const ir_node *irn);

ir_node *get_Reload_Spill(ir_node *reload);

void insert_perm(const be_node_factory_t *factory,
    const arch_register_class_t *reg_class,
    ir_node *in_front_of);

#endif /* _BENODE_T_H */
