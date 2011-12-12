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
 * @brief     Construct and access dominator / post dominator tree.
 * @author    Goetz Lindenmaier, Michael Beck, Rubino Geiss
 * @date      2.2002
 */
#include "config.h"

#include <string.h>

#include "irouts.h"

#include "xmalloc.h"
#include "irgwalk.h"
#include "irdom_t.h"
#include "irgraph_t.h"
#include "irnode_t.h"
#include "ircons_t.h"
#include "array_t.h"
#include "iredges.h"


#define get_dom_info(bl)  (&(bl)->attr.block.dom)
#define get_pdom_info(bl) (&(bl)->attr.block.pdom)

ir_node *get_Block_idom(const ir_node *bl)
{
	assert(is_Block(bl));
	if (get_Block_dom_depth(bl) == -1) {
		/* This block is not reachable from Start */
		ir_graph *irg = get_irn_irg(bl);
		return new_r_Bad(irg, mode_BB);
	}
	return get_dom_info(bl)->idom;
}

void set_Block_idom(ir_node *bl, ir_node *n)
{
	ir_dom_info *bli = get_dom_info(bl);

	assert(is_Block(bl));

	/* Set the immediate dominator of bl to n */
	bli->idom = n;

	/*
	 * If we don't set the root of the dominator tree
	 * Append bl to the dominates queue of n.
	 */
	if (n != NULL) {
		ir_dom_info *ni = get_dom_info(n);

		bli->next = ni->first;
		ni->first = bl;
	}
}

ir_node *get_Block_ipostdom(const ir_node *bl)
{
	assert(is_Block(bl));
	if (get_Block_postdom_depth(bl) == -1) {
		/* This block is not reachable from Start */
		ir_graph *irg = get_irn_irg(bl);
		return new_r_Bad(irg, mode_BB);
	}
	return get_pdom_info(bl)->idom;
}

void set_Block_ipostdom(ir_node *bl, ir_node *n)
{
	ir_dom_info *bli = get_pdom_info(bl);

	assert(is_Block(bl));

	/* Set the immediate post dominator of bl to n */
	bli->idom = n;

	/*
	 * If we don't set the root of the post dominator tree
	 * Append bl to the post dominates queue of n.
	 */
	if (n != NULL) {
		ir_dom_info *ni = get_pdom_info(n);

		bli->next = ni->first;
		ni->first = bl;
	}
}

int get_Block_dom_pre_num(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_dom_info(bl)->pre_num;
}

void set_Block_dom_pre_num(ir_node *bl, int num)
{
	assert(is_Block(bl));
	get_dom_info(bl)->pre_num = num;
}

int get_Block_dom_depth(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_dom_info(bl)->dom_depth;
}

void set_Block_dom_depth(ir_node *bl, int depth)
{
	assert(is_Block(bl));
	get_dom_info(bl)->dom_depth = depth;
}


int get_Block_postdom_pre_num(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_pdom_info(bl)->pre_num;
}

void set_Block_postdom_pre_num(ir_node *bl, int num)
{
	assert(is_Block(bl));
	get_pdom_info(bl)->pre_num = num;
}

int get_Block_postdom_depth(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_pdom_info(bl)->dom_depth;
}

void set_Block_postdom_depth(ir_node *bl, int depth)
{
	assert(is_Block(bl));
	get_pdom_info(bl)->dom_depth = depth;
}

unsigned get_Block_dom_tree_pre_num(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_dom_info(bl)->tree_pre_num;
}

unsigned get_Block_dom_max_subtree_pre_num(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_dom_info(bl)->max_subtree_pre_num;
}

unsigned get_Block_pdom_tree_pre_num(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_pdom_info(bl)->tree_pre_num;
}

unsigned get_Block_pdom_max_subtree_pre_num(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_pdom_info(bl)->max_subtree_pre_num;
}

int block_dominates(const ir_node *a, const ir_node *b)
{
	const ir_dom_info *ai, *bi;

	if (is_Block(a) && is_Block(b)) {
		ai = get_dom_info(a);
		bi = get_dom_info(b);
		return bi->tree_pre_num - ai->tree_pre_num
			<= ai->max_subtree_pre_num - ai->tree_pre_num;
	}

	return 0;
}

int block_strictly_dominates(const ir_node *a, const ir_node *b)
{
	return (a != b) && block_dominates(a, b);
}

ir_node *node_smallest_common_dominator(ir_node *a, ir_node *b)
{
	ir_node *bl_a   = is_Block(a) ? a : get_nodes_block(a);
	ir_node *bl_b   = is_Block(b) ? b : get_nodes_block(b);
	ir_node *dom_bl = NULL;

	/* Check if block of a dominates block of b */
	if (block_dominates(bl_a, bl_b))
		dom_bl = bl_a;
	/* Check if block of b dominates block of a */
	else if (block_dominates(bl_b, bl_a))
		dom_bl = bl_b;
	else {
		/* walk up dominator tree and search for first block dominating a and b */
		while (! dom_bl) {
			bl_a = get_Block_idom(bl_a);

			assert(! is_Bad(bl_a) && "block is dead?");

			if (block_dominates(bl_a, bl_b))
				dom_bl = bl_a;
		}
	}

	return dom_bl;
}

ir_node *node_users_smallest_common_dominator(ir_node *irn, int handle_phi)
{
	int n, j, i = 0, success;
	ir_node **user_blocks, *dom_bl;
	const ir_edge_t *edge;

	assert(! is_Block(irn) && "WRONG USAGE of node_users_smallest_common_dominator");
	assert(edges_activated(get_irn_irg(irn)) && "need edges activated");

	n = get_irn_n_edges(irn);

	/* get array to hold all block of the node users */
	NEW_ARR_A(ir_node *, user_blocks, n);
	foreach_out_edge(irn, edge) {
		ir_node *src = get_edge_src_irn(edge);

		if (is_Phi(src) && handle_phi) {
			/* get the corresponding cfg predecessor block if phi handling requested */
			j  = get_edge_src_pos(edge);
			assert(j >= 0 && "kaputt");
			user_blocks[i++] = get_Block_cfgpred_block(get_nodes_block(src), j);
		}
		else
			user_blocks[i++] = is_Block(src) ? src : get_nodes_block(src);
	}

	assert(i == n && "get_irn_n_edges probably broken");

	/* in case of only one user: return the block of the user */
	if (n == 1)
		return user_blocks[0];

	i = 0;
	/* search the smallest block dominating all user blocks */
	do {
		dom_bl  = node_smallest_common_dominator(user_blocks[i], user_blocks[i + 1]);
		success = 1;

		/* check if this block dominates all remaining blocks as well */
		for (j = i + 2; j < n; j++) {
			if (! block_dominates(dom_bl, user_blocks[j]))
				success = 0;
		}

		if (success)
			break;

		/* inherit the dominator block of the first (i + 1) users */
		user_blocks[++i] = dom_bl;
	} while (i < n - 1);

	assert(success && "no block found dominating all users");

	return dom_bl;
}

/* Get the first node in the list of nodes dominated by a given block. */
ir_node *get_Block_dominated_first(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_dom_info(bl)->first;
}

ir_node *get_Block_dominated_next(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_dom_info(bl)->next;
}

int block_postdominates(const ir_node *a, const ir_node *b)
{
	const ir_dom_info *ai, *bi;

	if (is_Block(a) && is_Block(b)) {
		ai = get_pdom_info(a);
		bi = get_pdom_info(b);
		return bi->tree_pre_num - ai->tree_pre_num
			<= ai->max_subtree_pre_num - ai->tree_pre_num;
	}

	return 0;
}

int block_strictly_postdominates(const ir_node *a, const ir_node *b)
{
	return (a != b) && block_postdominates(a, b);
}

ir_node *get_Block_postdominated_first(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_pdom_info(bl)->first;
}

ir_node *get_Block_postdominated_next(const ir_node *bl)
{
	assert(is_Block(bl));
	return get_pdom_info(bl)->next;
}

void dom_tree_walk(ir_node *bl, irg_walk_func *pre,
		irg_walk_func *post, void *env)
{
	ir_node *p;

	if (pre)
		pre(bl, env);

	dominates_for_each(bl, p) {
		dom_tree_walk(p, pre, post, env);
	}

	if (post)
		post(bl, env);
}

void postdom_tree_walk(ir_node *bl, irg_walk_func *pre,
		irg_walk_func *post, void *env)
{
	ir_node *p;

	if (pre)
		pre(bl, env);

	postdominates_for_each(bl, p) {
		postdom_tree_walk(p, pre, post, env);
	}

	if (post)
		post(bl, env);
}

void dom_tree_walk_irg(ir_graph *irg, irg_walk_func *pre,
		irg_walk_func *post, void *env)
{
	/* The root of the dominator tree should be the Start block. */
	ir_node *root = get_irg_start_block(irg);

	assert(is_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_DOMINANCE)
			&& "The dominators of the irg must be consistent");
	assert(root && "The start block of the graph is NULL?");
	assert(get_dom_info(root)->idom == NULL
			&& "The start node in the graph must be the root of the dominator tree");
	dom_tree_walk(root, pre, post, env);
}

void postdom_tree_walk_irg(ir_graph *irg, irg_walk_func *pre,
		irg_walk_func *post, void *env)
{
	/* The root of the post dominator tree should be the End block. */
	ir_node *root = get_irg_end_block(irg);

	assert(is_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_POSTDOMINANCE)
			&& "The dominators of the irg must be consistent");
	assert(root && "The end block of the graph is NULL?");
	assert(get_pdom_info(root)->idom == NULL
			&& "The End block node in the graph must be the root of the post dominator tree");
	postdom_tree_walk(root, pre, post, env);
}


static void assign_tree_dom_pre_order(ir_node *bl, void *data)
{
	unsigned *num = (unsigned*) data;
	ir_dom_info *bi = get_dom_info(bl);

	bi->tree_pre_num = (*num)++;
}

static void assign_tree_dom_pre_order_max(ir_node *bl, void *data)
{
	ir_dom_info *bi = get_dom_info(bl);
	ir_node *p;
	unsigned max = 0;
	unsigned children = 0;
	(void) data;

	for (p = bi->first; p; p = get_dom_info(p)->next) {
		unsigned max_p = get_dom_info(p)->max_subtree_pre_num;
		max = max > max_p ? max : max_p;
		children++;
	}

	bi->max_subtree_pre_num = children > 0 ? max : bi->tree_pre_num;
	assert(bi->max_subtree_pre_num >= bi->tree_pre_num);
}

static void assign_tree_postdom_pre_order(ir_node *bl, void *data)
{
	unsigned *num = (unsigned*) data;
	ir_dom_info *bi = get_pdom_info(bl);

	bi->tree_pre_num = (*num)++;
}

static void assign_tree_postdom_pre_order_max(ir_node *bl, void *data)
{
	ir_dom_info *bi = get_pdom_info(bl);
	ir_node *p;
	unsigned max = 0;
	unsigned children = 0;
	(void) data;

	for (p = bi->first; p; p = get_pdom_info(p)->next) {
		unsigned max_p = get_pdom_info(p)->max_subtree_pre_num;
		max = max > max_p ? max : max_p;
		children++;
	}

	bi->max_subtree_pre_num = children > 0 ? max : bi->tree_pre_num;
	assert(bi->max_subtree_pre_num >= bi->tree_pre_num);
}

/**
 * count the number of blocks and clears the post dominance info
 */
static void count_and_init_blocks_pdom(ir_node *bl, void *env)
{
	int *n_blocks = (int *) env;

	(*n_blocks) ++;

	memset(get_pdom_info(bl), 0, sizeof(ir_dom_info));
	set_Block_ipostdom(bl, NULL);
	set_Block_postdom_pre_num(bl, -1);
	set_Block_postdom_depth(bl, -1);
}

/** temporary type used while constructing the dominator / post dominator tree. */
typedef struct tmp_dom_info {
	ir_node *block;               /**< backlink */

	struct tmp_dom_info *semi;    /**< semidominator */
	struct tmp_dom_info *parent;
	struct tmp_dom_info *label;   /**< used for LINK and EVAL */
	struct tmp_dom_info *ancestor;/**< used for LINK and EVAL */
	struct tmp_dom_info *dom;     /**< After step 3, if the semidominator of w is
	                                   its immediate dominator, then w->dom is the
	                                   immediate dominator of w.  Otherwise w->dom
	                                   is a vertex v whose number is smaller than
	                                   w and whose immediate dominator is also w's
	                                   immediate dominator. After step 4, w->dom
	                                   is the immediate dominator of w.  */
	struct tmp_dom_info *bucket;  /**< set of vertices with same semidominator */
} tmp_dom_info;

/** Struct to pass info through walker. */
typedef struct {
	tmp_dom_info *d;
	int used;
} dom_env;


/**
 * Walks Blocks along the out data structure.  If recursion started with
 * Start block misses control dead blocks.
 */
static void init_tmp_dom_info(ir_node *bl, tmp_dom_info *parent,
                              tmp_dom_info *tdi_list, int *used, int n_blocks)
{
	tmp_dom_info *tdi;
	int i;

	assert(is_Block(bl));
	if (Block_block_visited(bl))
	  return;
	mark_Block_block_visited(bl);
	set_Block_dom_pre_num(bl, *used);

	assert(*used < n_blocks);
	tdi = &tdi_list[*used];
	++(*used);

	tdi->semi     = tdi;
	tdi->label    = tdi;
	tdi->ancestor = NULL;
	tdi->bucket   = NULL;
	tdi->parent   = parent;
	tdi->block    = bl;

	/* Iterate */
	for (i = get_Block_n_cfg_outs_ka(bl) - 1; i >= 0; --i) {
		ir_node *pred = get_Block_cfg_out_ka(bl, i);
		/* can happen for half-optimized dead code (I've seen this in student
		   projects */
		if (!is_Block(pred))
			continue;

		init_tmp_dom_info(pred, tdi, tdi_list, used, n_blocks);
	}
}

/**
 * Walks Blocks along the control flow.  If recursion started with
 * End block misses blocks in endless loops.
 */
static void init_tmp_pdom_info(ir_node *bl, tmp_dom_info *parent,
                               tmp_dom_info *tdi_list, int* used, int n_blocks)
{
	tmp_dom_info *tdi;
	int i;

	assert(is_Block(bl));
	if (get_irg_block_visited(current_ir_graph) == get_Block_block_visited(bl))
	  return;
	mark_Block_block_visited(bl);
	set_Block_postdom_pre_num(bl, *used);

	assert(*used < n_blocks);
	tdi = &tdi_list[*used];
	++(*used);

	tdi->semi = tdi;
	tdi->label = tdi;
	tdi->ancestor = NULL;
	tdi->bucket = NULL;
	tdi->parent = parent;
	tdi->block = bl;

	/* Iterate */
	for (i = get_Block_n_cfgpreds(bl) - 1; i >= 0; --i) {
		ir_node *pred = get_Block_cfgpred_block(bl, i);
		if (is_Bad(pred))
			continue;
		assert(is_Block(pred));
		init_tmp_pdom_info(pred, tdi, tdi_list, used, n_blocks);
	}

	/* Handle keep-alives. Note that the preprocessing
	   in init_construction() had already killed all
	   phantom keep-alive edges. All remaining block keep-alives
	   are really edges to endless loops.
	 */
	if (bl == get_irg_end_block(current_ir_graph)) {
		ir_node *end = get_irg_end(current_ir_graph);

		for (i = get_irn_arity(end) - 1; i >= 0; --i) {
			ir_node *pred = get_irn_n(end, i);

			if (is_Block(pred))
				init_tmp_pdom_info(pred, tdi, tdi_list, used, n_blocks);
		}
	}
}

static void dom_compress(tmp_dom_info *v)
{
	assert (v->ancestor);
	if (v->ancestor->ancestor) {
		dom_compress (v->ancestor);
		if (v->ancestor->label->semi < v->label->semi) {
			v->label = v->ancestor->label;
		}
		v->ancestor = v->ancestor->ancestor;
	}
}

/**
 * if V is a root, return v, else return the vertex u, not being the
 * root, with minimum u->semi on the path from v to its root.
 */
inline static tmp_dom_info *dom_eval(tmp_dom_info *v)
{
	if (!v->ancestor) return v;
	dom_compress (v);
	return v->label;
}

/** make V W's ancestor */
inline static void dom_link(tmp_dom_info *v, tmp_dom_info *w)
{
	w->ancestor = v;
}

/**
 * Walker: count the number of blocks and clears the dominance info
 */
static void count_and_init_blocks_dom(ir_node *bl, void *env)
{
	int *n_blocks = (int *) env;

	(*n_blocks) ++;

	memset(get_dom_info(bl), 0, sizeof(ir_dom_info));
	set_Block_idom(bl, NULL);
	set_Block_dom_pre_num(bl, -1);
	set_Block_dom_depth(bl, -1);
}

void compute_doms(ir_graph *irg)
{
	ir_graph *rem = current_ir_graph;
	int n_blocks, used, i, j;
	tmp_dom_info *tdi_list;   /* Ein Golf? */

	current_ir_graph = irg;

	/* Update graph state */
	assert(get_irg_phase_state(irg) != phase_building);

	/* Count the number of blocks in the graph. */
	n_blocks = 0;
	irg_block_walk_graph(irg, count_and_init_blocks_dom, NULL, &n_blocks);

	/* Memory for temporary information. */
	tdi_list = XMALLOCNZ(tmp_dom_info, n_blocks);

	/* We need the out data structure. */
	assure_irg_outs(irg);

	/* this with a standard walker as passing the parent to the sons isn't
	   simple. */
	used = 0;
	inc_irg_block_visited(irg);
	init_tmp_dom_info(get_irg_start_block(irg), NULL, tdi_list, &used, n_blocks);
	/* If not all blocks are reachable from Start by out edges this assertion
	   fails.
	   assert(used == n_blocks && "Precondition for dom construction violated"); */
	assert(used <= n_blocks && "Precondition for dom construction violated");
	n_blocks = used;


	for (i = n_blocks-1; i > 0; i--) {  /* Don't iterate the root, it's done. */
		tmp_dom_info *w     = &tdi_list[i];
		ir_node      *block = w->block;
		tmp_dom_info *v;
		int           irn_arity;

		/* Step 2 */
		irn_arity = get_irn_arity(block);
		for (j = 0; j < irn_arity;  j++) {
			ir_node *pred       = get_Block_cfgpred(block, j);
			ir_node *pred_block = get_nodes_block(pred);
			tmp_dom_info *u;

			if (is_Bad(pred) || (get_Block_dom_pre_num (pred_block) == -1))
				continue;    /* unreachable */

			u = dom_eval (&tdi_list[get_Block_dom_pre_num(pred_block)]);
			if (u->semi < w->semi)
				w->semi = u->semi;
		}

		/* handle keep-alives if we are at the end block */
		if (block == get_irg_end_block(irg)) {
			ir_node *end = get_irg_end(irg);

			irn_arity = get_irn_arity(end);
			for (j = 0; j < irn_arity;  j++) {
				ir_node *pred = get_irn_n(end, j);
				tmp_dom_info *u;

				if (!is_Block(pred) || get_Block_dom_pre_num(pred) == -1)
					continue;   /* unreachable */

				u = dom_eval (&tdi_list[get_Block_dom_pre_num(pred)]);
				if (u->semi < w->semi)
					w->semi = u->semi;
			}
		}

		/* Add w to w->semi's bucket.  w is in exactly one bucket, so
		   buckets can been implemented as linked lists. */
		w->bucket = w->semi->bucket;
		w->semi->bucket = w;

		dom_link (w->parent, w);

		/* Step 3 */
		while (w->parent->bucket) {
			tmp_dom_info *u;
			v = w->parent->bucket;
			/* remove v from w->parent->bucket */
			w->parent->bucket = v->bucket;
			v->bucket = NULL;

			u = dom_eval (v);
			if (u->semi < v->semi)
				v->dom = u;
			else
				v->dom = w->parent;
		}
	}
	/* Step 4 */
	tdi_list[0].dom = NULL;
	set_Block_idom(tdi_list[0].block, NULL);
	set_Block_dom_depth(tdi_list[0].block, 1);
	for (i = 1; i < n_blocks;  i++) {
		tmp_dom_info *w = &tdi_list[i];
		int depth;

		if (! w->dom)
			continue; /* control dead */

		if (w->dom != w->semi)
			w->dom = w->dom->dom;
		set_Block_idom(w->block, w->dom->block);

		/* blocks dominated by dead one's are still dead */
		depth = get_Block_dom_depth(w->dom->block);
		if (depth > 0)
			++depth;
		set_Block_dom_depth(w->block, depth);
	}

	/* clean up */
	free(tdi_list);

	/* Do a walk over the tree and assign the tree pre orders. */
	{
		unsigned tree_pre_order = 0;
		dom_tree_walk(get_irg_start_block(irg), assign_tree_dom_pre_order,
		                  assign_tree_dom_pre_order_max, &tree_pre_order);
	}
	current_ir_graph = rem;
	set_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_DOMINANCE);
}

void assure_doms(ir_graph *irg)
{
	if (! is_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_DOMINANCE))
		compute_doms(irg);
}

void free_dom(ir_graph *irg)
{
	/* Update graph state */
	assert(get_irg_phase_state(irg) != phase_building);
	clear_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_DOMINANCE);

	/* With the implementation right now there is nothing to free,
	   but better call it anyways... */
}

void compute_postdoms(ir_graph *irg)
{
	ir_graph *rem = current_ir_graph;
	int n_blocks, used, i, j;
	tmp_dom_info *tdi_list;

	current_ir_graph = irg;

	/* Update graph state */
	assert(get_irg_phase_state(irg) != phase_building);

	/* Count the number of blocks in the graph. */
	n_blocks = 0;
	irg_block_walk_graph(irg, count_and_init_blocks_pdom, NULL, &n_blocks);

	/* Memory for temporary information. */
	tdi_list = XMALLOCNZ(tmp_dom_info, n_blocks);

	/* We need the out data structure. */
	assure_irg_outs(irg);

	/* this with a standard walker as passing the parent to the sons isn't
	   simple. */
	used = 0;
	inc_irg_block_visited(irg);
	init_tmp_pdom_info(get_irg_end_block(irg), NULL, tdi_list, &used, n_blocks);
	/* If not all blocks are reachable from End by cfg edges this assertion
	   fails.
	   assert(used == n_blocks && "Precondition for dom construction violated"); */
	n_blocks = used;


	for (i = n_blocks-1; i > 0; i--) {  /* Don't iterate the root, it's done. */
		int irn_arity;
		tmp_dom_info *w = &tdi_list[i];
		tmp_dom_info *v;

		/* Step 2 */
		irn_arity = get_Block_n_cfg_outs_ka(w->block);
		for (j = 0;  j < irn_arity;  j++) {
			ir_node *succ = get_Block_cfg_out_ka(w->block, j);
			tmp_dom_info *u;

			if (get_Block_postdom_pre_num (succ) == -1)
				continue;    /* endless-loop */

			u = dom_eval (&tdi_list[get_Block_postdom_pre_num(succ)]);
			if (u->semi < w->semi) w->semi = u->semi;
		}
		/* Add w to w->semi's bucket.  w is in exactly one bucket, so
		   buckets can be implemented as linked lists. */
		w->bucket = w->semi->bucket;
		w->semi->bucket = w;

		dom_link (w->parent, w);

		/* Step 3 */
		while (w->parent->bucket) {
			tmp_dom_info *u;
			v = w->parent->bucket;
			/* remove v from w->parent->bucket */
			w->parent->bucket = v->bucket;
			v->bucket = NULL;

			u = dom_eval(v);
			if (u->semi < v->semi)
				v->dom = u;
			else
				v->dom = w->parent;
		}
	}
	/* Step 4 */
	tdi_list[0].dom = NULL;
	set_Block_ipostdom(tdi_list[0].block, NULL);
	set_Block_postdom_depth(tdi_list[0].block, 1);
	for (i = 1;  i < n_blocks;  i++) {
		tmp_dom_info *w = &tdi_list[i];

		if (w->dom != w->semi) w->dom = w->dom->dom;
		set_Block_ipostdom(w->block, w->dom->block);
		set_Block_postdom_depth(w->block, get_Block_postdom_depth(w->dom->block) + 1);
	}

	/* clean up */
	free(tdi_list);

	/* Do a walk over the tree and assign the tree pre orders. */
	{
		unsigned tree_pre_order = 0;
		postdom_tree_walk(get_irg_end_block(irg), assign_tree_postdom_pre_order,
			assign_tree_postdom_pre_order_max, &tree_pre_order);
	}
	current_ir_graph = rem;
	set_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_POSTDOMINANCE);
}

void assure_postdoms(ir_graph *irg)
{
	if (! is_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_POSTDOMINANCE))
		compute_postdoms(irg);
}

void free_postdom(ir_graph *irg)
{
	/* Update graph state */
	assert(get_irg_phase_state(irg) != phase_building);
	clear_irg_state(irg, IR_GRAPH_STATE_CONSISTENT_POSTDOMINANCE);

	/* With the implementation right now there is nothing to free,
	   but better call it anyways... */
}
