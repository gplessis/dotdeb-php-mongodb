--TEST--
BSON BSON\ObjectID #002 generates ObjectID for null or missing constructor argument
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$classname = BSON_NAMESPACE . "\\ObjectID";

var_dump(new $classname());
var_dump(new $classname(null));

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(MongoDB\BSON\ObjectID)#%d (%d) {
  ["oid"]=>
  string(24) "%x"
}
object(MongoDB\BSON\ObjectID)#%d (%d) {
  ["oid"]=>
  string(24) "%x"
}
===DONE===
