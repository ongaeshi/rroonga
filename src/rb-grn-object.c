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

#define SELF(object) (rb_rb_grn_object_from_ruby_object(object))

VALUE rb_cGrnObject;

typedef struct _RbGrnObject RbGrnObject;
struct _RbGrnObject
{
    grn_ctx *context;
    grn_obj *object;
};

static RbGrnObject *
rb_rb_grn_object_from_ruby_object (VALUE object)
{
    RbGrnObject *rb_grn_object;

    if (!RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cGrnObject))) {
	rb_raise(rb_eTypeError, "not a groonga object");
    }

    Data_Get_Struct(object, RbGrnObject, rb_grn_object);
    if (!rb_grn_object)
	rb_raise(rb_eGrnError, "groonga object is NULL");

    return rb_grn_object;
}

grn_obj *
rb_grn_object_from_ruby_object (VALUE object, grn_ctx *context)
{
    if (NIL_P(object))
        return NULL;

    if (context) {
	grn_obj *grn_object;
	if (RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cString))) {
	    grn_object = grn_ctx_lookup(context,
					StringValuePtr(object),
					RSTRING_LEN(object));
	    if (!grn_object)
		rb_raise(rb_eArgError,
			 "unregistered groonga object: name: <%s>",
			 rb_grn_inspect(object));
	    return grn_object;
	} else if (RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cInteger))) {
	    grn_object = grn_ctx_get(context, NUM2UINT(object));
	    if (!grn_object)
		rb_raise(rb_eArgError,
			 "unregistered groonga object: ID: <%s>",
			 rb_grn_inspect(object));
	    return grn_object;
	}
    }

    return rb_rb_grn_object_from_ruby_object(object)->object;
}

static void
rb_rb_grn_object_free (void *object)
{
    xfree(object);
}

VALUE
rb_grn_object_to_ruby_class (grn_obj *object)
{
    VALUE klass = Qnil;

    switch (object->header.type) {
      case GRN_DB:
	klass = rb_cGrnDatabase;
	break;
      case GRN_TABLE_HASH_KEY:
	klass = rb_cGrnHash;
	break;
      case GRN_TABLE_PAT_KEY:
	klass = rb_cGrnPatriciaTrie;
	break;
      case GRN_TABLE_NO_KEY:
	klass = rb_cGrnArray;
	break;
      case GRN_TYPE:
	klass = rb_cGrnType;
	break;
      case GRN_ACCESSOR:
	klass = rb_cGrnAccessor;
	break;
      case GRN_PROC:
	klass = rb_cGrnProcedure;
	break;
      case GRN_COLUMN_FIX_SIZE:
	klass = rb_cGrnFixSizeColumn;
	break;
      case GRN_COLUMN_VAR_SIZE:
	klass = rb_cGrnVarSizeColumn;
	break;
      case GRN_COLUMN_INDEX:
	klass = rb_cGrnIndexColumn;
	break;
      default:
	rb_raise(rb_eTypeError,
		 "unsupported groonga object type: %d",
		 object->header.type);
	break;
    }

    return klass;
}

VALUE
rb_grn_object_to_ruby_object (VALUE klass, grn_ctx *context, grn_obj *object)
{
    RbGrnObject *rb_grn_object;

    if (!object)
        return Qnil;

    rb_grn_object = ALLOC(RbGrnObject);
    rb_grn_object->context = context;
    rb_grn_object->object = object;

    if (NIL_P(klass))
        klass = GRNOBJECT2RCLASS(object);

    return Data_Wrap_Struct(klass, NULL, rb_rb_grn_object_free, rb_grn_object);
}

VALUE
rb_grn_object_alloc (VALUE klass)
{
    return Data_Wrap_Struct(klass, NULL, rb_rb_grn_object_free, NULL);
}

grn_ctx *
rb_grn_object_ensure_context (VALUE object, VALUE rb_context)
{
    if (NIL_P(rb_context)) {
	RbGrnObject *rb_grn_object;

	rb_grn_object = SELF(object);
	if (rb_grn_object && rb_grn_object->context)
	    return rb_grn_object->context;
    }

    return rb_grn_context_ensure(rb_context);
}

void
rb_grn_object_initialize (VALUE self, grn_ctx *context, grn_obj *object)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = ALLOC(RbGrnObject);
    DATA_PTR(self) = rb_grn_object;
    rb_grn_object->context = context;
    rb_grn_object->object = object;
}

VALUE
rb_grn_object_close (VALUE self)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = SELF(self);
    if (rb_grn_object->context && rb_grn_object->object) {
        GRN_OBJ_FIN(rb_grn_object->context, rb_grn_object->object);
        rb_grn_object->context = NULL;
        rb_grn_object->object = NULL;
    }
    return Qnil;
}

static VALUE
rb_grn_object_closed_p (VALUE self)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = SELF(self);
    if (rb_grn_object->context && rb_grn_object->object)
        return Qfalse;
    else
        return Qtrue;
}

VALUE
rb_grn_object_inspect_object (VALUE inspected, grn_ctx *context, grn_obj *object)
{
    VALUE rb_object;

    rb_object = GRNOBJECT2RVAL(Qnil, context, object);
    rb_str_concat(inspected, rb_inspect(rb_object));

    return inspected;
}

VALUE
rb_grn_object_inspect_header (VALUE self, VALUE inspected)
{
    rb_str_cat2(inspected, "#<");
    rb_str_concat(inspected, rb_inspect(rb_obj_class(self)));
    rb_str_cat2(inspected, " ");

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_id (VALUE inspected,
				  grn_ctx *context, grn_obj *object)
{
    grn_id id;
    VALUE rb_id;

    rb_str_cat2(inspected, "id: <");
    id = grn_obj_id(context, object);
    if (id == GRN_ID_NIL)
	rb_id = Qnil;
    else
	rb_id = UINT2NUM(id);
    rb_str_concat(inspected, rb_obj_as_string(rb_id));
    rb_str_cat2(inspected, ">");

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_name (VALUE inspected,
				    grn_ctx *context, grn_obj *object)
{
    int name_size;

    rb_str_cat2(inspected, "name: ");
    name_size = grn_obj_name(context, object, NULL, 0);
    if (name_size == 0) {
	rb_str_cat2(inspected, "(anonymous)");
    } else {
	VALUE name;

	name = rb_str_buf_new(name_size);
	grn_obj_name(context, object, RSTRING_PTR(name), name_size);
	rb_str_set_len(name, name_size);
	rb_str_cat2(inspected, "<");
	rb_str_concat(inspected, name);
	rb_str_cat2(inspected, ">");
    }

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_path (VALUE inspected,
				    grn_ctx *context, grn_obj *object)
{
    const char *path;

    rb_str_cat2(inspected, "path: ");
    path = grn_obj_path(context, object);
    if (path) {
	rb_str_cat2(inspected, "<");
	rb_str_cat2(inspected, path);
	rb_str_cat2(inspected, ">");
    } else {
	rb_str_cat2(inspected, "(temporary)");
    }

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_domain (VALUE inspected,
				      grn_ctx *context, grn_obj *object)
{
    grn_id domain;

    rb_str_cat2(inspected, "domain: ");
    domain = object->header.domain;
    rb_str_cat2(inspected, "<");
    if (domain == GRN_ID_NIL) {
	rb_str_cat2(inspected, "nil");
    } else {
	grn_obj *domain_object;

	domain_object = grn_ctx_get(context, domain);
	if (domain_object) {
	    rb_grn_object_inspect_object(inspected, context, domain_object);
	} else {
	    rb_str_concat(inspected, rb_obj_as_string(UINT2NUM(domain)));
	}
    }
    rb_str_cat2(inspected, ">");

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_range (VALUE inspected,
				     grn_ctx *context, grn_obj *object)
{
    grn_id range;

    rb_str_cat2(inspected, "range: ");
    range = grn_obj_get_range(context, object);
    rb_str_cat2(inspected, "<");
    if (range == GRN_ID_NIL) {
	rb_str_cat2(inspected, "nil");
    } else {
	grn_obj *range_object;

	range_object = grn_ctx_get(context, range);
	if (range_object) {
	    rb_grn_object_inspect_object(inspected, context, range_object);
	} else {
	    rb_str_concat(inspected, rb_obj_as_string(UINT2NUM(range)));
	}
    }
    rb_str_cat2(inspected, ">");

    return inspected;
}

VALUE
rb_grn_object_inspect_object_content (VALUE inspected,
				      grn_ctx *context, grn_obj *object)
{
    rb_grn_object_inspect_content_id(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_name(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_path(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_domain(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_range(inspected, context, object);

    return inspected;
}

VALUE
rb_grn_object_inspect_content (VALUE self, VALUE inspected)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_obj *object;

    rb_grn_object = SELF(self);
    context = rb_grn_object->context;
    object = rb_grn_object->object;

    if (!object)
	return inspected;

    rb_grn_object_inspect_object_content(inspected, context, object);

    return inspected;
}

VALUE
rb_grn_object_inspect_footer (VALUE self, VALUE inspected)
{
    rb_str_cat2(inspected, ">");

    return inspected;
}

static VALUE
rb_grn_object_inspect (VALUE self)
{
    VALUE inspected;

    inspected = rb_str_new2("");
    rb_grn_object_inspect_header(self, inspected);
    rb_grn_object_inspect_content(self, inspected);
    rb_grn_object_inspect_footer(self, inspected);

    return inspected;
}

static VALUE
rb_grn_object_get_id (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_id id;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    id = grn_obj_id(rb_grn_object->context, rb_grn_object->object);
    if (id == GRN_ID_NIL)
	return Qnil;
    else
        return UINT2NUM(id);
}

static VALUE
rb_grn_object_get_domain (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_id domain;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    context = rb_grn_object->context;
    domain = rb_grn_object->object->header.domain;
    if (domain == GRN_ID_NIL) {
	return Qnil;
    } else {
	grn_obj *domain_object;

	domain_object = grn_ctx_get(context, domain);
	if (domain_object)
	    return GRNOBJECT2RVAL(Qnil, context, domain_object);
	else
	    return UINT2NUM(domain);
    }
}

static VALUE
rb_grn_object_get_name (VALUE self)
{
    RbGrnObject *rb_grn_object;
    VALUE rb_name;
    int name_size;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    name_size = grn_obj_name(rb_grn_object->context, rb_grn_object->object,
			     NULL, 0);
    if (name_size == 0)
	return Qnil;

    rb_name = rb_str_buf_new(name_size);
    RSTRING_LEN(rb_name) = name_size;
    grn_obj_name(rb_grn_object->context, rb_grn_object->object,
		 RSTRING_PTR(rb_name), name_size);
    return rb_name;
}

static VALUE
rb_grn_object_get_range (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_id range;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    range = grn_obj_get_range(rb_grn_object->context, rb_grn_object->object);
    if (range == GRN_ID_NIL)
	return Qnil;
    else
	return INT2NUM(range);
}

static VALUE
rb_grn_object_equal (VALUE self, VALUE other)
{
    RbGrnObject *self_rb_grn_object, *other_rb_grn_object;

    if (self == other)
        return Qtrue;

    if (!RVAL2CBOOL(rb_funcall(rb_obj_class(self), rb_intern("=="), 1,
                               rb_obj_class(other))))
        return Qfalse;

    self_rb_grn_object = SELF(self);
    other_rb_grn_object = SELF(other);

    return self_rb_grn_object->object == other_rb_grn_object->object;
}

VALUE
rb_grn_bulk_to_ruby_object (grn_ctx *context, grn_obj *bulk)
{
    VALUE rb_value = Qnil;
    grn_obj *range;

    switch (bulk->header.domain) {
      case GRN_DB_VOID:
	break;
      case GRN_DB_INT:
	if (GRN_BULK_VSIZE(bulk) == sizeof(int32_t)) {
	    int32_t *int32_value = (int32_t *)GRN_BULK_HEAD(bulk);
	    rb_value = INT2NUM(*int32_value);
	}
	break;
      case GRN_DB_UINT:
	if (GRN_BULK_VSIZE(bulk) == sizeof(uint32_t)) {
	    uint32_t *uint32_value = (uint32_t *)GRN_BULK_HEAD(bulk);
	    rb_value = UINT2NUM(*uint32_value);
	} else if (GRN_BULK_VSIZE(bulk) == sizeof(uint64_t)) {
	    uint64_t *uint64_value = (uint64_t *)GRN_BULK_HEAD(bulk);
	    rb_value = ULL2NUM(*uint64_value);
	}
	break;
      case GRN_DB_INT64:
	if (GRN_BULK_VSIZE(bulk) == sizeof(int64_t)) {
	    int64_t *int64_value = (int64_t *)GRN_BULK_HEAD(bulk);
	    rb_value = LL2NUM(*int64_value);
	}
	break;
      case GRN_DB_FLOAT:
	if (GRN_BULK_VSIZE(bulk) == sizeof(double)) {
	    double *double_value = (double *)GRN_BULK_HEAD(bulk);
	    rb_value = rb_float_new(*double_value);
	}
	break;
      case GRN_DB_TIME:
	if (GRN_BULK_VSIZE(bulk) == sizeof(grn_timeval)) {
	    grn_timeval *time_value = (grn_timeval *)GRN_BULK_HEAD(bulk);
	    rb_value = rb_funcall(rb_cTime, rb_intern("at"), 2,
				  INT2NUM(time_value->tv_sec),
				  INT2NUM(time_value->tv_usec));
	}
      case GRN_DB_SHORTTEXT:
      case GRN_DB_TEXT:
      case GRN_DB_LONGTEXT:
      case GRN_DB_DELIMIT:
      case GRN_DB_UNIGRAM:
      case GRN_DB_BIGRAM:
      case GRN_DB_TRIGRAM:
      case GRN_DB_MECAB:
	break;
      default:
	range = grn_ctx_get(context, bulk->header.domain);
	if (range) {
	    switch (range->header.type) {
	      case GRN_TABLE_HASH_KEY:
	      case GRN_TABLE_PAT_KEY:
	      case GRN_TABLE_NO_KEY:
		if (GRN_BULK_VSIZE(bulk) == sizeof(int32_t)) {
		    int32_t *int32_value = (int32_t *)GRN_BULK_HEAD(bulk);
		    rb_value = INT2NUM(*int32_value);
		}
		break;
	      default:
		break;
	    }
	}
	break;
    }

    if (NIL_P(rb_value))
	rb_value = rb_str_new(GRN_BULK_HEAD(bulk), GRN_BULK_VSIZE(bulk));

    return rb_value;
}

VALUE
rb_grn_object_array_reference (VALUE self, VALUE rb_id)
{
    VALUE exception;
    RbGrnObject *rb_grn_object;
    grn_id id, range_id;
    grn_ctx *context;
    grn_obj *object;
    grn_obj *range;
    unsigned char range_type;
    grn_obj *value = NULL;
    VALUE rb_value = Qnil;

    rb_grn_object = SELF(self);
    context = rb_grn_object->context;
    object = rb_grn_object->object;
    if (!object)
	return Qnil;

    id = NUM2UINT(rb_id);
    range_id = grn_obj_get_range(context, object);
    range = grn_ctx_get(context, range_id);
    range_type = range ? range->header.type : GRN_VOID;
    switch (object->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_NO_KEY:
      case GRN_TYPE:
      case GRN_ACCESSOR: /* FIXME */
	value = grn_obj_open(context, GRN_BULK, 0, range_id);
	break;
      case GRN_COLUMN_VAR_SIZE:
      case GRN_COLUMN_FIX_SIZE:
	switch (object->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
	  case GRN_OBJ_COLUMN_VECTOR:
	    value = grn_obj_open(context, GRN_VECTOR, 0, range_id);
	    if (!value)
		rb_grn_context_check(context, self);
	    break;
	  case GRN_OBJ_COLUMN_INDEX:
	  case GRN_OBJ_COLUMN_SCALAR:
	    value = grn_obj_open(context, GRN_BULK, 0, range_id);
	    if (!value)
		rb_grn_context_check(context, self);
	    break;
	  default:
	    rb_raise(rb_eGrnError, "unsupported column type: %u: %s",
		     range_type, rb_grn_inspect(self));
	    break;
	}
	break;
      default:
	rb_raise(rb_eGrnError,
		 "unsupported type: %s", rb_grn_inspect(self));
	break;
    }

    value = grn_obj_get_value(context, object, id, value);
    if (!value) {
	rb_grn_context_check(context, self);
	return Qnil;
    }

    switch (value->header.type) {
      case GRN_BULK:
	if (GRN_BULK_EMPTYP(value)) {
	    exception = rb_grn_context_to_exception(context);
	    grn_obj_close(context, value);
	    if (!NIL_P(exception))
		rb_exc_raise(exception);
	    return Qnil;
	}
	rb_value = GRNBULK2RVAL(context, value);
	break;
      case GRN_VECTOR:
	rb_value = GRNVECTOR2RVAL(context, value);
	break;
      case GRN_UVECTOR:
	rb_value = GRNUVECTOR2RVAL(context, value);
	break;
      default:
	rb_raise(rb_eGrnError,
		 "unsupported value type: 0x%0x: %s",
		 value->header.type, rb_grn_inspect(self));
	break;
    }

    exception = rb_grn_context_to_exception(context);
    grn_obj_close(context, value);
    if (!NIL_P(exception))
	rb_exc_raise(exception);

    return rb_value;
}

static VALUE
rb_grn_object_set (VALUE self, VALUE rb_id, VALUE rb_value, int flags)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_id id;
    grn_obj *value;
    grn_rc rc;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    context = rb_grn_object->context;
    id = NUM2UINT(rb_id);
    value = RVAL2GRNBULK(context, rb_value);
    rc = grn_obj_set_value(context, rb_grn_object->object, id,
			   value, flags);
    grn_obj_close(context, value);
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

static VALUE
rb_grn_object_array_set (VALUE self, VALUE rb_id, VALUE rb_value)
{
    return rb_grn_object_set(self, rb_id, rb_value, GRN_OBJ_SET);
}

static VALUE
rb_grn_object_append_value (VALUE self, VALUE rb_id, VALUE rb_value)
{
    return rb_grn_object_set(self, rb_id, rb_value, GRN_OBJ_APPEND);
}

static VALUE
rb_grn_object_search (int argc, VALUE *argv, VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_obj *object;
    grn_obj *query;
    grn_obj *result;
    grn_sel_operator operator;
    grn_search_optarg search_options;
    rb_grn_boolean search_options_is_set = RB_GRN_FALSE;
    rb_grn_boolean query_is_created = RB_GRN_FALSE;
    grn_rc rc;
    VALUE rb_query, options, rb_result, rb_operator;
    VALUE rb_exact, rb_left_common_prefix, rb_suffix, rb_prefix, rb_partial;
    VALUE rb_near, rb_near2, rb_similar, rb_term_extract;
    VALUE rb_similarity_threshold, rb_max_interval, rb_weight_vector;
    VALUE rb_procedure, rb_max_size;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    context = rb_grn_object->context;
    object = rb_grn_object->object;

    rb_scan_args(argc, argv, "11", &rb_query, &options);

    if (CBOOL2RVAL(rb_obj_is_kind_of(rb_query, rb_cGrnQuery))) {
	grn_query *_query;
	_query = RVAL2GRNQUERY(rb_query);
	query = (grn_obj *)_query;
    } else if (CBOOL2RVAL(rb_obj_is_kind_of(rb_query, rb_cString))) {
	query_is_created = RB_GRN_TRUE;
	query = grn_obj_open(context, GRN_BULK, 0, 0);
	rc = grn_bulk_write(context, query,
			    StringValuePtr(rb_query), RSTRING_LEN(rb_query));
	rb_grn_context_check(context, rb_query);
	rb_grn_rc_check(rc, rb_query);
    } else {
	rb_raise(rb_eArgError,
		 "query should be Groonga::Query or query string: <%s>",
		 rb_grn_inspect(rb_query));
    }

    rb_grn_scan_options(options,
			"result", &rb_result,
			"operator", &rb_operator,
			"exact", &rb_exact,
			"left_common_prefix", &rb_left_common_prefix,
			"suffix", &rb_suffix,
			"prefix", &rb_prefix,
			"partial", &rb_partial,
			"near", &rb_near,
			"near2", &rb_near2,
			"similar", &rb_similar,
			"term_exact", &rb_term_extract,
			"similarity_threshold", &rb_similarity_threshold,
			"max_interval", &rb_max_interval,
			"weight_vector", &rb_weight_vector,
			"procedure", &rb_procedure,
			"max_size", &rb_max_size,
			NULL);

    if (NIL_P(rb_result)) {
	result = grn_table_create(context, NULL, 0, NULL,
				  GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
				  grn_ctx_get(context, object->header.domain),
				  0, GRN_ENC_NONE);
	rb_grn_context_check(context, self);
    } else {
	result = RVAL2GRNOBJECT(rb_result, context);
    }

    operator = RVAL2GRNSELECTOPERATOR(rb_operator);

    search_options.flags = 0;
    if (!NIL_P(rb_exact)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_exact))
	    search_options.flags |= GRN_SEARCH_EXACT;
    }
    if (!NIL_P(rb_left_common_prefix)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_left_common_prefix))
	    search_options.flags |= GRN_SEARCH_LCP;
    }
    if (!NIL_P(rb_suffix)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_suffix))
	    search_options.flags |= GRN_SEARCH_SUFFIX;
    }
    if (!NIL_P(rb_prefix)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_prefix))
	    search_options.flags |= GRN_SEARCH_PREFIX;
    }
    if (!NIL_P(rb_partial)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_partial))
	    search_options.flags |= GRN_SEARCH_PARTIAL;
    }
    if (!NIL_P(rb_near)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_near))
	    search_options.flags |= GRN_SEARCH_NEAR;
    }
    if (!NIL_P(rb_near2)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_near2))
	    search_options.flags |= GRN_SEARCH_NEAR2;
    }
    if (!NIL_P(rb_similar)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_similar))
	    search_options.flags |= GRN_SEARCH_SIMILAR;
    }
    if (!NIL_P(rb_term_extract)) {
	search_options_is_set = RB_GRN_TRUE;
	if (RVAL2CBOOL(rb_term_extract))
	    search_options.flags |= GRN_SEARCH_TERM_EXTRACT;
    }

    search_options.similarity_threshold = 30; /* FIXME */
    if (!NIL_P(rb_similarity_threshold)) {
	search_options_is_set = RB_GRN_TRUE;
	search_options.similarity_threshold = NUM2INT(rb_similarity_threshold);
    }

    search_options.max_interval = 5; /* FIXME */
    if (!NIL_P(rb_max_interval)) {
	search_options_is_set = RB_GRN_TRUE;
	search_options.max_interval = NUM2INT(rb_max_interval);
    }

    search_options.weight_vector = NULL; /* FIXME */
    if (!NIL_P(rb_weight_vector)) {
	search_options_is_set = RB_GRN_TRUE;
	/* FIXME */
	/* search_options.weight_vector = RVAL2INTVECTOR(rb_weight_vector); */
    }

    search_options.proc = NULL;
    if (!NIL_P(rb_procedure)) {
	search_options_is_set = RB_GRN_TRUE;
	search_options.proc = RVAL2GRNOBJECT(rb_procedure, context);
    }

    search_options.max_size = 100; /* FIXME */
    if (!NIL_P(rb_max_size)) {
	search_options_is_set = RB_GRN_TRUE;
	search_options.max_size = INT2NUM(rb_max_size);
    }

    rc = grn_obj_search(context,
			object,
			query,
			result,
			operator,
			search_options_is_set ? &search_options : NULL);
    if (query_is_created)
	grn_obj_close(context, query);
    rb_grn_rc_check(rc, self);

    if (NIL_P(rb_result))
	return GRNOBJECT2RVAL(Qnil, context, result);
    else
	return rb_result;
}

static VALUE
rb_grn_object_remove (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_rc rc;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    context = rb_grn_object->context;
    rc = grn_obj_remove(context, rb_grn_object->object);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

void
rb_grn_init_object (VALUE mGrn)
{
    rb_cGrnObject = rb_define_class_under(mGrn, "Object", rb_cObject);
    rb_define_alloc_func(rb_cGrnObject, rb_grn_object_alloc);

    rb_define_method(rb_cGrnObject, "inspect", rb_grn_object_inspect, 0);

    rb_define_method(rb_cGrnObject, "id", rb_grn_object_get_id, 0);
    rb_define_method(rb_cGrnObject, "domain", rb_grn_object_get_domain, 0);
    rb_define_method(rb_cGrnObject, "name", rb_grn_object_get_name, 0);
    rb_define_method(rb_cGrnObject, "range", rb_grn_object_get_range, 0);

    rb_define_method(rb_cGrnObject, "==", rb_grn_object_equal, 1);

    rb_define_method(rb_cGrnObject, "close", rb_grn_object_close, 0);
    rb_define_method(rb_cGrnObject, "closed?", rb_grn_object_closed_p, 0);

    rb_define_method(rb_cGrnObject, "[]", rb_grn_object_array_reference, 1);
    rb_define_method(rb_cGrnObject, "[]=", rb_grn_object_array_set, 2);
    rb_define_method(rb_cGrnObject, "append", rb_grn_object_append_value, 2);

    rb_define_method(rb_cGrnObject, "search", rb_grn_object_search, -1);

    rb_define_method(rb_cGrnObject, "remove", rb_grn_object_remove, 0);
}
