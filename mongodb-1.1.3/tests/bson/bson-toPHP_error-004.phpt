--TEST--
BSON\toPHP(): BSON decoding exceptions for bson_iter_visit_all() failure
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$tests = array(
    // Invalid UTF-8 character in root document's field name
    str_replace('INVALID!', "INVALID\xFE", fromPHP(['INVALID!' => 'bar'])),
    // Invalid UTF-8 character in embedded document's field name
    str_replace('INVALID!', "INVALID\xFE", fromPHP(['foo' => ['INVALID!' => 'bar']])),
    // Invalid UTF-8 character in string within array field
    str_replace('INVALID!', "INVALID\xFE", fromPHP(['foo' => ['INVALID!']])),
    /* Note: we don't use a three-character string in the underflow case, as
     * the 4-byte string length and payload (i.e. three characters + null byte)
     * coincidentally satisfy the expected size for an 8-byte double. We also
     * don't use a four-character string, since its null byte would be
     * interpreted as the document terminator. The actual document terminator
     * would then remain in the buffer and trigger a "did not exhaust" error.
     */
    pack('VCa*xVa*xx', 17, 1, 'foo', 3, 'ab'), // Invalid field type (underflow)
    pack('VCa*xVa*xx', 20, 1, 'foo', 6, 'abcde'), // Invalid field type (overflow)
);

foreach ($tests as $bson) {
    echo throws(function() use ($bson) {
        toPHP($bson);
    }, 'MongoDB\Driver\Exception\UnexpectedValueException'), "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
OK: Got MongoDB\Driver\Exception\UnexpectedValueException
Could not convert BSON document to a PHP variable
OK: Got MongoDB\Driver\Exception\UnexpectedValueException
Could not convert BSON document to a PHP variable
OK: Got MongoDB\Driver\Exception\UnexpectedValueException
Could not convert BSON document to a PHP variable
OK: Got MongoDB\Driver\Exception\UnexpectedValueException
Could not convert BSON document to a PHP variable
OK: Got MongoDB\Driver\Exception\UnexpectedValueException
Could not convert BSON document to a PHP variable
===DONE===
