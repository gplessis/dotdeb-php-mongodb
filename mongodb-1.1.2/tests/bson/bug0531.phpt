--TEST--
PHPC-531: Segfault on invalid BSON
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$src = fromPHP(["hello" => "world"]);
$src[4] = 1;
$arr = MongoDB\BSON\toPHP($src);

?>
===DONE===
<?php exit(0); ?>
--EXPECT--

===DONE===
