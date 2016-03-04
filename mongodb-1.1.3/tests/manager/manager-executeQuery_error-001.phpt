--TEST--
MongoDB\Driver\Manager::executeQuery() should not issue warning before exception
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$query = new MongoDB\Driver\Query([]);

// Invalid host cannot be resolved
$manager = new MongoDB\Driver\Manager('mongodb://invalid.host:27017', ['serverSelectionTimeoutMS' => 1]);

echo throws(function() use ($manager, $query) {
    $manager->executeQuery(NS, $query);
}, 'MongoDB\Driver\Exception\ConnectionTimeoutException'), "\n";

// Valid host refuses connection
$manager = new MongoDB\Driver\Manager('mongodb://localhost:54321', ['serverSelectionTimeoutMS' => 1]);

echo throws(function() use ($manager, $query) {
    $manager->executeQuery(NS, $query);
}, 'MongoDB\Driver\Exception\ConnectionTimeoutException'), "\n";

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
OK: Got MongoDB\Driver\Exception\ConnectionTimeoutException
No suitable servers found (`serverselectiontryonce` set): %s
OK: Got MongoDB\Driver\Exception\ConnectionTimeoutException
No suitable servers found (`serverselectiontryonce` set): %s
===DONE===
