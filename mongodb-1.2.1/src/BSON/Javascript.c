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
#include <ext/standard/php_var.h>
#if PHP_VERSION_ID >= 70000
# include <zend_smart_str.h>
#else
# include <ext/standard/php_smart_str.h>
#endif
/* Our Compatability header */
#include "phongo_compat.h"

/* Our stuffz */
#include "php_phongo.h"
#include "php_bson.h"


PHONGO_API zend_class_entry *php_phongo_javascript_ce;

zend_object_handlers php_phongo_handler_javascript;

/* Initialize the object and return whether it was successful. An exception will
 * be thrown on error. */
static bool php_phongo_javascript_init(php_phongo_javascript_t *intern, const char *code, phongo_zpp_char_len code_len, zval *scope TSRMLS_DC)
{
	if (scope && Z_TYPE_P(scope) != IS_OBJECT && Z_TYPE_P(scope) != IS_ARRAY && Z_TYPE_P(scope) != IS_NULL) {
		phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Expected scope to be array or object, %s given", zend_get_type_by_const(Z_TYPE_P(scope)));
		return false;
	}

	if (strlen(code) != code_len) {
		phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "Code cannot contain null bytes");
		return false;
	}

	intern->code = estrndup(code, code_len);
	intern->code_len = code_len;

	if (scope && (Z_TYPE_P(scope) == IS_OBJECT || Z_TYPE_P(scope) == IS_ARRAY)) {
		intern->scope = bson_new();
		phongo_zval_to_bson(scope, PHONGO_BSON_NONE, intern->scope, NULL TSRMLS_CC);
	} else {
		intern->scope = NULL;
	}

	return true;
}

/* Initialize the object from a HashTable and return whether it was successful.
 * An exception will be thrown on error. */
static bool php_phongo_javascript_init_from_hash(php_phongo_javascript_t *intern, HashTable *props TSRMLS_DC)
{
#if PHP_VERSION_ID >= 70000
	zval *code, *scope;

	if ((code = zend_hash_str_find(props, "code", sizeof("code")-1)) && Z_TYPE_P(code) == IS_STRING) {
		scope = zend_hash_str_find(props, "scope", sizeof("scope")-1);

		return php_phongo_javascript_init(intern, Z_STRVAL_P(code), Z_STRLEN_P(code), scope TSRMLS_CC);
	}
#else
	zval **code, **scope;

	if (zend_hash_find(props, "code", sizeof("code"), (void**) &code) == SUCCESS && Z_TYPE_PP(code) == IS_STRING) {
		zval *tmp = zend_hash_find(props, "scope", sizeof("scope"), (void**) &scope) == SUCCESS ? *scope : NULL;

		return php_phongo_javascript_init(intern, Z_STRVAL_PP(code), Z_STRLEN_PP(code), tmp TSRMLS_CC);
	}
#endif

	phongo_throw_exception(PHONGO_ERROR_INVALID_ARGUMENT TSRMLS_CC, "%s initialization requires \"code\" string field", ZSTR_VAL(php_phongo_javascript_ce->name));
	return false;
}

/* {{{ proto void Javascript::__construct(string $code[, array|object $scope])
   Construct a new BSON Javascript type. The scope is a document mapping
   identifiers and values, representing the scope in which the code string will
   be evaluated. Note that this type cannot be represented as Extended JSON. */
PHP_METHOD(Javascript, __construct)
{
	php_phongo_javascript_t   *intern;
	zend_error_handling        error_handling;
	char                      *code;
	phongo_zpp_char_len        code_len;
	zval                      *scope = NULL;


	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling TSRMLS_CC);
	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|A!", &code, &code_len, &scope) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

	php_phongo_javascript_init(intern, code, code_len, scope TSRMLS_CC);
}
/* }}} */

/* {{{ proto void Javascript::__set_state(array $properties)
*/
PHP_METHOD(Javascript, __set_state)
{
	php_phongo_javascript_t *intern;
	HashTable               *props;
	zval                    *array;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &array) == FAILURE) {
		RETURN_FALSE;
	}

	object_init_ex(return_value, php_phongo_javascript_ce);

	intern = Z_JAVASCRIPT_OBJ_P(return_value);
	props = Z_ARRVAL_P(array);

	php_phongo_javascript_init_from_hash(intern, props TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Javascript::__toString()
   Return the Javascript's code string. */
PHP_METHOD(Javascript, __toString)
{
	php_phongo_javascript_t *intern;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	PHONGO_RETURN_STRINGL(intern->code, intern->code_len);
}
/* }}} */

/* {{{ proto string Javascript::getCode()
*/
PHP_METHOD(Javascript, getCode)
{
	php_phongo_javascript_t *intern;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	PHONGO_RETURN_STRINGL(intern->code, intern->code_len);
}
/* }}} */

/* {{{ proto object|null Javascript::getScope()
*/
PHP_METHOD(Javascript, getScope)
{
	php_phongo_javascript_t *intern;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	if (!intern->scope) {
		RETURN_NULL();
	}

	if (intern->scope->len) {
		php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;

		phongo_bson_to_zval_ex(bson_get_data(intern->scope), intern->scope->len, &state);
#if PHP_VERSION_ID >= 70000
		RETURN_ZVAL(&state.zchild, 0, 1);
#else
		RETURN_ZVAL(state.zchild, 0, 1);
#endif
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto array Javascript::jsonSerialize()
*/
PHP_METHOD(Javascript, jsonSerialize)
{
	php_phongo_javascript_t *intern;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	array_init_size(return_value, 2);
	ADD_ASSOC_STRINGL(return_value, "$code", intern->code, intern->code_len);

	if (intern->scope && intern->scope->len) {
		php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;

		if (phongo_bson_to_zval_ex(bson_get_data(intern->scope), intern->scope->len, &state)) {
#if PHP_VERSION_ID >= 70000
			Z_ADDREF(state.zchild);
			ADD_ASSOC_ZVAL_EX(return_value, "$scope", &state.zchild);
#else
			Z_ADDREF_P(state.zchild);
			ADD_ASSOC_ZVAL_EX(return_value, "$scope", state.zchild);
#endif
		}

		zval_ptr_dtor(&state.zchild);
	}
}
/* }}} */

/* {{{ proto string Javascript::serialize()
*/
PHP_METHOD(Javascript, serialize)
{
	php_phongo_javascript_t  *intern;
#if PHP_VERSION_ID >= 70000
	zval                      retval;
#else
	zval                     *retval;
#endif
	php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;

	php_serialize_data_t      var_hash;
	smart_str                 buf = { 0 };

	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

#if PHP_VERSION_ID >= 70000
	if (intern->scope && intern->scope->len) {
		if (!phongo_bson_to_zval_ex(bson_get_data(intern->scope), intern->scope->len, &state)) {
			return;
		}
		Z_ADDREF(state.zchild);
	} else {
		ZVAL_NULL(&state.zchild);
	}
#else
	if (intern->scope && intern->scope->len) {
		if (!phongo_bson_to_zval_ex(bson_get_data(intern->scope), intern->scope->len, &state)) {
			return;
		}
		Z_ADDREF_P(state.zchild);
	} else {
		MAKE_STD_ZVAL(state.zchild);
		ZVAL_NULL(state.zchild);
		Z_ADDREF_P(state.zchild);
	}
#endif

#if PHP_VERSION_ID >= 70000
	array_init_size(&retval, 2);
	ADD_ASSOC_STRINGL(&retval, "code", intern->code, intern->code_len);
	ADD_ASSOC_ZVAL(&retval, "scope", &state.zchild);
#else
	ALLOC_INIT_ZVAL(retval);
	array_init_size(retval, 2);
	ADD_ASSOC_STRINGL(retval, "code", intern->code, intern->code_len);
	ADD_ASSOC_ZVAL(retval, "scope", state.zchild);
#endif

	PHP_VAR_SERIALIZE_INIT(var_hash);
	php_var_serialize(&buf, &retval, &var_hash TSRMLS_CC);
	smart_str_0(&buf);
	PHP_VAR_SERIALIZE_DESTROY(var_hash);

	PHONGO_RETVAL_SMART_STR(buf);

	smart_str_free(&buf);
	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&state.zchild);
}
/* }}} */

/* {{{ proto string Javascript::unserialize(string $serialized)
*/
PHP_METHOD(Javascript, unserialize)
{
	php_phongo_javascript_t *intern;
	zend_error_handling      error_handling;
	char                    *serialized;
	phongo_zpp_char_len      serialized_len;
#if PHP_VERSION_ID >= 70000
	zval                     props;
#else
	zval                    *props;
#endif
	php_unserialize_data_t   var_hash;

	intern = Z_JAVASCRIPT_OBJ_P(getThis());

	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &serialized, &serialized_len) == FAILURE) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		return;
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);

#if PHP_VERSION_ID < 70000
	ALLOC_INIT_ZVAL(props);
#endif
	PHP_VAR_UNSERIALIZE_INIT(var_hash);
	if (!php_var_unserialize(&props, (const unsigned char**) &serialized, (unsigned char *) serialized + serialized_len, &var_hash TSRMLS_CC)) {
		zval_ptr_dtor(&props);
		phongo_throw_exception(PHONGO_ERROR_UNEXPECTED_VALUE TSRMLS_CC, "%s unserialization failed", ZSTR_VAL(php_phongo_javascript_ce->name));

		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
		return;
	}
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

#if PHP_VERSION_ID >= 70000
	php_phongo_javascript_init_from_hash(intern, HASH_OF(&props) TSRMLS_CC);
#else
	php_phongo_javascript_init_from_hash(intern, HASH_OF(props) TSRMLS_CC);
#endif
	zval_ptr_dtor(&props);
}
/* }}} */

/* {{{ BSON\Javascript */

ZEND_BEGIN_ARG_INFO_EX(ai_Javascript___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, javascript)
	ZEND_ARG_INFO(0, scope)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Javascript___set_state, 0, 0, 1)
	ZEND_ARG_ARRAY_INFO(0, properties, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Javascript_unserialize, 0, 0, 1)
	ZEND_ARG_INFO(0, serialized)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Javascript_void, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_phongo_javascript_me[] = {
	PHP_ME(Javascript, __construct, ai_Javascript___construct, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Javascript, __set_state, ai_Javascript___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(Javascript, __toString, ai_Javascript_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Javascript, jsonSerialize, ai_Javascript_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Javascript, serialize, ai_Javascript_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Javascript, unserialize, ai_Javascript_unserialize, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Javascript, getCode, ai_Javascript_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Javascript, getScope, ai_Javascript_void, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_FE_END
};

/* }}} */


/* {{{ php_phongo_javascript_t object handlers */
static void php_phongo_javascript_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_javascript_t *intern = Z_OBJ_JAVASCRIPT(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	if (intern->code) {
		efree(intern->code);
	}
	if (intern->scope) {
		bson_destroy(intern->scope);
		intern->scope = NULL;
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

static int php_phongo_javascript_compare_objects(zval *o1, zval *o2 TSRMLS_DC) /* {{{ */
{
	php_phongo_javascript_t *intern1, *intern2;

	intern1 = Z_JAVASCRIPT_OBJ_P(o1);
	intern2 = Z_JAVASCRIPT_OBJ_P(o2);

	/* Do not consider the scope document for comparisons */
	return strcmp(intern1->code, intern2->code);
} /* }}} */

HashTable *php_phongo_javascript_get_properties(zval *object TSRMLS_DC) /* {{{ */
{
	php_phongo_javascript_t *intern;
	HashTable               *props;

	intern = Z_JAVASCRIPT_OBJ_P(object);
	props = zend_std_get_properties(object TSRMLS_CC);

	if (!intern->code) {
		return props;
	}

#if PHP_VERSION_ID >= 70000
	{
		zval code;

		ZVAL_STRING(&code, intern->code);
		zend_hash_str_update(props, "code", sizeof("code")-1, &code);

		if (intern->scope) {
			php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;

			if (phongo_bson_to_zval_ex(bson_get_data(intern->scope), intern->scope->len, &state)) {
				Z_ADDREF(state.zchild);
				zend_hash_str_update(props, "scope", sizeof("scope")-1, &state.zchild);
			} else {
				zval scope;

				ZVAL_NULL(&scope);
				zend_hash_str_update(props, "scope", sizeof("scope")-1, &scope);
			}

			zval_ptr_dtor(&state.zchild);
		} else {
			zval scope;

			ZVAL_NULL(&scope);
			zend_hash_str_update(props, "scope", sizeof("scope")-1, &scope);
		}
	}
#else
	{
		zval *code;

		MAKE_STD_ZVAL(code);
		ZVAL_STRING(code, intern->code, 1);
		zend_hash_update(props, "code", sizeof("code"), &code, sizeof(code), NULL);

		if (intern->scope) {
			php_phongo_bson_state state = PHONGO_BSON_STATE_INITIALIZER;

			if (phongo_bson_to_zval_ex(bson_get_data(intern->scope), intern->scope->len, &state)) {
				Z_ADDREF_P(state.zchild);
				zend_hash_update(props, "scope", sizeof("scope"), &state.zchild, sizeof(state.zchild), NULL);
			} else {
				zval *scope;

				MAKE_STD_ZVAL(scope);
				ZVAL_NULL(scope);
				zend_hash_update(props, "scope", sizeof("scope"), &scope, sizeof(scope), NULL);
			}

			zval_ptr_dtor(&state.zchild);
		} else {
			zval *scope;

			MAKE_STD_ZVAL(scope);
			ZVAL_NULL(scope);
			zend_hash_update(props, "scope", sizeof("scope"), &scope, sizeof(scope), NULL);
		}
	}
#endif

	return props;
} /* }}} */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(Javascript)
{
	zend_class_entry ce;
	(void)type;(void)module_number;


	INIT_NS_CLASS_ENTRY(ce, "MongoDB\\BSON", "Javascript", php_phongo_javascript_me);
	php_phongo_javascript_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_javascript_ce->create_object = php_phongo_javascript_create_object;
	PHONGO_CE_FINAL(php_phongo_javascript_ce);

	zend_class_implements(php_phongo_javascript_ce TSRMLS_CC, 1, php_json_serializable_ce);
	zend_class_implements(php_phongo_javascript_ce TSRMLS_CC, 1, php_phongo_type_ce);
	zend_class_implements(php_phongo_javascript_ce TSRMLS_CC, 1, zend_ce_serializable);

	memcpy(&php_phongo_handler_javascript, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_phongo_handler_javascript.compare_objects = php_phongo_javascript_compare_objects;
	php_phongo_handler_javascript.get_properties = php_phongo_javascript_get_properties;
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
