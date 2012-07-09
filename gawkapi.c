/*
 * gawkapi.c -- Implement the functions defined for gawkapi.h
 */

/* 
 * Copyright (C) 2012, the Free Software Foundation, Inc.
 * 
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 * 
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "awk.h"

static awk_bool_t node_to_awk_value(NODE *node, awk_value_t *result, awk_valtype_t wanted);

/*
 * Get the count'th paramater, zero-based.
 * Returns false if count is out of range, or if actual paramater
 * does not match what is specified in wanted. In the latter
 * case, fills in result->val_type with the actual type.
 */
static awk_bool_t
api_get_argument(awk_ext_id_t id, size_t count,
			awk_valtype_t wanted, awk_value_t *result)
{
	NODE *arg;

	if (result == NULL)
		return false;

	(void) id;

	/* set up default result */
	memset(result, 0, sizeof(*result));
	result->val_type = AWK_UNDEFINED;

	/*
	 * Song and dance here.  get_array_argument() and get_scalar_argument()
	 * will force a change in type of a parameter that is Node_var_new.
	 *
	 * Start by looking at the unadulterated argument as it was passed.
	 */
	arg = get_argument(count);
	if (arg == NULL)
		return false;

	/* if type is undefined */
	if (arg->type == Node_var_new) {
		if (wanted == AWK_UNDEFINED)
			return true;
		else if (wanted == AWK_ARRAY) {
			goto array;
		} else {
			goto scalar;
		}
	}
	
	/* at this point, we have real type */
	if (arg->type == Node_var_array || arg->type == Node_array_ref) {
		if (wanted != AWK_ARRAY && wanted != AWK_UNDEFINED)
			return false;
		goto array;
	} else
		goto scalar;

array:
	/* get the array here */
	arg = get_array_argument(count, false);
	if (arg == NULL)
		return false;

	return node_to_awk_value(arg, result, wanted);

scalar:
	/* at this point we have a real type that is not an array */
	arg = get_scalar_argument(count, false);
	if (arg == NULL)
		return false;

	return node_to_awk_value(arg, result, wanted);
}

static awk_bool_t
api_set_argument(awk_ext_id_t id,
		size_t count,
		awk_array_t new_array)
{
	NODE *arg;
	NODE *array = (NODE *) new_array;

	(void) id;

	if (array == NULL || array->type != Node_var_array)
		return false;

	if (   (arg = get_argument(count)) == NULL
	    || arg->type != Node_var_new)
		return false;

	arg = get_array_argument(count, false);
	if (arg == NULL)
		return false;

	array->vname = arg->vname;
	*arg = *array;
	freenode(array);

	return true;
}

/* awk_value_to_node --- convert a value into a NODE */

NODE *
awk_value_to_node(const awk_value_t *retval)
{
	NODE *ext_ret_val;

	if (retval == NULL)
		fatal(_("awk_value_to_node: received null retval"));

	ext_ret_val = NULL;
	if (retval->val_type == AWK_ARRAY) {
		ext_ret_val = (NODE *) retval->array_cookie;
	} else if (retval->val_type == AWK_UNDEFINED) {
		ext_ret_val = dupnode(Nnull_string);
	} else if (retval->val_type == AWK_NUMBER) {
		ext_ret_val = make_number(retval->num_value);
	} else {
		ext_ret_val = make_string(retval->str_value.str,
				retval->str_value.len);
	}

	return ext_ret_val;
}

/* Functions to print messages */

/* api_fatal --- print a fatal message and exit */

static void
api_fatal(awk_ext_id_t id, const char *format, ...)
{
	va_list args;

	(void) id;

	va_start(args, format);
	err(true, _("fatal: "), format, args);
	va_end(args);
}

/* api_warning --- print a warning message and exit */

static void
api_warning(awk_ext_id_t id, const char *format, ...)
{
	va_list args;

	(void) id;

	va_start(args, format);
	err(false, _("warning: "), format, args);
	va_end(args);
}

/* api_lintwarn --- print a lint warning message and exit if appropriate */

static void
api_lintwarn(awk_ext_id_t id, const char *format, ...)
{
	va_list args;

	(void) id;

	va_start(args, format);
	if (lintwarn == r_fatal) {
		err(true, _("fatal: "), format, args);
		va_end(args);
	} else {
		err(false, _("warning: "), format, args);
		va_end(args);
	}
}

/* api_register_open_hook --- register an open hook; for opening files read-only */

static void
api_register_open_hook(awk_ext_id_t id, void* (*open_func)(IOBUF_PUBLIC *))
{
	(void) id;

	register_open_hook(open_func);
}

/* Functions to update ERRNO */

/* api_update_ERRNO_int --- update ERRNO with an integer value */

static void
api_update_ERRNO_int(awk_ext_id_t id, int errno_val)
{
	(void) id;

	update_ERRNO_int(errno_val);
}

/* api_update_ERRNO_string --- update ERRNO with a string value */

static void
api_update_ERRNO_string(awk_ext_id_t id,
			const char *string,
			awk_bool_t translate)
{
	(void) id;

	update_ERRNO_string(string, (translate ? TRANSLATE : DONT_TRANSLATE));
}

/* api_unset_ERRNO --- unset ERRNO */

static void
api_unset_ERRNO(awk_ext_id_t id)
{
	(void) id;

	unset_ERRNO();
}


/* api_add_ext_func --- add a function to the interpreter, returns true upon success */

static awk_bool_t
api_add_ext_func(awk_ext_id_t id,
		const awk_ext_func_t *func,
		const char *namespace)
{
	(void) id;
	(void) namespace;

	return make_builtin(func);
}

/* Stuff for exit handler - do it as linked list */

struct ext_exit_handler {
	struct ext_exit_handler *next;
	void (*funcp)(void *data, int exit_status);
	void *arg0;
};
static struct ext_exit_handler *list_head = NULL;

/* run_ext_exit_handlers --- run the extension exit handlers, LIFO order */

void
run_ext_exit_handlers(int exitval)
{
	struct ext_exit_handler *p, *next;

	for (p = list_head; p != NULL; p = next) {
		next = p->next;
		p->funcp(p->arg0, exitval);
		free(p);
	}
	list_head = NULL;
}

/* api_awk_atexit --- add an exit call back, returns true upon success */

static void
api_awk_atexit(awk_ext_id_t id,
		void (*funcp)(void *data, int exit_status),
		void *arg0)
{
	struct ext_exit_handler *p;

	(void) id;

	/* allocate memory */
	emalloc(p, struct ext_exit_handler *, sizeof(struct ext_exit_handler), "api_awk_atexit");

	/* fill it in */
	p->funcp = funcp;
	p->arg0 = arg0;

	/* add to linked list, LIFO order */
	p->next = list_head;
	list_head = p;
}

/* node_to_awk_value --- convert a node into a value for an extension */

static awk_bool_t
node_to_awk_value(NODE *node, awk_value_t *val, awk_valtype_t wanted)
{
	awk_bool_t ret = false;

	switch (node->type) {
	case Node_var_new:	/* undefined variable */
		val->val_type = AWK_UNDEFINED;
		if (wanted == AWK_UNDEFINED) {
			ret = true;
		}
		break;

	case Node_var:
		node = node->var_value;
		/* FALL THROUGH */
	case Node_val:
		/* a scalar value */
		switch (wanted) {
		case AWK_NUMBER:
			val->val_type = AWK_NUMBER;

			(void) force_number(node);
			if (node->flags & NUMCUR) {
				val->num_value = get_number_d(node);
				ret = true;
			}
			break;

		case AWK_STRING:
			val->val_type = AWK_STRING;

			(void) force_string(node);
			if (node->flags & STRCUR) {
				val->str_value.str = node->stptr;
				val->str_value.len = node->stlen;
				ret = true;
			}
			break;

		case AWK_UNDEFINED:
			/* return true and actual type for request of undefined */
			if (node->flags & NUMBER) {
				val->val_type = AWK_NUMBER;
				val->num_value = get_number_d(node);
				ret = true;
			} else if (node->flags & STRING) {
				val->val_type = AWK_STRING;
				val->str_value.str = node->stptr;
				val->str_value.len = node->stlen;
				ret = true;
			} else
				val->val_type = AWK_UNDEFINED;
			break;

		case AWK_ARRAY:
			break;
		}
		break;

	case Node_var_array:
		val->val_type = AWK_ARRAY;
		if (wanted == AWK_ARRAY || wanted == AWK_UNDEFINED) {
			val->array_cookie = node;
			ret = true;
		} else {
			ret = false;
		}
		break;

	default:
		val->val_type = AWK_UNDEFINED;
		ret = false;
		break;
	}

	return ret;
}

/*
 * Symbol table access:
 * 	- No access to special variables (NF, etc.)
 * 	- One special exception: PROCINFO.
 *	- Use sym_update() to change a value, including from UNDEFINED
 *	  to scalar or array. 
 */
/*
 * Lookup a variable, fills in value. No messing with the value
 * returned. Returns false if the variable doesn't exist
 * or the wrong type was requested.
 * In the latter case, fills in vaule->val_type with the real type.
 * Built-in variables (except PROCINFO) may not be accessed by an extension.
 */

/* api_sym_lookup --- look up a symbol */

static awk_bool_t
api_sym_lookup(awk_ext_id_t id,
		const char *name,
		awk_valtype_t wanted,
		awk_value_t *result)
{
	NODE *node;

	if (   name == NULL
	    || *name == '\0'
	    || result == NULL
	    || is_off_limits_var(name)	/* most built-in vars not allowed */
	    || (node = lookup(name)) == NULL)
		return false;

	return node_to_awk_value(node, result, wanted);
}

/* api_sym_update --- update a symbol's value, see gawkapi.h for semantics */

static awk_bool_t
api_sym_update(awk_ext_id_t id, const char *name, awk_value_t *value)
{
	NODE *node;
	NODE *array_node;

	if (   name == NULL
	    || *name == '\0'
	    || is_off_limits_var(name)	/* most built-in vars not allowed */
	    || value == NULL)
		return false;

	switch (value->val_type) {
	case AWK_NUMBER:
	case AWK_STRING:
	case AWK_UNDEFINED:
	case AWK_ARRAY:
		break;

	default:
		/* fatal(_("api_sym_update: invalid value for type of new value (%d)"), value->val_type); */
		return false;
	}

	node = lookup(name);

	if (node == NULL) {
		/* new value to be installed */
		if (value->val_type == AWK_ARRAY) {
			array_node = awk_value_to_node(value);
			node = install_symbol(estrdup((char *) name, strlen(name)),
					Node_var_array);
			array_node->vname = node->vname;
			*node = *array_node;
			freenode(array_node);
		} else {
			/* regular variable */
			node = install_symbol(estrdup((char *) name, strlen(name)),
					Node_var);
			unref(node->var_value);
			node->var_value = awk_value_to_node(value);
		}
		return true;
	}

	/* if we get here, then it exists already */
	switch (value->val_type) {
	case AWK_STRING:
	case AWK_NUMBER:
		if (node->type == Node_var || node->type == Node_var_new) {
			unref(node->var_value);
			node->var_value = awk_value_to_node(value);
		} else {
			return false;
		}
		break;

	case AWK_ARRAY:
	case AWK_UNDEFINED:
		return false;	/* not allowed */
	}

	return true;
}

/* Array management */
/*
 * Return the value of an element - read only!
 * Use set_array_element to change it.
 */
static awk_bool_t
api_get_array_element(awk_ext_id_t id,
		awk_array_t a_cookie,
		const awk_value_t *const index,
		awk_valtype_t wanted,
		awk_value_t *result)
{
	NODE *array = (NODE *) a_cookie;
	NODE *subscript;
	NODE **aptr;

	/* don't check for index len zero, null str is ok as index */
	if (   array == NULL
	    || array->type != Node_var_array
	    || result == NULL
	    || index == NULL
	    || index->val_type != AWK_STRING
	    || index->str_value.str == NULL)
		return false;

	subscript = awk_value_to_node(index);
	aptr = assoc_lookup(array, subscript);
	unref(subscript);
	if (aptr == NULL)
		return false;

	return node_to_awk_value(*aptr, result, wanted);
}

/*
 * Change (or create) element in existing array with
 * element->index and element->value.
 */
static awk_bool_t
api_set_array_element(awk_ext_id_t id, awk_array_t a_cookie,
					const awk_value_t *const index,
					const awk_value_t *const value)
{
	NODE *array = (NODE *)a_cookie;
	NODE *tmp;
	NODE *elem;
	NODE **aptr;

	/* don't check for index len zero, null str is ok as index */
	if (   array == NULL
	    || array->type != Node_var_array
	    || index == NULL
	    || value == NULL
	    || index->str_value.str == NULL)
		return false;

	tmp = make_string(index->str_value.str, index->str_value.len);
	aptr = assoc_lookup(array, tmp);
	unref(tmp);
	unref(*aptr);
	elem = *aptr = awk_value_to_node(value);
	if (elem->type == Node_var_array) {
		elem->parent_array = array;
		elem->vname = estrdup(index->str_value.str,
					index->str_value.len);
		make_aname(elem);
	}

	return true;
}

/*
 * Remove the element with the given index.
 * Returns success if removed or if element did not exist.
 */
static awk_bool_t
api_del_array_element(awk_ext_id_t id,
		awk_array_t a_cookie, const awk_value_t* const index)
{
	NODE *array, *sub, *val;

	array = (NODE *) a_cookie;
	if (   array == NULL
	    || array->type != Node_var_array
	    || index == NULL
	    || index->val_type != AWK_STRING)
		return false;

	sub = awk_value_to_node(index);
	val = in_array(array, sub);

	if (val == NULL)
		return false;

	if (val->type == Node_var_array) {
		assoc_clear(val);
		/* cleared a sub-array, free Node_var_array */
		efree(val->vname);
		freenode(val);
	} else
		unref(val);

	(void) assoc_remove(array, sub);
	unref(sub);

	return true;
}

/*
 * Retrieve total number of elements in array.
 * Returns false if some kind of error.
 */
static awk_bool_t
api_get_element_count(awk_ext_id_t id,
		awk_array_t a_cookie, size_t *count)
{
	NODE *node = (NODE *) a_cookie;

	if (count == NULL || node == NULL || node->type != Node_var_array)
		return false;

	*count = node->table_size;
	return true;
}

/* Create a new array cookie to which elements may be added */
static awk_array_t
api_create_array(awk_ext_id_t id)
{
	NODE *n;

	getnode(n);
	memset(n, 0, sizeof(NODE));
	init_array(n);

	return (awk_array_t) n;
}

/* Clear out an array */
static awk_bool_t
api_clear_array(awk_ext_id_t id, awk_array_t a_cookie)
{
	NODE *node = (NODE *) a_cookie;

	if (node == NULL || node->type != Node_var_array)
		return false;

	assoc_clear(node);
	return true;
}

/* api_flatten_array --- flatten out an array so that it can be looped over easily. */

static awk_bool_t
api_flatten_array(awk_ext_id_t id,
		awk_array_t a_cookie,
		awk_flat_array_t **data)
{
	NODE **list;
	size_t i, j;
	NODE *array = (NODE *) a_cookie;
	size_t alloc_size;

	if (   array == NULL
	    || array->type != Node_var_array
	    || array->table_size == 0
	    || data == NULL)
		return false;

	alloc_size = sizeof(awk_flat_array_t) +
			(array->table_size - 1) * sizeof(awk_element_t);

	emalloc(*data, awk_flat_array_t *, alloc_size,
			"api_flatten_array");
	memset(*data, 0, alloc_size);

	list = assoc_list(array, "@unsorted", ASORTI);

	(*data)->opaque1 = array;
	(*data)->opaque2 = list;
	(*data)->count = array->table_size;

	for (i = j = 0; i < 2 * array->table_size; i += 2, j++) {
		NODE *index, *value;

		index = force_string(list[i]);
		value = list[i + 1]; /* number or string or subarray */

		/* convert index and value to ext types */
		if (! node_to_awk_value(index,
				& (*data)->elements[j].index, AWK_UNDEFINED)) {
			fatal(_("api_flatten_array: could not convert index %d\n"),
						(int) i);
		}
		if (! node_to_awk_value(value,
				& (*data)->elements[j].value, AWK_UNDEFINED)) {
			fatal(_("api_flatten_array: could not convert value %d\n"),
						(int) i);
		}
	}
	return true;
}

/*
 * When done, release the memory, delete any marked elements
 * Count must match what gawk thinks the size is.
 */
static awk_bool_t
api_release_flattened_array(awk_ext_id_t id,
		awk_array_t a_cookie,
		awk_flat_array_t *data)
{
	NODE *array = a_cookie;
	NODE **list;
	size_t i;

	if (   array == NULL
	    || array->type != Node_var_array
	    || data == NULL
	    || array != (NODE *) data->opaque1
	    || data->count != array->table_size
	    || data->opaque2 == NULL)
		return false;

	list = (NODE **) data->opaque2;

	/* Delete items flagged for delete. */
	for (i = 0; i < data->count; i++) {
		if ((data->elements[i].flags & AWK_ELEMENT_DELETE) != 0) {
			/* let the other guy do the work */
			(void) api_del_array_element(id, a_cookie,
					& data->elements[i].index);
		}
	}

	/* free index nodes */
	for (i = 0; i < 2 * array->table_size; i += 2) {
		unref(list[i]);
	}

	efree(list);
	efree(data);

	return true;
}

gawk_api_t api_impl = {
	GAWK_API_MAJOR_VERSION,	/* major and minor versions */
	GAWK_API_MINOR_VERSION,
	{ 0 },			/* do_flags */

	api_get_argument,
	api_set_argument,

	api_fatal,
	api_warning,
	api_lintwarn,

	api_register_open_hook,

	api_update_ERRNO_int,
	api_update_ERRNO_string,
	api_unset_ERRNO,

	api_add_ext_func,

	api_awk_atexit,

	api_sym_lookup,
	api_sym_update,

	api_get_array_element,
	api_set_array_element,
	api_del_array_element,
	api_get_element_count,
	api_create_array,
	api_clear_array,
	api_flatten_array,
	api_release_flattened_array,
};

/* init_ext_api --- init the extension API */

void
init_ext_api()
{
	/* force values to 1 / 0 */
	api_impl.do_flags[0] = (do_lint ? 1 : 0);
	api_impl.do_flags[1] = (do_traditional ? 1 : 0);
	api_impl.do_flags[2] = (do_profile ? 1 : 0);
	api_impl.do_flags[3] = (do_sandbox ? 1 : 0);
	api_impl.do_flags[4] = (do_debug ? 1 : 0);
	api_impl.do_flags[5] = (do_mpfr ? 1 : 0);
}

/* update_ext_api --- update the variables in the API that can change */

void
update_ext_api()
{
	api_impl.do_flags[0] = (do_lint ? 1 : 0);
}
