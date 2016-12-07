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
#include <ext/json/php_json.h>
#include <ext/standard/info.h>
#include <Zend/zend_interfaces.h>
#include <ext/spl/spl_iterators.h>
/* Our Compatability header */
#include "phongo_compat.h"

/* Our stuffz */
#include "php_phongo.h"
#include "php_bson.h"


PHONGO_API zend_class_entry *php_phongo_minkey_ce;

zend_object_handlers php_phongo_handler_minkey;

/* {{{ proto MinKey::__set_state(array $properties)
*/
PHP_METHOD(MinKey, __set_state)
{
	zval *array;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &array) == FAILURE) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, php_phongo_minkey_ce);
}
/* }}} */

/* {{{ proto array MinKey::jsonSerialize()
*/
PHP_METHOD(MinKey, jsonSerialize)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init_size(return_value, 1);
	ADD_ASSOC_LONG_EX(return_value, "$minKey", 1);
}
/* }}} */

/* {{{ proto string MinKey::serialize()
*/
PHP_METHOD(MinKey, serialize)
{
	PHONGO_RETURN_STRING("");
}
/* }}} */

/* {{{ proto string MinKey::unserialize(string $serialized)
*/
PHP_METHOD(MinKey, unserialize)
{
	zend_error_handling     error_handling;
	char                   *serialized;
	phongo_zpp_char_len     serialized_len;

	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &serialized, &serialized_len) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);
}
/* }}} */

/* {{{ BSON\MinKey */

ZEND_BEGIN_ARG_INFO_EX(ai_MinKey___set_state, 0, 0, 1)
	ZEND_ARG_ARRAY_INFO(0, properties, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_MinKey_unserialize, 0, 0, 1)
	ZEND_ARG_INFO(0, serialized)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_MinKey_void, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_phongo_minkey_me[] = {
	PHP_ME(MinKey, __set_state, ai_MinKey___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(MinKey, jsonSerialize, ai_MinKey_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(MinKey, serialize, ai_MinKey_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(MinKey, unserialize, ai_MinKey_unserialize, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_FE_END
};

/* }}} */


/* {{{ php_phongo_minkey_t object handlers */
static void php_phongo_minkey_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_minkey_t *intern = Z_OBJ_MINKEY(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

#if PHP_VERSION_ID < 70000
	efree(intern);
#endif
} /* }}} */

phongo_create_object_retval php_phongo_minkey_create_object(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	php_phongo_minkey_t *intern = NULL;

	intern = PHONGO_ALLOC_OBJECT_T(php_phongo_minkey_t, class_type);

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

#if PHP_VERSION_ID >= 70000
	intern->std.handlers = &php_phongo_handler_minkey;

	return &intern->std;
#else
	{
		zend_object_value retval;
		retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_phongo_minkey_free_object, NULL TSRMLS_CC);
		retval.handlers = &php_phongo_handler_minkey;

		return retval;
	}
#endif
} /* }}} */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(MinKey)
{
	zend_class_entry ce;
	(void)type;(void)module_number;


	INIT_NS_CLASS_ENTRY(ce, "MongoDB\\BSON", "MinKey", php_phongo_minkey_me);
	php_phongo_minkey_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_minkey_ce->create_object = php_phongo_minkey_create_object;
	PHONGO_CE_FINAL(php_phongo_minkey_ce);

	zend_class_implements(php_phongo_minkey_ce TSRMLS_CC, 1, php_json_serializable_ce);
	zend_class_implements(php_phongo_minkey_ce TSRMLS_CC, 1, php_phongo_type_ce);
	zend_class_implements(php_phongo_minkey_ce TSRMLS_CC, 1, zend_ce_serializable);

	memcpy(&php_phongo_handler_minkey, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
#if PHP_VERSION_ID >= 70000
	php_phongo_handler_minkey.free_obj = php_phongo_minkey_free_object;
	php_phongo_handler_minkey.offset = XtOffsetOf(php_phongo_minkey_t, std);
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
