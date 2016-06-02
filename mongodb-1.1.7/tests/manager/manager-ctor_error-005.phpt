--TEST--
MongoDB\Driver\Manager::__construct(): invalid read preference
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

try {
    new MongoDB\Driver\Manager(STANDALONE, array('readPreference' => 'nothing'));
} catch (MongoDB\Driver\Exception\InvalidArgumentException $e) {
    echo "Exception\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Exception
===DONE===
