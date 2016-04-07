/*
  +---------------------------------------------------------------------------+
  | PHP Driver for MongoDB                                                    |
  +---------------------------------------------------------------------------+
  | Copyright 2013-2015 MongoDB, Inc.                                         |
  |                                                                           |
  | Licensed under the Apache License, Version 2.0 (the "License");           |
  | you may not use this file except in compliance with the License.          |
  | You may obtain a copy of the License at                                   |
  |                                                                           |
  | http://www.apache.org/licenses/LICENSE-2.0                                |
  |                                                                           |
  | Unless required by applicable law or agreed to in writing, software       |
  | distributed under the License is distributed on an "AS IS" BASIS,         |
  | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  |
  | See the License for the specific language governing permissions and       |
  | limitations under the License.                                            |
  +---------------------------------------------------------------------------+
  | Copyright (c) 2014-2015 MongoDB, Inc.                                     |
  +---------------------------------------------------------------------------+
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
/* Our Compatability header */
#include "phongo_compat.h"

/* Our stuffz */
#include "php_phongo.h"
#include "php_bson.h"


PHONGO_API zend_class_entry *php_phongo_javascript_ce;

zend_object_handlers php_phongo_handler_javascript;

/* {{{ proto BSON\Javascript Javascript::__construct(string $javascript[, array|object $document])
 * The string is JavaScript code. The document is a mapping from identifiers to values, representing the scope in which the string should be evaluated
 * NOTE: eJSON does not support this type :( */
PHP_METHOD(Javascript, __construct)
{
	php_phongo_javascript_t   *intern;
	zend_error_handling        error_handling;
	char                      *javascript;
	phongo_zpp_char_len        javascript_len;
	zval                      *document = NULL;
	bson_t                     scope = BSON_INITIALIZER;


	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling TSRMLS_CC);
	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|A!", &javascript, &javascript_len, &document) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	if (document) {
		phongo_zval_to_bson(document, PHONGO_BSON_NONE, &scope, NULL TSRMLS_CC);
	}

	php_phongo_new_javascript_from_javascript_and_scope(0, getThis(), javascript, javascript_len, &scope TSRMLS_CC);
	bson_destroy(&scope);
}
/* }}} */

/* {{{ BSON\Javascript */

ZEND_BEGIN_ARG_INFO_EX(ai_Javascript___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, javascript)
	ZEND_ARG_INFO(0, scope)
ZEND_END_ARG_INFO();

static zend_function_entry php_phongo_javascript_me[] = {
	PHP_ME(Javascript, __construct, ai_Javascript___construct, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Manager, __wakeUp, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* }}} */


/* {{{ php_phongo_javascript_t object handlers */
static void php_phongo_javascript_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_javascript_t *intern = Z_OBJ_JAVASCRIPT(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	if (intern->javascript) {
		efree(intern->javascript);
	}
	if (intern->document) {
		bson_destroy(intern->document);
		intern->document = NULL;
	}

#if PHP_VERSION_ID < 70000
	efree(intern);
#endif
} /* }}} */

phongo_create_object_retval php_phongo_javascript_create_object(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	php_phongo_javascript_t *intern = NULL;

	intern = PHONGO_ALLOC_OBJECT_T(php_phongo_javascript_t, class_type);
	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

	intern->document = NULL;

#if PHP_VERSION_ID >= 70000
	intern->std.handlers = &php_phongo_handler_javascript;

	return &intern->std;
#else
	{
		zend_object_value retval;
		retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_phongo_javascript_free_object, NULL TSRMLS_CC);
		retval.handlers = &php_phongo_handler_javascript;

		return retval;
	}
#endif
} /* }}} */

HashTable *php_phongo_javascript_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	php_phongo_javascript_t *intern;
#if PHP_VERSION_ID >= 70000
	zval                     retval;
#else
	zval                     retval = zval_used_for_init;
#endif


	*is_temp = 1;
	intern =  Z_JAVASCRIPT_OBJ_P(object);

	array_init(&retval);

	ADD_ASSOC_STRINGL(&retval, "javascript", intern->javascript, intern->javascript_len);

	if (intern->document) {
		php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;

		if (phongo_bson_to_zval_ex(bson_get_data(intern->document), intern->document->len, &state)) {
#if PHP_VERSION_ID >= 70000
			Z_ADDREF(state.zchild);
			ADD_ASSOC_ZVAL_EX(&retval, "scope", &state.zchild);
#else
			Z_ADDREF_P(state.zchild);
			ADD_ASSOC_ZVAL_EX(&retval, "scope", state.zchild);
#endif
		} else {
			ADD_ASSOC_NULL_EX(&retval, "scope");
		}

		zval_ptr_dtor(&state.zchild);
	}

	return Z_ARRVAL(retval);
} /* }}} */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(Javascript)
{
	zend_class_entry ce;
	(void)type;(void)module_number;


	INIT_NS_CLASS_ENTRY(ce, BSON_NAMESPACE, "Javascript", php_phongo_javascript_me);
	php_phongo_javascript_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_javascript_ce->create_object = php_phongo_javascript_create_object;
	PHONGO_CE_INIT(php_phongo_javascript_ce);

	zend_class_implements(php_phongo_javascript_ce TSRMLS_CC, 1, php_phongo_type_ce);

	memcpy(&php_phongo_handler_javascript, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_phongo_handler_javascript.get_debug_info = php_phongo_javascript_get_debug_info;
#if PHP_VERSION_ID >= 70000
	php_phongo_handler_javascript.free_obj = php_phongo_javascript_free_object;
	php_phongo_handler_javascript.offset = XtOffsetOf(php_phongo_javascript_t, std);
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
