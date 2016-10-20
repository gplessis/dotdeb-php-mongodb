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


PHONGO_API zend_class_entry *php_phongo_writeerror_ce;

zend_object_handlers php_phongo_handler_writeerror;

/* {{{ proto integer WriteError::getCode()
   Returns the MongoDB error code */
PHP_METHOD(WriteError, getCode)
{
	php_phongo_writeerror_t  *intern;


	intern = Z_WRITEERROR_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}


	RETURN_LONG(intern->code);
}
/* }}} */
/* {{{ proto integer WriteError::getIndex()
   Returns the Bulk index where this WriteError occurred in */
PHP_METHOD(WriteError, getIndex)
{
	php_phongo_writeerror_t  *intern;


	intern = Z_WRITEERROR_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}


	RETURN_LONG(intern->index);
}
/* }}} */
/* {{{ proto string WriteError::getMessage()
   Returns the actual error message from the server */
PHP_METHOD(WriteError, getMessage)
{
	php_phongo_writeerror_t  *intern;


	intern = Z_WRITEERROR_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}


	PHONGO_RETURN_STRING(intern->message);
}
/* }}} */
/* {{{ proto mixed WriteError::getInfo()
   Returns additional metadata for the error */
PHP_METHOD(WriteError, getInfo)
{
	php_phongo_writeerror_t *intern;


	intern = Z_WRITEERROR_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}


	if (!Z_ISUNDEF(intern->info)) {
#if PHP_VERSION_ID >= 70000
		RETURN_ZVAL(&intern->info, 1, 0);
#else
		RETURN_ZVAL(intern->info, 1, 0);
#endif
	}
}
/* }}} */

/**
 * Value object for a write error (e.g. duplicate key).
 */
/* {{{ MongoDB\Driver\WriteError */

ZEND_BEGIN_ARG_INFO_EX(ai_WriteError_getCode, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(ai_WriteError_getIndex, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(ai_WriteError_getInfo, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(ai_WriteError_getMessage, 0, 0, 0)
ZEND_END_ARG_INFO();


static zend_function_entry php_phongo_writeerror_me[] = {
	PHP_ME(Server, __construct, NULL, ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(WriteError, getCode, ai_WriteError_getCode, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(WriteError, getIndex, ai_WriteError_getIndex, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(WriteError, getMessage, ai_WriteError_getMessage, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(WriteError, getInfo, ai_WriteError_getInfo, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Manager, __wakeUp, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* }}} */


/* {{{ php_phongo_writeerror_t object handlers */
static void php_phongo_writeerror_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_writeerror_t *intern = Z_OBJ_WRITEERROR(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	if (intern->message) {
		efree(intern->message);
	}

	if (!Z_ISUNDEF(intern->info)) {
		zval_ptr_dtor(&intern->info);
	}

#if PHP_VERSION_ID < 70000
	efree(intern);
#endif
} /* }}} */

phongo_create_object_retval php_phongo_writeerror_create_object(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	php_phongo_writeerror_t *intern = NULL;

	intern = PHONGO_ALLOC_OBJECT_T(php_phongo_writeerror_t, class_type);

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

#if PHP_VERSION_ID >= 70000
	intern->std.handlers = &php_phongo_handler_writeerror;

	return &intern->std;
#else
	{
		zend_object_value retval;
		retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_phongo_writeerror_free_object, NULL TSRMLS_CC);
		retval.handlers = &php_phongo_handler_writeerror;

		return retval;
	}
#endif
} /* }}} */

HashTable *php_phongo_writeerror_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	php_phongo_writeerror_t *intern;
#if PHP_VERSION_ID >= 70000
	zval                     retval;
#else
	zval                     retval = zval_used_for_init;
#endif


	*is_temp = 1;
	intern = Z_WRITEERROR_OBJ_P(object);

	array_init_size(&retval, 3);
	ADD_ASSOC_STRING(&retval, "message", intern->message);
	ADD_ASSOC_LONG_EX(&retval, "code", intern->code);
	ADD_ASSOC_LONG_EX(&retval, "index", intern->index);
	if (!Z_ISUNDEF(intern->info)) {
#if PHP_VERSION_ID >= 70000
		Z_ADDREF(intern->info);
		ADD_ASSOC_ZVAL_EX(&retval, "info", &intern->info);
#else
		Z_ADDREF_P(intern->info);
		ADD_ASSOC_ZVAL_EX(&retval, "info", intern->info);
#endif
	} else {
		ADD_ASSOC_NULL_EX(&retval, "info");
	}

	return Z_ARRVAL(retval);
} /* }}} */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(WriteError)
{
	zend_class_entry ce;
	(void)type;(void)module_number;

	INIT_NS_CLASS_ENTRY(ce, "MongoDB\\Driver", "WriteError", php_phongo_writeerror_me);
	php_phongo_writeerror_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_writeerror_ce->create_object = php_phongo_writeerror_create_object;
	PHONGO_CE_INIT(php_phongo_writeerror_ce);

	memcpy(&php_phongo_handler_writeerror, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_phongo_handler_writeerror.get_debug_info = php_phongo_writeerror_get_debug_info;
#if PHP_VERSION_ID >= 70000
	php_phongo_handler_writeerror.free_obj = php_phongo_writeerror_free_object;
	php_phongo_handler_writeerror.offset = XtOffsetOf(php_phongo_writeerror_t, std);
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
