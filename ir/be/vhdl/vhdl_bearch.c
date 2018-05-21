/*
 * This file is part of libFirm.
 * Copyright (C) 2019 University of Karlsruhe.
 */

#include "vhdl_bearch_t.h"

#include "be_t.h"
#include "beemitter.h"
#include "bemodule.h"
#include "gen_vhdl_new_nodes.h"
#include "gen_vhdl_regalloc_if.h"
#include "irarch.h"
#include "irprog_t.h"
#include "target_t.h"
#include "vhdl_bemain.h"
#include "vhdl_emitter.h"
#include "vhdl_lower.h"
#include "vhdl_modes.h"
#include "vhdl_transform.h"

static int vhdl_ifconv(ir_node const *const sel, ir_node const *const mux_false,
                       ir_node const *mux_true)
{
	(void) sel;
	(void) mux_false;
	(void) mux_true;
	return true;
}

void vhdl_init(void)
{
	vhdl_init_modes();
	vhdl_create_opcodes();
	vhdl_register_init();

	ir_target.experimental = "the VHDL backend is highly experimental and unfinished";
	ir_target.allow_ifconv = vhdl_ifconv;
	ir_target.float_int_overflow = ir_overflow_indefinite;
}

void vhdl_finish(void)
{
	vhdl_free_opcodes();
}

static void vhdl_select_instructions(ir_graph *const irg)
{
	be_timer_push(T_CODEGEN);
	vhdl_transform_graph(irg);
	be_timer_pop(T_CODEGEN);
	be_dump(DUMP_BE, irg, "code-selection");
}

static void vhdl_lower_for_target(ir_graph *irg)
{
	//ir_arch_lower(&vhdl_arch_dep);
	lower_for_vhdl(irg);
}

void vhdl_generate_code(char const *const cup_name)
{

	vhdl_be_begin(cup_name);


	foreach_irp_irg(i, irg) {
		if (!(mtp_special_instruction & get_entity_additional_properties(get_irg_entity(irg)))) {
			continue;
		}
		opt_if_conv_cb(irg, vhdl_ifconv);
		be_dump(DUMP_BE, irg, "if-conv");

		const char *entity_name = get_entity_ld_name(get_irg_entity(irg));
		char filename[1024];
		snprintf(filename, sizeof filename, "%s%s", entity_name, ".vhd");
		FILE *out = fopen(filename, "w");
		be_emit_init(out);

		vhdl_lower_for_target(irg);
		be_after_transform(irg, "lower-arch-dep");


		vhdl_select_instructions(irg);

		vhdl_emit_function(irg);
		vhdl_be_step_last(irg);
		be_emit_exit();
	}

	vhdl_be_finish();
}

BE_REGISTER_MODULE_CONSTRUCTOR(be_init_arch_vhdl)

void be_init_arch_vhdl(void)
{
}