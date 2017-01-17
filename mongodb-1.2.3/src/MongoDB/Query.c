/*
 * Copyright 2014-2017 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

/* External libs */
#include <bson.h>
#include <mongoc.h>

/* PHP Core stuff */
#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#include <Zend/zend_interfaces.h>
#include <ext/spl/spl_iterators.h>

/* PHP array helpers */
#include "php_array_api.h"

/* Our Compatability header */
#include "phongo_compat.h"

/* Our stuffz */
#include "php_phongo.h"
#include "php_bson.h"


PHONGO_API zend_class_entry *php_phongo_query_ce;

zend_object_handlers php_phongo_handler_query;

/* Appends a string field into the BSON options. Returns true on
 * success; otherwise, false is returned and an exception is thrown. */
static bool php_phongo_query_opts_append_string(bson_t *opts, const char *opts_key, zval *zarr, const char *zarr_key TSRMLS_DC)
{
	zval *value = php_array_fetch(zarr, zarr_key);

	if (Z_TYPE_P(value) != IS_STRING) {
		phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected \"%s\" %s to be string, %s given", zarr_key, zarr_key[0] == '$' ? "modifier" : "option", zend_get_type_by_const(Z_TYPE_P(value)));
		return false;
	}

	if (!bson_append_utf8(opts, opts_key, strlen(opts_key), Z_STRVAL_P(value), Z_STRLEN_P(value))) {
		phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Error appending \"%s\" option", opts_key);
		return false;
	}

	return true;
}

/* Appends a document field for the given opts document and key. Returns true on
 * success; otherwise, false is returned and an exception is thrown. */
static bool php_phongo_query_opts_append_document(bson_t *opts, const char *opts_key, zval *zarr, const char *zarr_key TSRMLS_DC)
{
	zval *value = php_array_fetch(zarr, zarr_key);
	bson_t b = BSON_INITIALIZER;

	if (Z_TYPE_P(value) != IS_OBJECT && Z_TYPE_P(value) != IS_ARRAY) {
		phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected \"%s\" %s to be array or object, %s given", zarr_key, zarr_key[0] == '$' ? "modifier" : "option", zend_get_type_by_const(Z_TYPE_P(value)));
		return false;
	}

	phongo_zval_to_bson(value, PHONGO_BSON_NONE, &b, NULL TSRMLS_CC);

	if (EG(exception)) {
		bson_destroy(&b);
		return false;
	}

	if (!BSON_APPEND_DOCUMENT(opts, opts_key, &b)) {
		phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Error appending \"%s\" option", opts_key);
		bson_destroy(&b);
		return false;
	}

	bson_destroy(&b);
	return true;
}

#define PHONGO_QUERY_OPT_BOOL(opt, zarr, key) \
	if ((zarr) && php_array_existsc((zarr), (key))) { \
		if (!BSON_APPEND_BOOL(intern->opts, (opt), php_array_fetchc_bool((zarr), (key)))) { \
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Error appending \"%s\" option", (opt)); \
			return false; \
		} \
	}

#define PHONGO_QUERY_OPT_DOCUMENT(opt, zarr, key) \
	if ((zarr) && php_array_existsc((zarr), (key))) { \
		if (!php_phongo_query_opts_append_document(intern->opts, (opt), (zarr), (key) TSRMLS_CC)) { \
			return false; \
		} \
	}

	/* Note: handling of integer options will depend on SIZEOF_ZEND_LONG and we
	 * are not converting strings to 64-bit integers for 32-bit platforms. */

#define PHONGO_QUERY_OPT_INT64(opt, zarr, key) \
	if ((zarr) && php_array_existsc((zarr), (key))) { \
		if (!BSON_APPEND_INT64(intern->opts, (opt), php_array_fetchc_long((zarr), (key)))) { \
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Error appending \"%s\" option", (opt)); \
			return false; \
		} \
	}

#define PHONGO_QUERY_OPT_STRING(opt, zarr, key) \
	if ((zarr) && php_array_existsc((zarr), (key))) { \
		if (!php_phongo_query_opts_append_string(intern->opts, (opt), (zarr), (key) TSRMLS_CC)) { \
			return false; \
		} \
	}

/* Initialize the "hint" option. Returns true on success; otherwise, false is
 * returned and an exception is thrown.
 *
 * The "hint" option (or "$hint" modifier) must be a string or document. Check
 * for both types and merge into BSON options accordingly. */
static bool php_phongo_query_init_hint(php_phongo_query_t *intern, zval *options, zval *modifiers TSRMLS_DC) /* {{{ */
{
	/* The "hint" option (or "$hint" modifier) must be a string or document.
	 * Check for both types and merge into BSON options accordingly. */
	if (php_array_existsc(options, "hint")) {
		zend_uchar type = Z_TYPE_P(php_array_fetchc(options, "hint"));

		if (type == IS_STRING) {
			PHONGO_QUERY_OPT_STRING("hint", options, "hint");
		} else if (type == IS_OBJECT || type == IS_ARRAY) {
			PHONGO_QUERY_OPT_DOCUMENT("hint", options, "hint");
		} else {
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected \"hint\" option to be string, array, or object, %s given", zend_get_type_by_const(type));
			return false;
		}
	} else if (modifiers && php_array_existsc(modifiers, "$hint")) {
		zend_uchar type = Z_TYPE_P(php_array_fetchc(modifiers, "$hint"));

		if (type == IS_STRING) {
			PHONGO_QUERY_OPT_STRING("hint", modifiers, "$hint");
		} else if (type == IS_OBJECT || type == IS_ARRAY) {
			PHONGO_QUERY_OPT_DOCUMENT("hint", modifiers, "$hint");
		} else {
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected \"$hint\" modifier to be string, array, or object, %s given", zend_get_type_by_const(type));
			return false;
		}
	}

	return true;
} /* }}} */

/* Initialize the "limit" and "singleBatch" options. Returns true on success;
 * otherwise, false is returned and an exception is thrown.
 *
 * mongoc_collection_find_with_opts() requires a non-negative limit. For
 * backwards compatibility, a negative limit should be set as a positive value
 * and default singleBatch to true. */
static bool php_phongo_query_init_limit_and_singlebatch(php_phongo_query_t *intern, zval *options TSRMLS_DC) /* {{{ */
{
	if (php_array_existsc(options, "limit") && php_array_fetchc_long(options, "limit") < 0) {
		phongo_long limit = php_array_fetchc_long(options, "limit");

		if (!BSON_APPEND_INT64(intern->opts, "limit", -limit)) {
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Error appending \"limit\" option");
			return false;
		}

		if (php_array_existsc(options, "singleBatch") && !php_array_fetchc_bool(options, "singleBatch")) {
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Negative \"limit\" option conflicts with false \"singleBatch\" option");
			return false;
		} else {
			if (!BSON_APPEND_BOOL(intern->opts, "singleBatch", true)) {
				phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Error appending \"singleBatch\" option");
				return false;
			}
		}
	} else {
		PHONGO_QUERY_OPT_INT64("limit", options, "limit");
		PHONGO_QUERY_OPT_BOOL("singleBatch", options, "singleBatch");
	}

	return true;
} /* }}} */

/* Initialize the "readConcern" option. Returns true on success; otherwise,
 * false is returned and an exception is thrown.
 *
 * The "readConcern" option should be a MongoDB\Driver\ReadConcern instance,
 * which must be converted to a mongoc_read_concern_t. */
static bool php_phongo_query_init_readconcern(php_phongo_query_t *intern, zval *options TSRMLS_DC) /* {{{ */
{
	if (php_array_existsc(options, "readConcern")) {
		zval *read_concern = php_array_fetchc(options, "readConcern");

		if (Z_TYPE_P(read_concern) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(read_concern), php_phongo_readconcern_ce TSRMLS_CC)) {
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected \"readConcern\" option to be %s, %s given", ZSTR_VAL(php_phongo_readconcern_ce->name), zend_get_type_by_const(Z_TYPE_P(read_concern)));
			return false;
		}

		intern->read_concern = mongoc_read_concern_copy(phongo_read_concern_from_zval(read_concern TSRMLS_CC));
	}

	return true;
} /* }}} */

/* Initializes the php_phongo_query_t from filter and options arguments. This
 * function will fall back to a modifier in the absence of a top-level option
 * (where applicable). */
static bool php_phongo_query_init(php_phongo_query_t *intern, zval *filter, zval *options TSRMLS_DC) /* {{{ */
{
	zval *modifiers = NULL;

	intern->filter = bson_new();
	intern->opts = bson_new();

	phongo_zval_to_bson(filter, PHONGO_BSON_NONE, intern->filter, NULL TSRMLS_CC);

	/* Note: if any exceptions are thrown, we can simply return as PHP will
	 * invoke php_phongo_query_free_object to destruct the object. */
	if (EG(exception)) {
		return false;
	}

	if (!options) {
		return true;
	}

	if (php_array_existsc(options, "modifiers")) {
		modifiers = php_array_fetchc(options, "modifiers");

		if (Z_TYPE_P(modifiers) != IS_ARRAY) {
			phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected \"modifiers\" option to be array, %s given", zend_get_type_by_const(Z_TYPE_P(modifiers)));
			return false;
		}
	}

	PHONGO_QUERY_OPT_BOOL("allowPartialResults", options, "allowPartialResults")
		else PHONGO_QUERY_OPT_BOOL("allowPartialResults", options, "partial");
	PHONGO_QUERY_OPT_BOOL("awaitData", options, "awaitData");
	PHONGO_QUERY_OPT_INT64("batchSize", options, "batchSize");
	PHONGO_QUERY_OPT_DOCUMENT("collation", options, "collation");
	PHONGO_QUERY_OPT_STRING("comment", options, "comment")
		else PHONGO_QUERY_OPT_STRING("comment", modifiers, "$comment");
	PHONGO_QUERY_OPT_BOOL("exhaust", options, "exhaust");
	PHONGO_QUERY_OPT_DOCUMENT("max", options, "max")
		else PHONGO_QUERY_OPT_DOCUMENT("max", modifiers, "$max");
	PHONGO_QUERY_OPT_INT64("maxScan", options, "maxScan")
		else PHONGO_QUERY_OPT_INT64("maxScan", modifiers, "$maxScan");
	PHONGO_QUERY_OPT_INT64("maxTimeMS", options, "maxTimeMS")
		else PHONGO_QUERY_OPT_INT64("maxTimeMS", modifiers, "$maxTimeMS");
	PHONGO_QUERY_OPT_DOCUMENT("min", options, "min")
		else PHONGO_QUERY_OPT_DOCUMENT("min", modifiers, "$min");
	PHONGO_QUERY_OPT_BOOL("noCursorTimeout", options, "noCursorTimeout");
	PHONGO_QUERY_OPT_BOOL("oplogReplay", options, "oplogReplay");
	PHONGO_QUERY_OPT_DOCUMENT("projection", options, "projection");
	PHONGO_QUERY_OPT_BOOL("returnKey", options, "returnKey")
		else PHONGO_QUERY_OPT_BOOL("returnKey", modifiers, "$returnKey");
	PHONGO_QUERY_OPT_BOOL("showRecordId", options, "showRecordId")
		else PHONGO_QUERY_OPT_BOOL("showRecordId", modifiers, "$showDiskLoc");
	PHONGO_QUERY_OPT_INT64("skip", options, "skip");
	PHONGO_QUERY_OPT_DOCUMENT("sort", options, "sort")
		else PHONGO_QUERY_OPT_DOCUMENT("sort", modifiers, "$orderby");
	PHONGO_QUERY_OPT_BOOL("snapshot", options, "snapshot")
		else PHONGO_QUERY_OPT_BOOL("snapshot", modifiers, "$snapshot");
	PHONGO_QUERY_OPT_BOOL("tailable", options, "tailable");

	/* The "$explain" modifier should be converted to an "explain" option, which
	 * libmongoc will later convert back to a modifier for the OP_QUERY code
	 * path. This modifier will be ignored for the find command code path. */
	PHONGO_QUERY_OPT_BOOL("explain", modifiers, "$explain");

	if (!php_phongo_query_init_hint(intern, options, modifiers TSRMLS_CC)) {
		return false;
	}

	if (!php_phongo_query_init_limit_and_singlebatch(intern, options TSRMLS_CC)) {
		return false;
	}

	if (!php_phongo_query_init_readconcern(intern, options TSRMLS_CC)) {
		return false;
	}

	return true;
} /* }}} */

#undef PHONGO_QUERY_OPT_BOOL
#undef PHONGO_QUERY_OPT_DOCUMENT
#undef PHONGO_QUERY_OPT_INT64
#undef PHONGO_QUERY_OPT_STRING

/* {{{ proto void Query::__construct(array|object $filter[, array $options = array()])
   Constructs a new Query */
PHP_METHOD(Query, __construct)
{
	php_phongo_query_t  *intern;
	zend_error_handling  error_handling;
	zval                *filter;
	zval                *options = NULL;
	SUPPRESS_UNUSED_WARNING(return_value_ptr) SUPPRESS_UNUSED_WARNING(return_value) SUPPRESS_UNUSED_WARNING(return_value_used)


	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling TSRMLS_CC);
	intern = Z_QUERY_OBJ_P(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "A|a!", &filter, &options) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	php_phongo_query_init(intern, filter, options TSRMLS_CC);
}
/* }}} */

/* {{{ MongoDB\Driver\Query */

ZEND_BEGIN_ARG_INFO_EX(ai_Query___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, filter)
	ZEND_ARG_ARRAY_INFO(0, options, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Query_void, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_phongo_query_me[] = {
	PHP_ME(Query, __construct, ai_Query___construct, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Manager, __wakeup, ai_Query_void, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* }}} */


/* {{{ php_phongo_query_t object handlers */
static void php_phongo_query_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_query_t *intern = Z_OBJ_QUERY(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	if (intern->filter) {
		bson_clear(&intern->filter);
	}

	if (intern->opts) {
		bson_clear(&intern->opts);
	}

	if (intern->read_concern) {
		mongoc_read_concern_destroy(intern->read_concern);
	}

#if PHP_VERSION_ID < 70000
	efree(intern);
#endif
} /* }}} */

phongo_create_object_retval php_phongo_query_create_object(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	php_phongo_query_t *intern = NULL;

	intern = PHONGO_ALLOC_OBJECT_T(php_phongo_query_t, class_type);

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

#if PHP_VERSION_ID >= 70000
	intern->std.handlers = &php_phongo_handler_query;

	return &intern->std;
#else
	{
		zend_object_value retval;
		retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_phongo_query_free_object, NULL TSRMLS_CC);
		retval.handlers = &php_phongo_handler_query;

		return retval;
	}
#endif
} /* }}} */

HashTable *php_phongo_query_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	php_phongo_query_t    *intern;
#if PHP_VERSION_ID >= 70000
	zval                   retval;
#else
	zval                   retval = zval_used_for_init;
#endif


	*is_temp = 1;
	intern = Z_QUERY_OBJ_P(object);

	array_init_size(&retval, 3);

	/* Avoid using PHONGO_TYPEMAP_NATIVE_ARRAY for decoding filter and opts
	 * documents so that users can differentiate BSON arrays and documents. */
	if (intern->filter) {
#if PHP_VERSION_ID >= 70000
		zval zv;
#else
		zval *zv;
#endif

		phongo_bson_to_zval(bson_get_data(intern->filter), intern->filter->len, &zv);
#if PHP_VERSION_ID >= 70000
		ADD_ASSOC_ZVAL_EX(&retval, "filter", &zv);
#else
		ADD_ASSOC_ZVAL_EX(&retval, "filter", zv);
#endif
	} else {
		ADD_ASSOC_NULL_EX(&retval, "filter");
	}

	if (intern->opts) {
#if PHP_VERSION_ID >= 70000
		zval zv;
#else
		zval *zv;
#endif

		phongo_bson_to_zval(bson_get_data(intern->opts), intern->opts->len, &zv);
#if PHP_VERSION_ID >= 70000
		ADD_ASSOC_ZVAL_EX(&retval, "options", &zv);
#else
		ADD_ASSOC_ZVAL_EX(&retval, "options", zv);
#endif
	} else {
		ADD_ASSOC_NULL_EX(&retval, "options");
	}

	if (intern->read_concern) {
#if PHP_VERSION_ID >= 70000
		zval read_concern;

		php_phongo_read_concern_to_zval(&read_concern, intern->read_concern);
		ADD_ASSOC_ZVAL_EX(&retval, "readConcern", &read_concern);
#else
		zval *read_concern = NULL;
		MAKE_STD_ZVAL(read_concern);

		php_phongo_read_concern_to_zval(read_concern, intern->read_concern);
		ADD_ASSOC_ZVAL_EX(&retval, "readConcern", read_concern);
#endif
	} else {
		ADD_ASSOC_NULL_EX(&retval, "readConcern");
	}

	return Z_ARRVAL(retval);

} /* }}} */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(Query)
{
	zend_class_entry ce;
	(void)type;(void)module_number;

	INIT_NS_CLASS_ENTRY(ce, "MongoDB\\Driver", "Query", php_phongo_query_me);
	php_phongo_query_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_query_ce->create_object = php_phongo_query_create_object;
	PHONGO_CE_FINAL(php_phongo_query_ce);
	PHONGO_CE_DISABLE_SERIALIZATION(php_phongo_query_ce);

	memcpy(&php_phongo_handler_query, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_phongo_handler_query.get_debug_info = php_phongo_query_get_debug_info;
#if PHP_VERSION_ID >= 70000
	php_phongo_handler_query.free_obj = php_phongo_query_free_object;
	php_phongo_handler_query.offset = XtOffsetOf(php_phongo_query_t, std);
#endif

	return SUCCESS;
}
/* }}} */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
