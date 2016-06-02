--TEST--
PHPC-672: ObjectID constructor should not modify string argument's memory
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$id = '56925B7330616224D0000001';
var_dump(new MongoDB\BSON\ObjectID($id));
var_dump($id);

$invalidId = 'T123456';
throws(function() use ($invalidId) {
    new MongoDB\BSON\ObjectID($invalidId);
}, "MongoDB\\Driver\\Exception\\InvalidArgumentException");
var_dump($invalidId);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(MongoDB\BSON\ObjectID)#%d (%d) {
  ["oid"]=>
  string(24) "56925b7330616224d0000001"
}
string(24) "56925B7330616224D0000001"
OK: Got MongoDB\Driver\Exception\InvalidArgumentException
string(7) "T123456"
===DONE===
