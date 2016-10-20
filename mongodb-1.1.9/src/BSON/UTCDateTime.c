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
#include <ext/date/php_date.h>
/* Our Compatability header */
#include "phongo_compat.h"

/* Our stuffz */
#include "php_phongo.h"
#include "php_bson.h"


PHONGO_API zend_class_entry *php_phongo_utcdatetime_ce;

zend_object_handlers php_phongo_handler_utcdatetime;

/* {{{ proto BSON\UTCDateTime UTCDateTime::__construct(integer $milliseconds)
   Construct a new UTCDateTime */
PHP_METHOD(UTCDateTime, __construct)
{
	php_phongo_utcdatetime_t    *intern;
	zend_error_handling       error_handling;


	zend_replace_error_handling(EH_THROW, phongo_exception_from_phongo_domain(PHONGO_ERROR_INVALID_ARGUMENT), &error_handling TSRMLS_CC);
	intern = Z_UTCDATETIME_OBJ_P(getThis());

#if SIZEOF_PHONGO_LONG == 8
	{
		phongo_long milliseconds;

		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &milliseconds) == FAILURE) {
			zend_restore_error_handling(&error_handling TSRMLS_CC);
			return;
		}

		intern->milliseconds = milliseconds;
	}
#elif SIZEOF_PHONGO_LONG == 4
	{
		char                *s_milliseconds;
		phongo_zpp_char_len  s_milliseconds_len;

		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &s_milliseconds, &s_milliseconds_len) == FAILURE) {
			zend_restore_error_handling(&error_handling TSRMLS_CC);
			return;
		}

		intern->milliseconds = STRTOLL(s_milliseconds);
	}
#else
# error Unsupported architecture (integers are neither 32-bit nor 64-bit)
#endif

	zend_restore_error_handling(&error_handling TSRMLS_CC);

}
/* }}} */
/* {{{ proto string UTCDateTime::__toString()
   Returns the string representation of the UTCDateTime */
PHP_METHOD(UTCDateTime, __toString)
{
	php_phongo_utcdatetime_t    *intern;
	char *tmp;
	int tmp_len;


	intern = Z_UTCDATETIME_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	tmp_len = spprintf(&tmp, 0, "%" PRId64, intern->milliseconds);
	PHONGO_RETVAL_STRINGL(tmp, tmp_len);
	efree(tmp);
}
/* }}} */
/* {{{ proto string UTCDateTime::toDateTime()
   Returns DateTime object representing this UTCDateTime */
PHP_METHOD(UTCDateTime, toDateTime)
{
	php_phongo_utcdatetime_t *intern;
	php_date_obj             *datetime_obj;
	char                     *sec;
	size_t                    sec_len;


	intern = Z_UTCDATETIME_OBJ_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	object_init_ex(return_value, php_date_get_date_ce());
	datetime_obj = Z_PHPDATE_P(return_value);

	sec_len = spprintf(&sec, 0, "@%" PRId64, intern->milliseconds / 1000);
	php_date_initialize(datetime_obj, sec, sec_len, NULL, NULL, 0 TSRMLS_CC);
	efree(sec);

	datetime_obj->time->f = (double) (intern->milliseconds % 1000) / 1000;
}
/* }}} */


/* {{{ BSON\UTCDateTime */

ZEND_BEGIN_ARG_INFO_EX(ai_UTCDateTime___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, milliseconds)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(ai_UTCDateTime___toString, 0, 0, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(ai_UTCDateTime_toDateTime, 0, 0, 0)
ZEND_END_ARG_INFO();


static zend_function_entry php_phongo_utcdatetime_me[] = {
	PHP_ME(UTCDateTime, __construct, ai_UTCDateTime___construct, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(UTCDateTime, __toString, ai_UTCDateTime___toString, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(UTCDateTime, toDateTime, ai_UTCDateTime_toDateTime, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(Manager, __wakeUp, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/* }}} */


/* {{{ php_phongo_utcdatetime_t object handlers */
static void php_phongo_utcdatetime_free_object(phongo_free_object_arg *object TSRMLS_DC) /* {{{ */
{
	php_phongo_utcdatetime_t *intern = Z_OBJ_UTCDATETIME(object);

	zend_object_std_dtor(&intern->std TSRMLS_CC);

#if PHP_VERSION_ID < 70000
	efree(intern);
#endif
} /* }}} */

phongo_create_object_retval php_phongo_utcdatetime_create_object(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	php_phongo_utcdatetime_t *intern = NULL;

	intern = PHONGO_ALLOC_OBJECT_T(php_phongo_utcdatetime_t, class_type);

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

#if PHP_VERSION_ID >= 70000
	intern->std.handlers = &php_phongo_handler_utcdatetime;

	return &intern->std;
#else
	{
		zend_object_value retval;
		retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t) zend_objects_destroy_object, php_phongo_utcdatetime_free_object, NULL TSRMLS_CC);
		retval.handlers = &php_phongo_handler_utcdatetime;

		return retval;
	}
#endif
} /* }}} */

HashTable *php_phongo_utcdatetime_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	php_phongo_utcdatetime_t *intern;
#if PHP_VERSION_ID >= 70000
	zval                      retval;
#else
	zval                      retval = zval_used_for_init;
#endif

	*is_temp = 1;
	intern =  Z_UTCDATETIME_OBJ_P(object);

	array_init(&retval);

#if SIZEOF_LONG == 4
	{
		char tmp[24];
		int tmp_len;

		tmp_len = snprintf(tmp, sizeof(tmp), "%" PRId64, intern->milliseconds);
		ADD_ASSOC_STRINGL(&retval, "milliseconds", tmp, tmp_len);
	}
#else
	ADD_ASSOC_LONG_EX(&retval, "milliseconds", intern->milliseconds);
#endif

	return Z_ARRVAL(retval);
} /* }}} */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(UTCDateTime)
{
	zend_class_entry ce;
	(void)type;(void)module_number;

	INIT_NS_CLASS_ENTRY(ce, BSON_NAMESPACE, "UTCDateTime", php_phongo_utcdatetime_me);
	php_phongo_utcdatetime_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_phongo_utcdatetime_ce->create_object = php_phongo_utcdatetime_create_object;
	PHONGO_CE_INIT(php_phongo_utcdatetime_ce);

	zend_class_implements(php_phongo_utcdatetime_ce TSRMLS_CC, 1, php_phongo_type_ce);
	memcpy(&php_phongo_handler_utcdatetime, phongo_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_phongo_handler_utcdatetime.get_debug_info = php_phongo_utcdatetime_get_debug_info;
#if PHP_VERSION_ID >= 70000
	php_phongo_handler_utcdatetime.free_obj = php_phongo_utcdatetime_free_object;
	php_phongo_handler_utcdatetime.offset = XtOffsetOf(php_phongo_utcdatetime_t, std);
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
