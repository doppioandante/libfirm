#include "adt/array.h"
#include "assert.h"

#include "kaps.h"
#include "matrix.h"
#include "pbqp_edge.h"
#include "pbqp_edge_t.h"
#include "pbqp_node.h"
#include "pbqp_node_t.h"
#include "pbqp_t.h"

pbqp_edge *alloc_edge(pbqp *pbqp, int src_index, int tgt_index, pbqp_matrix *costs)
{
	int transpose = 0;

	if (tgt_index < src_index) {
		int tmp = src_index;
		src_index = tgt_index;
		tgt_index = tmp;

		transpose = 1;
	}

	pbqp_edge *edge = obstack_alloc(&pbqp->obstack, sizeof(*edge));
	assert(edge);

	pbqp_node *src_node = get_node(pbqp, src_index);
	assert(src_node);

	pbqp_node *tgt_node = get_node(pbqp, tgt_index);
	assert(tgt_node);

	if (transpose) {
		edge->costs = pbqp_matrix_copy_and_transpose(pbqp, costs);
	} else {
		edge->costs = pbqp_matrix_copy(pbqp, costs);
	}

	/*
	 * Connect edge with incident nodes. Since the edge is allocated, we know
	 * that it don't appear in the edge lists of the nodes.
	 */
	ARR_APP1(pbqp_edge *, src_node->edges, edge);
	edge->src = src_node;
	ARR_APP1(pbqp_edge *, tgt_node->edges, edge);
	edge->tgt = tgt_node;

	return edge;
}

void delete_edge(pbqp_edge *edge)
{
	pbqp_node  *src_node;
	pbqp_node  *tgt_node;

	assert(edge);

	src_node = edge->src;
	tgt_node = edge->tgt;
	assert(src_node);
	assert(tgt_node);

	disconnect_edge(src_node, edge);
	disconnect_edge(tgt_node, edge);
}
