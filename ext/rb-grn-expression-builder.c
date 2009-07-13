/* -*- c-file-style: "ruby" -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "rb-grn.h"

VALUE rb_cGrnExpressionBuilder;

VALUE
rb_grn_expression_builder_new (VALUE table)
{
    return rb_funcall(rb_cGrnExpressionBuilder, rb_intern("new"), 1, table);
}

static VALUE
build (VALUE self)
{
    return rb_funcall(self, rb_intern("build"), 0);
}

static VALUE
build_block (VALUE self)
{
    return rb_funcall(rb_block_proc(), rb_intern("call"), 1, self);
}

VALUE
rb_grn_expression_builder_build (VALUE self)
{
    return rb_iterate(build, self, build_block, self);
}

void
rb_grn_init_expression_builder (VALUE mGrn)
{
    rb_cGrnExpressionBuilder =
        rb_const_get(mGrn, rb_intern("ExpressionBuilder"));
}