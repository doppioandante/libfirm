#!/usr/bin/perl -w

# This script generates the C code which creates the irop's and
# their coresponding node constructors for all operations in a given spec
# so they can be used as normal firm nodes.
# Creation: 2005/10/19
# $Id$

use strict;
use Data::Dumper;

my $specfile   = $ARGV[0];
my $target_dir = $ARGV[1];
my $state      = 1;
my $cur_op     = "";
my $line_nr    = 0;

our $arch;
our $additional_opcodes;
our %nodes;

# include spec file

my $return;

no strict "subs";
unless ($return = do $specfile) {
	warn "couldn't parse $specfile: $@" if $@;
	warn "couldn't do $specfile: $!"    unless defined $return;
	warn "couldn't run $specfile"       unless $return;
}
use strict "subs";

my $target_c = $target_dir."/gen_".$arch."_new_nodes.c.inl";
my $target_h = $target_dir."/gen_".$arch."_new_nodes.h";

#print Dumper(%nodes);
#print Dumper(%operands);

# create c code file from specs

my @obst_opvar;       # stack for the "ir_op *op_<arch>_<op-name> = NULL;" statements
my @obst_get_opvar;   # stack for the get_op_<arch>_<op-name>() functions
my @obst_constructor; # stack for node constructor functions
my @obst_new_irop;    # stack for the new_ir_op calls
my @obst_enum_op;     # stack for creating the <arch>_opcode enum
my @obst_header;      # stack for function prototypes
my @obst_is_archirn;  # stack for the is_$arch_irn() function
my @obst_cmp_attr;    # stack for the compare attribute functions
my @obst_proj;        # stack for the pn_ numbers
my $orig_op;
my $arity;
my $cmp_attr_func;
my $temp;
my $n_opcodes = 0;    # number of opcodes

# for registering additional opcodes
$n_opcodes += $additional_opcodes if (defined($additional_opcodes));

push(@obst_header, "void ".$arch."_create_opcodes(void);\n");

push(@obst_enum_op, "typedef enum _$arch\_opcodes {\n");
foreach my $op (keys(%nodes)) {
	my %n = %{ $nodes{"$op"} };
	my $tuple = 0;
	my $n_res = 0;

	# determine arity from in requirements
	$arity = 0;
	if (exists($n{"reg_req"}) && exists($n{"reg_req"}{"in"})) {
		$arity = scalar(@{ $n{"reg_req"}{"in"} });
	}

	$orig_op = $op;
	$op      = $arch."_".$op;
	$temp    = "";

	if (exists($n{"outs"})) {
		undef my @outs;
		@outs = @{ $n{"outs"} };
		push(@obst_proj, "\nenum pn_$op {\n");
		for (my $idx = 0; $idx <= $#outs; $idx++) {
			push(@obst_proj, "  pn_$op\_".$outs[$idx]." = $idx,\n");
		}
		push(@obst_proj, "};\n");
		$tuple = 1;
	}

	push(@obst_opvar, "ir_op *op_$op = NULL;\n");
	push(@obst_get_opvar, "ir_op *get_op_$op(void)         { return op_$op; }\n");
	push(@obst_get_opvar, "int    is_$op(const ir_node *n) { return get_irn_op(n) == op_$op; }\n\n");

	push(@obst_is_archirn, "is_$op(node)");

	push(@obst_header, "extern ir_op *op_$op;\n");
	push(@obst_header, "ir_op *get_op_$op(void);\n");
	push(@obst_header, "int is_$op(const ir_node *n);\n");

	$cmp_attr_func = 0;
	# create compare attribute function if needed
	if (exists($n{"cmp_attr"})) {
		push(@obst_cmp_attr, "static int cmp_attr_$op(ir_node *a, ir_node *b) {\n");
		push(@obst_cmp_attr, "  $arch\_attr_t *attr_a = get_$arch\_attr(a);\n");
		push(@obst_cmp_attr, "  $arch\_attr_t *attr_b = get_$arch\_attr(b);\n");
		push(@obst_cmp_attr, $n{"cmp_attr"});
		push(@obst_cmp_attr, "}\n\n");

		$cmp_attr_func = 1;
	}

	if (exists($n{"rd_constructor"}) && $n{"rd_constructor"} =~ /^NONE$/i) {
		# we explicitly skip the constructor if the specification entry says NONE
	}
	else {
		$n{"comment"} = "construct $op" if(!exists($n{"comment"}));
		$n{"comment"} =~ s/^"|"$//g;    # remove "
		$n{"comment"} = "/* ".$n{"comment"}." */\n";
		push(@obst_constructor, $n{"comment"});

		# create constructor head
		my $complete_args = "";
		my $arg_names     = "";
		$temp             = "";

		$temp = "ir_node *new_rd_$op(dbg_info *db, ir_graph *irg, ir_node *block";
		if (!exists($n{"args"}) || $n{"args"} =~ /^DEFAULT$/i) { # default args
			if ($arity !~ /^\d+$/) {
				print "DEFAULT args require numeric arity (0, 1, 2, ...)! Ignoring op $orig_op!\n";
				next;
			}
			for (my $i = 1; $i <= $arity; $i++) {
				$complete_args .= ", ir_node *op".$i;
				$arg_names     .= ", op".$i;
			}
			if ($tuple == 0) {
				$complete_args .= ", ir_mode *mode";
				$arg_names     .= ", mode";
			}
		}
		else { # user defined args
			for my $href (@{ $n{"args"} }) {
				$href->{"type"} .= " " if ($href->{"type"} !~ / [*]?$/); # put a space between name and type if there is none at the end
				$complete_args  .= ", ".$href->{"type"}.$href->{"name"};
				$arg_names      .= ", ".$href->{"name"};
			}
		}

		# we have additional attribute arguements
		if (exists($n{"attr"})) {
			$complete_args .= ", ".$n{"attr"};
		}

		# $complete_args = substr($complete_args, 2);
		$temp .= "$complete_args)";
		push(@obst_constructor, $temp." {\n");
		push(@obst_header, $temp.";\n");

		# emit constructor code
		if (!exists($n{"rd_constructor"}) || $n{"rd_constructor"} =~ /^DEFAULT$/i) { # default constructor
			if ($arity !~ /^\d+$/) {
				print "DEFAULT rd_constructor requires numeric arity! Ignoring op $orig_op!\n";
				next;
			}

			$temp  = "  ir_node *res;\n";
			$temp .= "  ir_node *in[$arity];\n" if ($arity > 0);
			$temp .= "  int flags = 0;\n";
			$temp .= "  $arch\_attr_t *attr;\n" if (exists($n{"init_attr"}));

			undef my $in_req_var;
			undef my $out_req_var;

			# set up static variables for requirements and registers
			if (exists($n{"reg_req"})) {
				my %req = %{ $n{"reg_req"} };
				my $idx;

				undef my @in;
				@in = @{ $req{"in"} } if (exists($req{"in"}));
				undef my @out;
				@out = @{ $req{"out"} } if exists(($req{"out"}));

				if (@in) {
					$in_req_var = "_in_req_$op";
					$temp .= "  static const $arch\_register_req_t *".$in_req_var."[] =\n  {\n";
					for ($idx = 0; $idx <= $#in; $idx++) {
						$temp .= "    ".$op."_reg_req_in_".$idx.",\n";
					}
					$temp .= "  };\n";
				}

				if (@out) {
					$out_req_var = "_out_req_$op";

					$temp .= "  static const $arch\_register_req_t *".$out_req_var."[] =\n  {\n";
					for ($idx = 0; $idx <= $#out; $idx++) {
						$temp .= "    ".$op."_reg_req_out_".$idx.",\n";
					}
					$temp .= "  };\n";
				}
			}

			$temp .= "\n";
			$temp .= "  if (!op_$op) {\n";
			$temp .= "    assert(0);\n";
			$temp .= "    return NULL;\n";
			$temp .= "  }\n\n";
			for (my $i = 1; $i <= $arity; $i++) {
				$temp .= "  in[".($i - 1)."] = op".$i.";\n";
			}

			# set flags
			if (exists($n{"irn_flags"})) {
				foreach my $flag (split(/\|/, $n{"irn_flags"})) {
					if ($flag eq "R") {
						$temp .= "  flags |= arch_irn_flags_rematerializable;   /* op can be easily recalculated */\n";
					}
					elsif ($flag eq "N") {
						$temp .= "  flags |= arch_irn_flags_dont_spill;         /* op is NOT spillable */\n";
					}
					elsif ($flag eq "I") {
						$temp .= "  flags |= arch_irn_flags_ignore;             /* ignore op for register allocation */\n";
					}
				}
			}

			my $in_param;
			my $out_param;
			# allocate memory and set pointer to register requirements
			if (exists($n{"reg_req"})) {
				my %req = %{ $n{"reg_req"} };

				undef my @in;
				@in = @{ $req{"in"} } if (exists($req{"in"}));
				undef my @out;
				@out = @{ $req{"out"} } if exists(($req{"out"}));

				if (@in) {
					$in_param = $in_req_var;
				}
				else {
					$in_param = "NULL";
				}

				if (@out) {
					$out_param = $out_req_var.", ".($#out + 1);
					$n_res     = $#out;
				}
				else {
					$out_param = "NULL, 0";
				}
			}
			$temp .= "\n  /* create node */\n";
			my $mode = "mode";
			if ($tuple == 1) {
				$mode = "mode_T";
			}
			$temp .= "  res = new_ir_node(db, irg, block, op_$op, $mode, $arity, ".($arity > 0 ? "in" : "NULL").");\n";

			$temp .= "\n  /* init node attributes */\n";
			$temp .= "  init_$arch\_attributes(res, flags, $in_param, $out_param);\n";

			if (exists($n{"init_attr"})) {
				$temp .= "  attr = get_$arch\_attr(res);\n";
				$temp .= $n{"init_attr"}."\n";
			}

			$temp .= "\n  /* optimize node */\n";
			$temp .= "  res = optimize_node(res);\n";
			$temp .= "  irn_vrfy_irg(res, irg);\n\n";

			$temp .= "\n  return res;\n";

			push(@obst_constructor, $temp);
		}
		else { # user defined constructor
			push(@obst_constructor, $n{"rd_constructor"});
		}

		# close constructor function
		push(@obst_constructor, "}\n\n");

	} # constructor creation

	# set default values for state and flags if not given
	$n{"state"}    = "floats" if (! exists($n{"state"}));
	$n{"op_flags"} = "N"      if (! exists($n{"op_flags"}));


	push(@obst_new_irop, "\n  memset(&ops, 0, sizeof(ops));\n");
	push(@obst_new_irop, "  ops.dump_node     = $arch\_dump_node;\n");

	if ($cmp_attr_func) {
		push(@obst_new_irop, "  ops.node_cmp_attr = cmp_attr_$op;\n");
	}

	$n_opcodes++;
	$temp  = "  op_$op = new_ir_op(cur_opcode + iro_$op, \"$op\", op_pin_state_".$n{"state"}.", ".$n{"op_flags"};
	$temp .= "|M, ".translate_arity($arity).", 0, sizeof($arch\_attr_t) + $n_res * sizeof(arch_register_t *), &ops);\n";
	push(@obst_new_irop, $temp);
	push(@obst_enum_op, "  iro_$op,\n");
}
push(@obst_enum_op, "  iro_$arch\_last_generated,\n");
push(@obst_enum_op, "  iro_$arch\_last = iro_$arch\_last_generated");
push(@obst_enum_op, " + $additional_opcodes") if (defined($additional_opcodes));
push(@obst_enum_op, "\n} $arch\_opcodes;\n\n");

# emit the code

open(OUT, ">$target_c") || die("Could not open $target_c, reason: $!\n");

print OUT "#include \"gen_$arch\_regalloc_if_t.h\"\n\n";
print OUT @obst_cmp_attr;
print OUT "\n";
print OUT @obst_opvar;
print OUT "\n";
print OUT @obst_get_opvar;
print OUT "\n";

print OUT<<ENDOFISIRN;

static int $arch\_opcode_start = -1;
static int $arch\_opcode_end   = -1;

/** Return the opcode number of the first $arch opcode. */
int get_$arch\_opcode_first(void) {
  return $arch\_opcode_start;
}

/** Return the opcode number of the last $arch opcode + 1. */
int get_$arch\_opcode_last(void) {
  return $arch\_opcode_end;
}

/** Return non-zero if the given node is a $arch machine node. */
int is_$arch\_irn(const ir_node *node) {
  unsigned opc = (unsigned)get_irn_opcode(node);

  assert($arch\_opcode_start > 0 && "missing opcode init");
  assert($arch\_opcode_end > 0 && "missing opcode init");

  if (opc - (unsigned)$arch\_opcode_start < (unsigned)($arch\_opcode_end - $arch\_opcode_start))
    return 1;

  return 0;
}

int get_$arch\_irn_opcode(const ir_node *node) {
  assert(is_$arch\_irn(node));
  return get_irn_opcode(node) - $arch\_opcode_start;
}

ENDOFISIRN

print OUT @obst_constructor;

print OUT<<ENDOFMAIN;
/**
 * Creates the $arch specific Firm machine operations
 * needed for the assembler irgs.
 */
void $arch\_create_opcodes(void) {
#define N   irop_flag_none
#define L   irop_flag_labeled
#define C   irop_flag_commutative
#define X   irop_flag_cfopcode
#define I   irop_flag_ip_cfopcode
#define F   irop_flag_fragile
#define Y   irop_flag_forking
#define H   irop_flag_highlevel
#define c   irop_flag_constlike
#define K   irop_flag_keep
#define M   irop_flag_machine
#define O   irop_flag_machine_op
#define R   (irop_flag_user << 0)

  ir_op_ops ops;
  int cur_opcode = get_next_ir_opcodes(iro_$arch\_last);

  $arch\_opcode_start = cur_opcode;
ENDOFMAIN

print OUT @obst_new_irop;
print OUT "\n";
print OUT "  $arch\_register_additional_opcodes(cur_opcode);\n" if (defined($additional_opcodes));
print OUT "  $arch\_opcode_end = cur_opcode + iro_$arch\_last";
print OUT " + $additional_opcodes" if (defined($additional_opcodes));
print OUT ";\n";
print OUT "}\n";

close(OUT);

open(OUT, ">$target_h") || die("Could not open $target_h, reason: $!\n");

print OUT "#ifndef __GEN_$arch\_NEW_NODES_H__\n";
print OUT "#define __GEN_$arch\_NEW_NODES_H__\n\n";
print OUT @obst_enum_op;
print OUT "int is_$arch\_irn(const ir_node *node);\n\n";
print OUT "int get_$arch\_opcode_first(void);\n";
print OUT "int get_$arch\_opcode_last(void);\n";
print OUT "int get_$arch\_irn_opcode(const ir_node *node);\n";
print OUT @obst_header;
print OUT @obst_proj;
print OUT "\n#endif /* __GEN_$arch\_NEW_NODES_H__ */\n";

close(OUT);

###
# Translates numeric arity into string constant.
###
sub translate_arity {
	my $arity = shift;

	if ($arity =~ /^\d+$/) {
		if    ($arity == 0) {
			return "oparity_zero";
		}
		elsif ($arity == 1) {
			return "oparity_unary";
		}
		elsif ($arity == 2) {
			return "oparity_binary";
		}
		elsif ($arity == 3) {
			return "oparity_trinary";
		}
		else {
			return "$arity";
		}
	}
	else {
		return "oparity_".$arity;
	}
}
